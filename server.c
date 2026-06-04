#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "db.h"

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int Record_Size = sizeof(Record);
int id = 0;
int db;

void error_check(int status, const char *msg) {
    if (status < 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}


void insert(int conn_sock, int db, char tokens[TOKENS_MAX][size], int count) {
    char key[size] = {0};
    char value[size] = {0};
    char *response = "";
    int ok = 1;
   
    Record data = {0};
    for (int i = 1; i < count; ++i) {
        sscanf(tokens[i], "%[^=]=%s", key, value);
        if (strcmp(key, "name") == 0) {
            strcpy(data.name, value);
        } else if (strcmp(key, "age") == 0) {
            data.age = atoi(value);
            if (data.age == 0) {
                response = "Error: the age must be a positive number.";
                ok = 0;
                break;
            }
        } else if (strcmp(key, "salary") == 0) {
            data.salary = atof(value);
            if (data.salary == 0) {
                response = "Error: the salary must be a positive number.";
                ok = 0;
                break;
            }
        } else {
            response = "Error: the matching key was not found in the table.";
            ok = 0;
            break;
        }
    }  
    if (ok) {

        pthread_mutex_lock(&mtx);

        data.id = id + 1;
        ssize_t ws = pwrite(db, &data, sizeof(data), id*Record_Size);
        if (ws != Record_Size) {
            perror("write failed");
            close(conn_sock);
            exit(EXIT_FAILURE);
        }
        fsync(db);
        ++id;

        pthread_mutex_unlock(&mtx);

        response = "Success! Your record is now stored in the database.";
    }
    if (send(conn_sock, response, strlen(response), 0) < 0) {
        perror("send failed");
        close(conn_sock);
        exit(EXIT_FAILURE);
    }
}

void Select(int conn_sock, int db, char tokens[TOKENS_MAX][size], int count) {
    char *response;
    pthread_mutex_lock(&mtx);

    off_t file_size = lseek(db, 0, SEEK_END);
    int records_count = file_size / Record_Size;
    Record *records = calloc(records_count, Record_Size);
    lseek(db, 0, SEEK_SET);
    ssize_t rb = read(db, records, file_size);
    if (rb != file_size) {
        perror("read failed");
        free(records);
        pthread_mutex_unlock(&mtx);
        response = "Error: read failed";
        send(conn_sock, response, strlen(response), 0);
        return;
    }

    int ok = 1;
    char rows[records_count][SIZE];
    memset(rows, 0, sizeof(rows));
    int k = 0;
    for (int i = 0; i < records_count; ++i) {
        if (records[i].id != -1) {
            sprintf(rows[k], "%d", records[i].id);
            strcat(rows[k], " ");
            for (int j = 1; j < count; ++j) {
                if (strcmp(tokens[j], "name") == 0) {
                    strcat(rows[k], records[i].name);
                    strcat(rows[k], " ");
                } else if (strcmp(tokens[j], "age") == 0) {
                    char str_age[16] = {0};
                    sprintf(str_age, "%d", records[i].age);
                    strcat(rows[k], str_age);
                    strcat(rows[k], " ");
                } else if (strcmp(tokens[j], "salary") == 0) {
                    char str_salary[size] = {0};
                    sprintf(str_salary, "%lf", records[i].salary);
                    strcat(rows[k], str_salary);
                } else {
                    response = "Error: the matching keys was not found in the table.";
                    ok = 0;
                    break;
                }
            }
            if (!ok) {
                break;
            }
            ++k;
        }
    }
    pthread_mutex_unlock(&mtx);
    if (ok) {
        char str_count[size] = {0};
        sprintf(str_count, "%d", k);
        if (send(conn_sock, str_count, size, 0) < 0) {
            perror("send failed");
            close(conn_sock);
            free(records);
            exit(EXIT_FAILURE);
        } if (send(conn_sock, rows, k * SIZE, 0) < 0) {
            perror("send failed");
            close(conn_sock);
            free(records);
            exit(EXIT_FAILURE);
        }
    } else {
        if (send(conn_sock, response, strlen(response), 0) < 0) {
            perror("send failed");
            close(conn_sock);
            free(records);
            exit(EXIT_FAILURE);
        }
    }
    free(records);
}

void Delete(int conn_sock, int db, char tokens[TOKENS_MAX][size]) {
    pthread_mutex_lock(&mtx);

    Record data = {0};
    int deleted_id = atoi(&tokens[1][3]);
    --deleted_id;
    char *response = "";
    if (deleted_id >= id) {
        response = "Could not delete, we do not have such an id.";
    } else {
        ssize_t s = pread(db, &data, sizeof(data), deleted_id*Record_Size);
        if (s != Record_Size) {
            perror("write failed");
            close(conn_sock);
            exit(EXIT_FAILURE);
        }
        if (data.id == -1) {
            response = "Your record is already deleted.";
        } else {
            memset(&data, 0, sizeof(data));
            data.id = -1;
            s = pwrite(db, &data, sizeof(data), deleted_id*Record_Size);
            if (s != Record_Size) {
                perror("write failed");
                close(conn_sock);
                exit(EXIT_FAILURE);
            }
            fsync(db);
            response = "Success! Your record is deleted.";
        }
    }

    pthread_mutex_unlock(&mtx);

    if (send(conn_sock, response, strlen(response), 0) < 0) {
        perror("send failed");
        close(conn_sock);
        exit(EXIT_FAILURE);
    }
}

void *client_handler(void *arg) {
    int conn_sock = *((int*)arg);
    char tokens[TOKENS_MAX][size] = {0};
    int count = 0;

    while (1) {
        if (recv(conn_sock, tokens, TOKENS_MAX*size, 0) < 0) {
            perror("recv failed");
            close(conn_sock);
            free(arg);
            exit(EXIT_FAILURE);
        }
        count = atoi(tokens[TOKENS_MAX - 1]);
        if (strncmp(tokens[0], "exit", 4) == 0) {
            close(conn_sock);
            free(arg);
            return NULL;
        } if (strcmp(tokens[0], "INSERT") == 0) {
            insert(conn_sock, db, tokens, count);
        } else if (strcmp(tokens[0], "SELECT") == 0) {
            Select(conn_sock, db, tokens, count);
        } else {
            Delete(conn_sock, db, tokens);
        }
        memset(tokens, 0, TOKENS_MAX * size);
    }
}
    

int main() {
    int s_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(s_socket, "socket failed");

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    int ret = setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    error_check(ret, "setsockopt failed");

    ret = bind(s_socket, (struct sockaddr *)&saddr, sizeof(saddr));
    error_check(ret, "bind failed");

    ret = listen(s_socket, SOMAXCONN);
    error_check(ret, "listen failed");
    printf("db is waiting for a request...\n");
 
    db = open("db.txt", O_RDWR | O_CREAT, 0644);
    if (db < 0) {
        perror("db open failed");
        close(s_socket);
        exit(EXIT_FAILURE);
    }
    off_t file_size = lseek(db, 0, SEEK_END);
    id = file_size/Record_Size;

    while (1) {
        int conn_sock = accept(s_socket, NULL, NULL);
        if (conn_sock < 0) {
            perror("accept failed");
            close(s_socket);
            exit(EXIT_FAILURE);
        }
        int *conn_sock_ptr = malloc(sizeof(int));
        *conn_sock_ptr = conn_sock;
        pthread_t pth_id;
        ret = pthread_create(&pth_id, NULL, client_handler, conn_sock_ptr);
        error_check(ret, "pthread_create failed");
        
        ret = pthread_detach(pth_id);
        error_check(ret, "pthread_detach");
    }
}