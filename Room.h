#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "Player.h"

#define MAX_ROOM 20

typedef struct room
{
  int isAvailable;
  char board[9];
  int currentPlayer;
  int numTurns;
  Player player1;
  Player player2;
} Room;

int findRoom(Room *rooms, char *username)
{
  for (int i = 0; i < MAX_ROOM; ++i)
  {
    if (rooms[i].isAvailable)
      continue;
    if (strcmp(rooms[i].player1.username, username) == 0 || strcmp(rooms[i].player2.username, username) == 0)
      return i;
  }
  return -1;
}

int createRoom(Room *rooms, Player player1, Player player2)
{
  int roomIndex = -1;
  for (int i = 0; i < MAX_ROOM; ++i)
  {
    if (rooms[i].isAvailable)
    {
      roomIndex = i;
      rooms[i].isAvailable = 0;
      break;
    }
  }
  if (roomIndex == -1)
    return -1;

  strcpy(rooms[roomIndex].player1.username, player1.username);
  rooms[roomIndex].player1.cfd = player1.cfd;
  rooms[roomIndex].player1.threadID = player1.threadID;
  strcpy(rooms[roomIndex].player2.username, player2.username);
  rooms[roomIndex].player2.cfd = player2.cfd;
  rooms[roomIndex].player2.threadID = player2.threadID;
  int firstPlayer = rand() % 2;
  rooms[roomIndex].currentPlayer = firstPlayer;
  return roomIndex;
}

void initRooms(Room *rooms)
{
  srand(time(0));
  for (int i = 0; i < MAX_ROOM; ++i)
  {
    rooms[i].isAvailable = 1;
    rooms[i].currentPlayer = 0;
    rooms[i].numTurns = 0;
  }
}

void resetRoom(Room *room)
{
  room->isAvailable = 1;
  memset(room->board, 0, sizeof(room->board));
  room->currentPlayer = 0;
  room->numTurns = 0;
  resetPlayer(&room->player1);
  resetPlayer(&room->player2);
}

pthread_t getOponentThreadID(Room room, pthread_t selfID)
{
  return (room.player1.threadID == selfID)
             ? room.player2.threadID
             : room.player1.threadID;
}