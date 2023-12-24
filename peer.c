#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

struct ThreadArgs {
    int socket;
};

void *sendThread(void *args) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    int sendSocket = threadArgs->socket;

    while (1) {
        char message[BUFFER_SIZE];
        printf("Enter message to send: ");
        fgets(message, BUFFER_SIZE, stdin);
        send(sendSocket, message, strlen(message), 0);
    }

    pthread_exit(NULL);
}

void *receiveThread(void *args) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    int receiveSocket = threadArgs->socket;

    while (1) {
        char receivedMessage[BUFFER_SIZE];
        ssize_t bytesReceived = recv(receiveSocket, receivedMessage, sizeof(receivedMessage), 0);

        if (bytesReceived <= 0) {
            printf("Connection closed.\n");
            break;
        }

        receivedMessage[bytesReceived] = '\0';
        printf("Received message: %s", receivedMessage);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <SendingPort> <ReceivingPort>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sendingPort = atoi(argv[1]);
    int receivingPort = atoi(argv[2]);

    int sendSocket, receiveSocket;
    struct sockaddr_in sendAddr, receiveAddr;

    // Create sending socket
    if ((sendSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating sending socket");
        exit(EXIT_FAILURE);
    }

    memset(&sendAddr, 0, sizeof(sendAddr));
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_port = htons(sendingPort);
    sendAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for communication on the same machine

    // Create receiving socket
    if ((receiveSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating receiving socket");
        close(sendSocket);
        exit(EXIT_FAILURE);
    }

    memset(&receiveAddr, 0, sizeof(receiveAddr));
    receiveAddr.sin_family = AF_INET;
    receiveAddr.sin_port = htons(receivingPort);
    receiveAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for communication on the same machine

    // Bind the receiving socket
    if (bind(receiveSocket, (struct sockaddr *)&receiveAddr, sizeof(receiveAddr)) == -1) {
        perror("Error binding receiving socket");
        close(sendSocket);
        close(receiveSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections on the receiving socket
    if (listen(receiveSocket, 1) == -1) {
        perror("Error listening for connections on receiving socket");
        close(sendSocket);
        close(receiveSocket);
        exit(EXIT_FAILURE);
    }

    // Connect to the receiving socket
    if (connect(sendSocket, (struct sockaddr *)&receiveAddr, sizeof(receiveAddr)) == -1) {
        perror("Error connecting sending socket");
        close(sendSocket);
        close(receiveSocket);
        exit(EXIT_FAILURE);
    }

    // Accept the incoming connection on the receiving socket
    int receiveConnection = accept(receiveSocket, NULL, NULL);
    if (receiveConnection == -1) {
        perror("Error accepting connection on receiving socket");
        close(sendSocket);
        close(receiveSocket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the other end.\n");

    // Create thread arguments
    struct ThreadArgs sendThreadArgs = { .socket = sendSocket };
    struct ThreadArgs receiveThreadArgs = { .socket = receiveConnection };

    // Create threads for sending and receiving
    pthread_t sendThreadId, receiveThreadId;
    if (pthread_create(&sendThreadId, NULL, sendThread, (void *)&sendThreadArgs) != 0 ||
        pthread_create(&receiveThreadId, NULL, receiveThread, (void *)&receiveThreadArgs) != 0) {
        perror("Error creating threads");
        close(sendSocket);
        close(receiveSocket);
        close(receiveConnection);
        exit(EXIT_FAILURE);
    }

    // Wait for threads to finish
    pthread_join(sendThreadId, NULL);
    pthread_join(receiveThreadId, NULL);

    // Close sockets
    close(sendSocket);
    close(receiveSocket);
    close(receiveConnection);

    return 0;
}
