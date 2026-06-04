#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "db.h"


#define IP INADDR_LOOPBACK

void error_check(int status, const char* msg) {
    if (status < 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void check_syntax(char* query, char tokens[][size]) {
    char *word = strtok(query, " \n\t");
    int count = 0;
    while (word != NULL && count < TOKENS_MAX) {
        strcpy(tokens[(count)++], word);
        word = strtok(NULL, " \n\t");
    }
    sprintf(tokens[TOKENS_MAX - 1], "%d", count);
    if (strcmp(tokens[0], "SELECT") == 0) {
        if (count > 2 && strcmp(tokens[1], "*") == 0) {
            printf("Invaled syntax...\nUsage: SELECT *\n");
            return;
        }
        if (count > 5 || count < 2) {
            printf("Invaled syntax...\nUsage: SELECT id name age\nUsage: SELECT *\n");
            return;
        }
    } else if (strcmp(tokens[0], "INSERT") == 0) {
        if (count > 5 || count < 2) {
            printf("Invaled syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
            return;
        }
        for (int i = 1; i < count; ++i) {
            char *ptr;
            if ((ptr = strchr(tokens[i], '=')) == NULL) {
                printf("Invaled syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
                return;
            } else if (*(ptr+1) == ' ' || *(ptr-1) == ' ') {
                printf("Invaled syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
                return;
            } else if (*(ptr+1) == '\"' && strchr(ptr+2, '\"') == NULL) {
                printf("Invaled syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
                return;
            }
        }
    }
    else if (strcmp(tokens[0], "DELETE") == 0)
    {
        if (count != 2) {
            printf("Invaled syntax...\nUsage: DELETE name=\"Mary\" //delete by name, or id, or age\n");
            return;
        }
        char *ptr;
        if ((ptr = strchr(tokens[1], '=')) == NULL){
            printf("Invaled syntax...\nUsage: DELETE name=\"Mary\" //delete by name, or id, or age\n");
            return;
        } else if (*(ptr + 1) == ' ' || *(ptr - 1) == ' ') {
            printf("Invaled syntax...\nUsage: DELETE name=\"Mary\" //delete by name, or id, or age\n");
            return;
        } else if (*(ptr + 1) == '\"' && strchr(ptr + 2, '\"') == NULL)
        {
            printf("Invaled syntax...\nUsage: DELETE name=\"Mary\" //delete by name, or id, or age\n");
            return;
        }
    }
    else
    {
        printf("Invalid command\n");
    }
}

int main() {
    int c_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(c_socket, "socket failed");

    struct sockaddr_in c_addr;
    c_addr.sin_port = htons(PORT);
    c_addr.sin_family = AF_INET;
    c_addr.sin_addr.s_addr = htonl(IP);
    
    if (connect(c_socket, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
        perror("connect failed");
        close(c_socket);
        exit(EXIT_FAILURE);
    }

    //SELECT name salary 
    //SELECT *
    //INSERT name="Mary" age=15 salary=156.5
    //DELETE name="Alise"

    char tokens[TOKENS_MAX][size] = {0};
    char query[SIZE] = {0};
    char response[SIZE] = {0};
    while (strncmp(query, "exit", 4) != 0) {
        printf("Input your query#\n");
        fgets(query, SIZE, stdin);
        check_syntax(query, tokens);
        if (send(c_socket, tokens, TOKENS_MAX*size, 0) < 0) {
            perror("send failed");
            close(c_socket);
        }
        if (strcmp(tokens[0], "INSERT") == 0) {
            if (recv(c_socket, response, SIZE, 0) < 0) {
                perror("send failed");
                close(c_socket);
            }
            printf("%s\n", response);
        } else if (strcmp(tokens[0], "SELECT") == 0) {
            if (recv(c_socket, response, size, 0) < 0) {
                perror("send failed");
                close(c_socket);
            }
            int count = atoi(response);
            char rows[count][SIZE];
            memset(rows, 0, sizeof(rows));
            if (recv(c_socket, rows, count*SIZE, 0) < 0) {
                perror("recv failed");
                close(c_socket);
            }
            if (strncmp(response, "Error", 5) == 0) {
                printf("%s\n", response);
            } else  {
                for (int i = 0; i < count; ++i) {
                    printf("%s\n", rows[i]);
                }
            } 
        }
        memset(query, 0, SIZE);
        memset(response, 0, SIZE);
        memset(tokens, 0, TOKENS_MAX * size);
    }
}