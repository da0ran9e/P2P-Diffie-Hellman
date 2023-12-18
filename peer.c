#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int recvPort;  // Moved declaration here
char message[BUFFER_SIZE];  // Moved declaration here

void connectPort(int destination);
void send_message(int peerSock, char *buffer);
void receiving(int server_fd);
void *receive_thread(void *server_fd);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int PORT = atoi(argv[1]);

    int server_fd, new_socket;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    pthread_t tid;
    pthread_create(&tid, NULL, &receive_thread, &server_fd);

    printf("Enter a port to communicate with: ");
    scanf("%d", &recvPort);
    connectPort(recvPort);

    while (1) {
        printf("Enter a message (or 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);

        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        send_message(recvPort, message);

        if (strcmp(message, "exit") == 0) {
            break;
        }
    }

    close(server_fd);
    pthread_join(tid, NULL);

    return 0;
}

void connectPort(int destination) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(destination);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\nConnection Failed \n");
        exit(EXIT_FAILURE);
    } else {
        printf("\nConnected !\n");
    }
}

void send_message(int peerSock, char *buffer) {
    send(peerSock, buffer, strlen(buffer), 0);
    printf("\nMessage sent\n");
}

void *receive_thread(void *server_fd) {
    int s_fd = *((int *)server_fd);

    while (1) {
        sleep(2);
        receiving(s_fd);
    }
}

void receiving(int server_fd) {
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);

    while (1) {
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {
                if (i == server_fd) {
                    int client_socket;

                    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                                (socklen_t *)&addrlen)) < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &current_sockets);
                } else {
                    int valread = recv(i, buffer, sizeof(buffer), 0);
                    if (valread <= 0) {
                        // Connection closed or error
                        printf("Connection closed by peer or an error occurred.\n");
                        close(i);
                        FD_CLR(i, &current_sockets);
                    } else {
                        printf("\nReceived: %s\n", buffer);
                    }
                }
            }
        }
    }
}
