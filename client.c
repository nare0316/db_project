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

int check_syntax(char* query, char tokens[][size]) {
    char *word = strtok(query, " \n\t");
    int count = 0;
    while (word != NULL && count < TOKENS_MAX) {
        strcpy(tokens[(count)++], word);
        word = strtok(NULL, " \n\t");
    }
    sprintf(tokens[TOKENS_MAX - 1], "%d", count);
    if (strcmp(tokens[0], "SELECT") == 0) {
        if (count > 2 && strcmp(tokens[1], "*") == 0) {
            printf("Invalid syntax...\nUsage: SELECT *\n");
            return 0;
        }
        if (count > 5 || count < 2) {
            printf("Invalid syntax...\nUsage: SELECT id name age\nUsage: SELECT *\n");
            return 0;
        }
        if (tokens[1][0] == '*') {
            sprintf(tokens[TOKENS_MAX - 1], "%d", 4);
            strcpy(tokens[1], "name");
            strcpy(tokens[2], "age");
            strcpy(tokens[3], "salary");
        }
    } else if (strcmp(tokens[0], "INSERT") == 0) {
        if (count != 4) {
            printf("Invalid syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
            return 0;
        }
        for (int i = 1; i < count; ++i) {
            char *ptr;
            if ((ptr = strchr(tokens[i], '=')) == NULL) {
                printf("Invalid syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
                return 0;
            } else if (*(ptr+1) == ' ' || *(ptr-1) == ' ') {
                printf("Invalid syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
                return 0;
            } else if (*(ptr+1) == '\"' && strchr(ptr+2, '\"') == NULL) {
                printf("Invalid syntax...\nUsage: INSERT name=\"Mary\" age=15 salary=156.5\n");
                return 0;
            }
        }
    } else if (strcmp(tokens[0], "DELETE") == 0) {
        if (count != 2 || strncmp(tokens[1], "id", 2) != 0) {
            printf("Invalid syntax...\nUsage: DELETE id=[your id]\n");
            return 0;
        }
        int id = atoi(&tokens[1][3]);
        if (id == 0) {
            printf("Invalid syntax...ID must be a positive number...Usage: DELETE id=[your id]\n");
            return 0;
        }
        char *ptr;
        if ((ptr = strchr(tokens[1], '=')) == NULL) {
            printf("Invalid syntax...\nUsage: DELETE id=[your id]\n");
            return 0;
        } else if (*(ptr + 1) == ' ' || *(ptr - 1) == ' ') {
            printf("Invalid syntax...\nUsage: DELETE id=[your id]\n");
            return 0;
        } 
    } else {
        printf("Invalid command\n");
        return 0;
    }
    return 1;
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
    //DELETE id=1

    char tokens[TOKENS_MAX][size] = {0};
    char query[SIZE] = {0};
    char response[SIZE] = {0};
    while (1) {
        printf("Input your query# ");
        fgets(query, SIZE, stdin);
        if (strncmp(query, "exit", 4) == 0) {
            strcpy(tokens[0], "exit");
            if (send(c_socket, tokens, TOKENS_MAX*size, 0) < 0) {
                perror("send failed");
                close(c_socket);
            }
            break;
        } 
        if (strncmp(query, "clear", 5) == 0) {
            system("clear");
            continue;
        }
        if (!check_syntax(query, tokens)) {
            memset(query, 0, SIZE);
            memset(tokens, 0, TOKENS_MAX * size);
            continue;
        }
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
            if (strncmp(response, "Error", 5) == 0) {
                printf("%s\n", response);
                memset(query, 0, SIZE);
                memset(response, 0, SIZE);
                memset(tokens, 0, TOKENS_MAX * size);
                continue;
            }
            int count = atoi(response);
            char rows[count][SIZE];
            memset(rows, 0, sizeof(rows));
            if (recv(c_socket, rows, count*SIZE, 0) < 0) {
                perror("recv failed");
                close(c_socket);
            }
            printf("%-20s", "id"); 
            for (int i = 1; i < TOKENS_MAX - 1; ++i) {
                printf("%-20s", tokens[i]); 
            }
            printf("\n");
            for (int i = 0; i < count; ++i) {
                char *data = strtok(rows[i], " ");
                printf("%-20s", data); 
                while ((data = strtok(NULL, " ")) != NULL) {
                    printf("%-20s", data);
                }
                printf("\n");
            }
        } else  {
            if (recv(c_socket, response, SIZE, 0) < 0) {
                perror("send failed");
                close(c_socket);
            }
            printf("%s\n", response);
        }
        memset(query, 0, SIZE);
        memset(response, 0, SIZE);
        memset(tokens, 0, TOKENS_MAX * size);
    }
    close(c_socket);
}