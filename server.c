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
#include "responseCode.h"
#include "requestCode.h"
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

void cleanupHandler(void *arg)
{
  int cfd = *((int *)arg);
  printf("Called clean-up handler %d, %d\n", (int)pthread_self(), cfd);
  close(cfd);
  free(arg);
}

char *handleLogin(int cfd)
{
  int result;
  char buffer[1024];
  char username[MAX_USERNAME];
  // Send login code
  send(cfd, CONNECT_SUCCESS_RES_CODE, strlen(CONNECT_SUCCESS_RES_CODE), 0);
  while (1)
  {
    // Receive username
    memset(buffer, 0, sizeof(buffer));
    int result = recv(cfd, buffer, sizeof(buffer), 0);
    clearInput(buffer);

    // Check quit command
    if (result <= 0 || strcasecmp(buffer, QUIT_REQ_CODE) == 0)
    {
      send(cfd, QUIT_RES_CODE, strlen(QUIT_RES_CODE), 0);
      return NULL;
    }

    // Check username
    strcpy(username, buffer + 6);
    printf("%s\n", username);
    if (isValidLoginCommand(buffer) && isValidUsername(buffer))
    {
      // Send success login code
      send(cfd, LOGIN_SUCCESS_RES_CODE, strlen(LOGIN_SUCCESS_RES_CODE), 0);
      break;
    }
    else
    {
      // Send failure login code
      send(cfd, LOGIN_FAIL_RES_CODE, strlen(LOGIN_FAIL_RES_CODE), 0);
    }
  }

  char *returnValue = (char *)malloc((strlen(username) + 1) * sizeof(char));
  strcpy(returnValue, username);
  return returnValue;
}

void sendMatchedResponse(Room room)
{
  char matchedCode[100];
  sprintf(matchedCode, "%s 0 %d\r", MATCHED_RES_CODE, room.currentPlayer);
  send(room.player1.cfd, matchedCode, strlen(matchedCode), 0);
  sprintf(matchedCode, "%s 1 %d\r", MATCHED_RES_CODE, room.currentPlayer);
  send(room.player2.cfd, matchedCode, strlen(matchedCode), 0);
}

void sendDrawResponse(Room room)
{
  char drawCode[100];
  char encodedBoard[10];
  stringifyBoard(encodedBoard, room.board);
  sprintf(drawCode, "%s %s\r", DRAW_RES_CODE, encodedBoard);
  send(room.player1.cfd, drawCode, strlen(drawCode), 0);
  send(room.player2.cfd, drawCode, strlen(drawCode), 0);
}

void sendWinResponse(Room room)
{
  char winCode[100];
  char encodedBoard[10];
  stringifyBoard(encodedBoard, room.board);
  sprintf(winCode, "%s %d %s\r", WIN_RES_CODE, room.currentPlayer, encodedBoard);
  send(room.player1.cfd, winCode, strlen(winCode), 0);
  send(room.player2.cfd, winCode, strlen(winCode), 0);
}

void sendNextTurnResponse(Room room)
{
  char nextCode[100];
  char encodedBoard[10];
  stringifyBoard(encodedBoard, room.board);
  sprintf(nextCode, "%s %d %s\r", NEXT_RES_CODE, room.currentPlayer, encodedBoard);

  send(room.player1.cfd, nextCode, strlen(nextCode), 0);
  send(room.player2.cfd, nextCode, strlen(nextCode), 0);
}

void sendQuitResponse(Room room)
{
  send(room.player1.cfd, QUIT_RES_CODE, strlen(QUIT_RES_CODE), 0);
  send(room.player2.cfd, QUIT_RES_CODE, strlen(QUIT_RES_CODE), 0);
}

int isMyTurn(Room room, pthread_t selfID)
{
  return room.currentPlayer == 0
             ? room.player1.threadID == selfID
             : room.player2.threadID == selfID;
}

void *handleClient(void *arg)
{
  int cfd = *((int *)arg);
  char buffer[1024];
  int result;
  pthread_t myThreadID = pthread_self();

  // Setup cleanup handler
  pthread_cleanup_push(cleanupHandler, arg);

  char *username = handleLogin(cfd);
  if (username == NULL)
    return NULL;

  // Check waiting player -> wait for oponent or create room and start game
  Player newPlayer;
  strcpy(newPlayer.username, username);
  newPlayer.cfd = cfd;
  newPlayer.threadID = myThreadID;

  int roomIndex = -1;
  if (waitingPlayer == NULL)
  {
    // Set new player to waiting
    waitingPlayer = &newPlayer;
    send(cfd, WAIT_RES_CODE, strlen(WAIT_RES_CODE), 0);
  }
  else
  {
    // Create new room
    roomIndex = createRoom(rooms, *waitingPlayer, newPlayer);
    numRooms++;
    waitingPlayer = NULL;
    printf("Create room: %d for %d and %d\n", roomIndex, (int)rooms[roomIndex].player1.threadID, (int)rooms[roomIndex].player2.threadID);
    fflush(stdout);

    // Send matched code
    sendMatchedResponse(rooms[roomIndex]);
  }
  free(username);

  while (1)
  {
    memset(buffer, 0, sizeof(buffer));
    result = recv(cfd, buffer, sizeof(buffer), 0);
    clearInput(buffer);
    if (result > 0 && roomIndex == -1)
    {
      roomIndex = findRoom(rooms, newPlayer.username);
      // Not found any room but client sent move
      if (roomIndex == -1)
      {
        send(cfd, WAIT_RES_CODE, strlen(WAIT_RES_CODE), 0);
        continue;
      }
    }
    if (result <= 0 || strcasecmp(buffer, QUIT_REQ_CODE) == 0)
      break;

    // Check turn
    if (!isMyTurn(rooms[roomIndex], myThreadID))
    {
      send(cfd, WAIT_RES_CODE, strlen(WAIT_RES_CODE), 0);
      continue;
    }

    // Check move
    if (!isValidMove(rooms[roomIndex].board, buffer))
    {
      send(cfd, INVALID_MOVE_RES_CODE, strlen(INVALID_MOVE_RES_CODE), 0);
      continue;
    }
    // Update current state
    int position = buffer[5] - '0';
    rooms[roomIndex].board[position] = rooms[roomIndex].currentPlayer == 0 ? 'X' : 'O';
    if (isDraw(rooms[roomIndex].board))
    {
      // Send draw code
      sendDrawResponse(rooms[roomIndex]);
      break;
    }
    else if (isWin(rooms[roomIndex].board))
    {
      // Send win code
      sendWinResponse(rooms[roomIndex]);
      break;
    }
    else
    {
      // Update next state
      rooms[roomIndex].currentPlayer = !rooms[roomIndex].currentPlayer;
      rooms[roomIndex].numTurns++;
      // Send back next state
      sendNextTurnResponse(rooms[roomIndex]);
    }
  }

  sendQuitResponse(rooms[roomIndex]);
  pthread_t id = getOponentThreadID(rooms[roomIndex], myThreadID);
  resetRoom(&rooms[roomIndex]);
  numRooms--;

  pthread_cancel(id);
  pthread_cleanup_pop(1);
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

      printf("New connection handle by %d, %d\n", (int)threadIDs[numThread], tmp);
      numThread++;
    }
  }
}