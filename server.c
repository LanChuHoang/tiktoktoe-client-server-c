#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include "helpers.h"
#include "response.h"
#include "Room.h"

Room rooms[MAX_ROOM];
int numRooms = 0;

pthread_t *threadIDs;
int numThread = 0;

Player *waitingPlayer = NULL;

int isValidUsername(char *username)
{
  if (strlen(username) == 0)
    return 0;
  if (waitingPlayer != NULL && strcmp(waitingPlayer->username, username) == 0)
    return 0;
  return findRoom(rooms, username) == -1;
}

char *handleLogin(int cfd)
{
  int result;
  char buffer[1024];
  // Send login code
  send(cfd, LOGIN_CODE, strlen(LOGIN_CODE), 0);
  while (1)
  {
    // Receive username
    memset(buffer, 0, sizeof(buffer));
    int result = recv(cfd, buffer, sizeof(buffer), 0);
    clearInput(buffer);

    // Check quit command
    if (result <= 0 || strcasecmp(buffer, "quit") == 0)
      return NULL;

    // Check username
    if (isValidUsername(buffer))
    {
      // Send success login code
      send(cfd, LOGIN_SUCCESS_CODE, strlen(LOGIN_SUCCESS_CODE), 0);

      char *result = (char *)malloc((strlen(buffer) + 1) * sizeof(char));
      strcpy(result, buffer);
      return result;
    }
    else
    {
      // Send failure login code
      send(cfd, LOGIN_FAIL_CODE, strlen(LOGIN_FAIL_CODE), 0);
    }
  }
}

void sendMatchedResponse(Room room)
{
  char matchedCode[100];

  sprintf(matchedCode, "%s 0 %d \n", MATCHED_CODE, room.currentPlayer);
  send(room.player1.cfd, matchedCode, strlen(matchedCode), 0);

  sprintf(matchedCode, "%s 1 %d \n", MATCHED_CODE, room.currentPlayer);
  send(room.player2.cfd, matchedCode, strlen(matchedCode), 0);
}

void sendDrawResponse(Room room)
{
  send(room.player1.cfd, DRAW_CODE, strlen(DRAW_CODE), 0);
  send(room.player2.cfd, DRAW_CODE, strlen(DRAW_CODE), 0);
}

void sendWinResponse(Room room)
{
  char winCode[100];
  sprintf(winCode, "%s %d\n", WIN_CODE, room.currentPlayer);
  send(room.player1.cfd, winCode, strlen(winCode), 0);
  send(room.player2.cfd, winCode, strlen(winCode), 0);
}

void sendNextTurnResponse(Room room)
{
  char nextCode[100];
  sprintf(nextCode, "%s %d \n", NEXT_CODE, room.currentPlayer);

  send(room.player1.cfd, nextCode, strlen(nextCode), 0);
  send(room.player2.cfd, nextCode, strlen(nextCode), 0);
}

void *handleClient(void *arg)
{
  int cfd = *((int *)arg);
  char buffer[1024];
  int result;

  char *username = handleLogin(cfd);
  if (username == NULL)
  {
    free(arg);
    close(cfd);
    return NULL;
  }

  // Check waiting player -> wait for oponent or create room and start game
  Player newPlayer;
  newPlayer.cfd = cfd;
  strcpy(newPlayer.username, username);

  int roomIndex = -1;
  if (waitingPlayer == NULL)
  {
    // Set new player to waiting
    waitingPlayer = &newPlayer;
    send(cfd, WAIT_CODE, strlen(WAIT_CODE), 0);
  }
  else
  {
    // Create new room
    roomIndex = createRoom(rooms, *waitingPlayer, newPlayer);
    numRooms++;
    waitingPlayer = NULL;

    // Send matched code
    sendMatchedResponse(rooms[roomIndex]);
  }
  free(username);

  while (1)
  {
    memset(buffer, 0, sizeof(buffer));
    result = recv(cfd, buffer, sizeof(buffer), 0);
    clearInput(buffer);
    if (result <= 0 || strcasecmp(buffer, "quit") == 0)
      break;

    if (roomIndex == -1)
      roomIndex = findRoom(rooms, newPlayer.username);

    // Update current state
    int position = atoi(buffer);
    rooms[roomIndex].board[position] = rooms[roomIndex].currentPlayer == 0 ? 'X' : 'O';
    if (isDraw(rooms[roomIndex].board))
    {
      // Send draw code
      sendDrawResponse(rooms[roomIndex]);
    }
    else if (isWin(rooms[roomIndex].board))
    {
      // Send win code
      sendWinResponse(rooms[roomIndex]);
    }
    else
    {
      // Update next state
      rooms[roomIndex].currentPlayer = !rooms[roomIndex].currentPlayer;
      rooms[roomIndex].numTurns++;
      // Send back next state
      sendNextTurnResponse(rooms[roomIndex]);
    }

    for (int i = 0; i < 9; ++i)
    {
      if (rooms[roomIndex].board[i] == 0)
        printf("_ ");
      else
        printf("%c ", rooms[roomIndex].board[i]);
      if ((i + 1) % 3 == 0)
        printf("\n");
    }
  }
  free(arg);
  close(cfd);
  return NULL;
}

int main(int argc, char **argv)
{
  initRooms(rooms);

  int PORT = 8000;
  int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in saddr;
  struct sockaddr caddr;          // Bien ra chua dia chi client noi den
  socklen_t clen = sizeof(caddr); // Bien vao + ra chua so byte duoc ghi vao caddr
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(PORT);
  saddr.sin_addr.s_addr = 0; // ANY ADDRESS
  bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
  listen(sfd, 10);

  printf("Server is running at port %d\n", PORT);
  while (1)
  {
    int tmp = accept(sfd, (struct sockaddr *)&caddr, &clen);
    if (tmp >= 0)
    {
      threadIDs = realloc(threadIDs, (numThread + 1) * sizeof(pthread_t));

      int *arg = (int *)calloc(1, sizeof(int));
      *arg = tmp;
      pthread_create(&threadIDs[numThread], NULL, handleClient, arg);

      printf("New connection handle by %d\n", (int)threadIDs[numThread]);
      numThread++;
    }
  }
}