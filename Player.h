#include <pthread.h>
#define MAX_USERNAME 20

typedef struct player
{
  char username[20];
  int cfd;
  pthread_t threadID;
} Player;

void resetPlayer(Player *player)
{
  player->cfd = 0;
  memset(player->username, 0, sizeof(player->username));
}