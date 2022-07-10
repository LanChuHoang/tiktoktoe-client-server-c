#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "responseCode.h"
#include "requestCode.h"
#include "helpers.h"

int sfd;
pthread_t send_tid, recv_tid;
char playerNumber;

void printTurn(char currentPlayer)
{
    if (currentPlayer == playerNumber)
        printf("Your turn, please enter your move: ");
    else
        printf("Oponent's turn, please wait.\n");
}

void handleResponseCode(char *response)
{
    if (strncmp(response, CONNECT_SUCCESS_RES_CODE, 15) == 0)
    {
        printf("Server connected, please enter login command: ");
    }
    else if (strncmp(response, LOGIN_SUCCESS_RES_CODE, 13) == 0)
    {
        printf("Login successfully.\n");
    }
    else if (strncmp(response, LOGIN_FAIL_RES_CODE, 10) == 0)
    {
        printf("Login failed, please try again: ");
    }
    else if (strncmp(response, WAIT_RES_CODE, 4) == 0)
    {
        printf("Please wait.\n");
    }
    else if (strncmp(response, MATCHED_RES_CODE, 7) == 0)
    {
        playerNumber = response[8];
        char firstPlayer = response[10];
        printf("Matched found, you are player %c.\n", playerNumber);
        printEmptyBoard();
        printTurn(firstPlayer);
    }
    else if (strncmp(response, WIN_RES_CODE, 3) == 0)
    {
        char board[10];
        strcpy(board, response + 6);
        printBoard(board);
        char winPlayer = response[4];
        if (playerNumber == winPlayer)
            printf("You won!\n");
        else
            printf("You lose!\n");
    }
    else if (strncmp(response, DRAW_RES_CODE, 4) == 0)
    {
        char board[10];
        strcpy(board, response + 5);
        printBoard(board);
        printf("Draw!\n");
    }
    else if (strncmp(response, NEXT_RES_CODE, 4) == 0)
    {
        char board[10];
        strcpy(board, response + 7);
        printBoard(board);
        char currentPlayer = response[5];
        printTurn(currentPlayer);
    }
    else if (strncmp(response, INVALID_MOVE_RES_CODE, 12) == 0)
    {
        printf("Invalid move, please try again: ");
    }
    fflush(stdout);
}

int isEnd(char *response)
{
    return strcmp(response, QUIT_RES_CODE) == 0;
}

void *send_thread_handler(void *argv)
{
    char buffer[1024];
    int resutl;
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        gets(buffer);
        if (strlen(buffer) == 0)
        {
            printf("Invalid command, please try again: ");
            continue;
        }
        int result = send(sfd, buffer, strlen(buffer), 0);
        if (result <= 0 || strncasecmp(buffer, QUIT_REQ_CODE, 4) == 0)
            break;
    }
    printf("exit\n");
    exit(0);
    return NULL;
}

void *recv_thread_handler(void *argv)
{
    char buffer[1024];
    int result;
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        result = recv(sfd, buffer, sizeof(buffer), 0);
        if (result <= 0 || isEnd(buffer))
            break;
        if (strlen(buffer) > 0)
        {
            char *code = strtok(buffer, "\r");
            while (code != NULL)
            {
                handleResponseCode(code);
                code = strtok(NULL, "\r");
            }
        }
    }
    printf("exit");
    exit(0);
    return NULL;
}

int main()
{
    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int result = connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (result >= 0)
    {
        pthread_create(&send_tid, NULL, &send_thread_handler, NULL);
        pthread_create(&recv_tid, NULL, &recv_thread_handler, NULL);
        pthread_join(send_tid, NULL);
        pthread_join(recv_tid, NULL);
    }
    close(sfd);
    return 0;
}