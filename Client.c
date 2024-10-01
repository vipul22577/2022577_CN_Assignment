#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8081
void *client_thread(void *arg) {
    int sock;
    struct sockaddr_in server_addr;
    char *message = "GET_CPU_INFO";
    char buffer[1024] = {0};
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return NULL;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "192.168.246.128", &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return NULL;
    }
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return NULL;
    }
    send(sock, message, strlen(message), 0);
    printf("Request sent\n");
    read(sock, buffer, sizeof(buffer));
    printf("Server response: %s\n", buffer);
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_connections>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);
    pthread_t threads[n];
    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, client_thread, NULL);
    }
    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
