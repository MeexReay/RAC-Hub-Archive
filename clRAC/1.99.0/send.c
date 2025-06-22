#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in addr;
    char message_to_server[1024];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    message_to_server[0] = 0x01;
    strcpy(&message_to_server[1], argv[3]);
    strcpy(&message_to_server[strlen(argv[3]) + 1], ": ");
    strcpy(&message_to_server[strlen(argv[3]) + 3], argv[4]);
    send(sock, message_to_server, strlen(&message_to_server[1]) + 1, 0);
    close(sock);

    return 0;
}
