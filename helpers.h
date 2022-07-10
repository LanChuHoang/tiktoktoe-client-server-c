#include <string.h>

void clearInput(char *text)
{
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

int isValidMove(char *board, int position)
{
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