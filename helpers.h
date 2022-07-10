#include <string.h>
#include <ctype.h>
#include "requestCode.h"

void clearInput(char *text)
{
  if (text[strlen(text) - 1] == '\0')
    text[strlen(text) - 1] = '\0';
}

int isDraw(char *board)
{
  for (int i = 0; i < 9; i++)
  {
    if (board[i] == 0)
      return 0;
  }
  return 1;
}

int isWin(char *board)
{
  int combination[8][3] = {
      {0, 1, 2},
      {3, 4, 5},
      {6, 7, 8},
      {0, 3, 6},
      {1, 4, 7},
      {2, 5, 8},
      {0, 4, 8},
      {2, 4, 6},
  };
  for (int i = 0; i < 8; i++)
  {
    int x = combination[i][0];
    int y = combination[i][1];
    int z = combination[i][2];

    if (board[x] != 0 && board[x] == board[y] && board[y] == board[z])
      return 1;
  }
  return 0;
}

void printBoard(char *board)
{
  for (int i = 0; i < 9; ++i)
  {
    if (board[i] == 0)
      printf("_ ");
    else
      printf("%c ", board[i]);
    if ((i + 1) % 3 == 0)
      printf("\n");
  }
}

void printEmptyBoard()
{
  for (int i = 0; i < 9; ++i)
  {
    printf("_ ");
    if ((i + 1) % 3 == 0)
      printf("\n");
  }
}

int isValidLoginCommand(char *request)
{
  return strlen(request) > 6 && strncasecmp(request, LOGIN_REQ_CODE, 5) == 0;
}

int isValidMove(char *board, char *request)
{
  if (strlen(request) <= 5 || strncasecmp(request, MOVE_REQ_CODE, 4) != 0 || !isdigit(request[5]))
    return 0;
  int position = request[5] - '0';
  return position >= 0 && position <= 8 && board[position] == 0;
}

void stringifyBoard(char *result, char *board)
{
  for (int i = 0; i < 9; ++i)
  {
    result[i] = board[i] == 0 ? '_' : board[i];
  }
  result[9] = '\0';
}