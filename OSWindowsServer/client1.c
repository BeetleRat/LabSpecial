// This code is not protected by any copyright as far as I know
// It is based on misc public sources on the Internet
// Prepared by Mark Polyak in 2020

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "string.h"

// #define SERVER_IP "192.168.56.1"
// #define EQUATION_SIZE 255
#define RESULT_SIZE 255
#define SOCKET_NOT_CONNECTED 0
#define END_CONNECTION_CODE "End of operation"

char* SERVER_IP;
int SERVER_PORT;
int equation_size = 255;
int result_size = 255;
int connect_socket;

char* read_string(int* string_size) {
    *string_size = 0;
    int capacity = 1;
    char* string = (char*)malloc(sizeof(char));
    char c = getchar();
    while (c != '\n') {
        string[(*string_size)++] = c;
        if ((*string_size) >= capacity) {
            capacity *= 2;
            string = (char*)realloc(string, capacity*sizeof(char));
        }
        c = getchar();
    }
    string[(*string_size)] = '\0';
    return string;
}

int socket_connect() {
    struct sockaddr_in connect_address;
    // Создаем сокет с протоколом по умолчанию
    connect_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect_socket < 0) {
        perror("socket");
        getchar();
        return 0;
    }

    connect_address.sin_family = AF_INET;
    connect_address.sin_port = htons(SERVER_PORT);
    connect_address.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (connect(
            connect_socket,
            (struct sockaddr *)&connect_address,
            sizeof(connect_address)) < 0) {
        perror("connect");
        getchar();
        return 0;
    }
    return 1;
}

void hand_input() {
    int equation_size = 0;
    char* equation;
    char result[RESULT_SIZE];

    while (1 == 1) {
        printf("Enter the equation: ");
        equation = read_string(&equation_size);
        if (equation[0] == 'x' || equation[0] == 'X') {
            strcpy(equation, END_CONNECTION_CODE);
            printf("equation: %s\n", equation);
            equation_size = sizeof(END_CONNECTION_CODE)-1;
            send(connect_socket, (char*)&equation_size, sizeof(int), 0);
            send(connect_socket, equation, equation_size, 0);
            break;
        }
        printf("equation: %s\n", equation);
        send(connect_socket, (char*)&equation_size, sizeof(int), 0);
        send(connect_socket, equation, equation_size, 0);
        sleep(1);
        recv(connect_socket, result, RESULT_SIZE, 0);
        printf("result: %s\n", result);
    }
}

char* generate_expression(int* string_size) {
    int min_lexemes = rand()%15;
    int dice;
    int bracket_count = 0;
    *string_size = 0;
    int capacity = 1;
    char* string = (char*)malloc(sizeof(char));

    if (rand()%2 == 0) {
        bracket_count++;
        string[(*string_size)++] = '(';
    }
    if ((*string_size) >= capacity) {
        capacity *= 2;
        string = (char*)realloc(string, capacity*sizeof(char));
    }
    dice = rand()%9+1;
    string[(*string_size)++] = dice+'0';
    while (min_lexemes > 0) {
        // printf("min_lexemes: %d\n", min_lexemes);
        // printf(" %s\n", string);
        if (string[(*string_size)-1] >= '0'
            && string[(*string_size)-1] <= '9') {
        dice = rand()%100;
        } else {
            dice = rand()%25;
            if (string[(*string_size)-1] == ')') {
                dice = dice+26;
            }
        }
        // printf("dice: %d\n", dice);
        if (dice <= 25) {
            if (string[(*string_size)-1]<'0'
                || string[(*string_size)-1] > '9') {
                    dice = rand()%9+1;
                } else {
                    dice = rand()%10;
                }
            string[(*string_size)++] = dice+'0';
        } else {
            if (dice <= 40) {
                string[(*string_size)++] = '+';
                min_lexemes--;
            } else {
                if (dice <= 55) {
                    string[(*string_size)++] = '-';
                    min_lexemes--;
                } else {
                    if (dice <= 70) {
                        string[(*string_size)++] = '/';
                        min_lexemes--;
                    } else {
                        if (dice <= 85) {
                            string[(*string_size)++] = '*';
                            min_lexemes--;
                        } else {
                            if (dice <= 95) {
                                if (string[(*string_size)-1] != '+'
                                && string[(*string_size)-1] != '-'
                                && string[(*string_size)-1] !='*'
                                && string[(*string_size)-1] != '/') {
                                    switch (rand()%4) {
                                        case 0:
                                            string[(*string_size)++] = '+';
                                            break;
                                        case 1:
                                            string[(*string_size)++] = '-';
                                            break;
                                        case 2:
                                            string[(*string_size)++] = '*';
                                            break;
                                        case 3:
                                            string[(*string_size)++] = '/';
                                            break;
                                    }
                                    if ((*string_size) >= capacity) {
                                        capacity *= 2;
                                        string = (char*)realloc(
                                                        string,
                                                        capacity*sizeof(char));
                                    }
                                }
                                string[(*string_size)++] = '(';
                                bracket_count++;
                                min_lexemes--;
                            }
                        }
                    }
                }
            }
            if (dice == 99) {
                string[(*string_size)++] = 'E';
            }
            if ((*string_size) >= capacity) {
                capacity *= 2;
                string = (char*)realloc(string, capacity*sizeof(char));
            }
        }
    }

    while (bracket_count > 0) {
        // printf("bracket_count: %d\n", bracket_count);
        // printf(" %s\n", string);
        if (string[(*string_size)-1] >= '0'
            && string[(*string_size)-1] <= '9') {
            dice = rand()%100;
        } else {
            dice = rand()%25;
            if (string[(*string_size)-1] == ')') {
                dice = dice+26;
            }
        }
        // printf("dice: %d\n", dice);
        if (dice <= 25) {
            if (string[(*string_size)-1]<'0'
            || string[(*string_size)-1] > '9') {
                dice = rand()%9+1;
            } else {
                dice = rand()%10;
            }
            string[(*string_size)++] = dice+'0';
        } else {
            if (dice <= 40) {
                string[(*string_size)++] = '+';
            } else {
                if (dice <= 55) {
                    string[(*string_size)++] = '-';
                } else {
                    if (dice <= 70) {
                        string[(*string_size)++] = '/';
                    } else {
                        if (dice <= 85) {
                            string[(*string_size)++] = '*';
                        } else {
                            if (dice > 85) {
                                string[(*string_size)++] = ')';
                                bracket_count--;
                            }
                        }
                    }
                }
            }
        }
        if ((*string_size) >= capacity) {
            capacity *= 2;
            string = (char*)realloc(string, capacity*sizeof(char));
        }
    }
    if (string[(*string_size)-1] == '+'
        || string[(*string_size)-1] == '-'
        || string[(*string_size)-1] == '*'
        || string[(*string_size)-1] == '/') {
        dice = rand()%9+1;
        string[(*string_size)++] = dice+'0';
        if ((*string_size) >= capacity) {
            capacity *= 2;
            string = (char*)realloc(string, capacity*sizeof(char));
        }
    }
    string[(*string_size)]='\0';
    return string;
}

void auto_input(int equations_count) {
    int equation_size;
    char* equation;
    char result[RESULT_SIZE];
    while (equations_count > 0) {
        equation_size = 0;
        equation = generate_expression(&equation_size);
        printf("equation: %s\n", equation);
        send(connect_socket, (char*)&equation_size, sizeof(int), 0);
        send(connect_socket, equation, equation_size, 0);
        sleep(1);
        recv(connect_socket, result, RESULT_SIZE, 0);
        printf("result: %s\n", result);
        equations_count--;
    }
    strcpy(equation, END_CONNECTION_CODE);
    printf("equation: %s\n", equation);
    equation_size = sizeof(END_CONNECTION_CODE)-1;
    send(connect_socket, (char*)&equation_size, sizeof(int), 0);
    send(connect_socket, equation, equation_size, 0);
}

int argv_to_int(char* arguments) {
    int arguments_int = 0;
    int i = 0;
    while (arguments[i] != NULL) {
        if ((int)arguments[i] < (int)'0' || (int)arguments[i] > (int)'9') {
            return 0;
        }
        arguments_int = (arguments_int+((int)arguments[i]-(int)'0'))*10;
        i++;
    }
    return arguments_int/10;
}
int main(int args, char *argv[]) {
    srand(time(0));
    // system("chcp 1251");


    if (args > 2) {
        SERVER_IP = argv[1];
        printf("SERVER_IP: %s\n", SERVER_IP);
        SERVER_PORT = argv_to_int(argv[2]);
        printf("SERVER_PORT: %d\n", SERVER_PORT);
    } else {
        SERVER_IP = "192.168.56.1";
        printf("SERVER_IP: %s\n", SERVER_IP);
        SERVER_PORT = 1;
        printf("SERVER_PORT: %d\n", SERVER_PORT);
    }
    if (socket_connect() == SOCKET_NOT_CONNECTED) {
        close(connect_socket);
        exit(1);
    }
    if (args > 3) {
        int equations_count = argv_to_int(argv[3]);
        switch (equations_count) {
            case 0:
                hand_input();
                break;
            default:
                auto_input(equations_count);
        }
    } else {
        hand_input();
    }
    close(connect_socket);

    return 0;
}
