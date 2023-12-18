// peer.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MESSAGE_SIZE 1024

void receive_messages(int socket_fd) {
    char buffer[MAX_MESSAGE_SIZE];
    while (1) {
        ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            perror("recv");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);
    }
}

void send_message(int socket_fd) {
    char message[MAX_MESSAGE_SIZE];
    while (1) {
        printf("Enter a message (or 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);
        if (strcmp(message, "exit\n") == 0) {
            break;
        }
        send(socket_fd, message, strlen(message), 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_addr_len = sizeof(client_address);

    // Creating socket file descriptor
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // Binding the socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_socket, 1) == -1) {
        perror("listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for a connection on port %d...\n", port);

    // Accepting a connection
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_addr_len)) == -1) {
        perror("accept failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connection established with peer.\n");

    // Create two threads for sending and receiving messages
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        send_message(client_socket);
    } else if (pid > 0) {
        // Parent process
        receive_messages(client_socket);
    } else {
        perror("fork failed");
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Close sockets
    close(client_socket);
    close(server_socket);

    return 0;
}
