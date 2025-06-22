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

    if (argc < 5) {
        printf("Usage: /path/to/clrac-send server_ip server_port user_name message\nExample: ./clrac-messages 1.2.3.4 42666 user message\n");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    message_to_server[0] = 0x01;
    strcpy(&message_to_server[1], "<");
    strcpy(&message_to_server[2], argv[3]);
    strcpy(&message_to_server[strlen(argv[3]) + 2], "> ");
    strcpy(&message_to_server[strlen(argv[3]) + 4], argv[4]);
    send(sock, message_to_server, strlen(&message_to_server[1]) + 1, 0);
    close(sock);

    return 0;
}
