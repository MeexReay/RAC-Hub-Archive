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
    char buffer[1024];
    int read_bytes;

    if (argc < 5) {
        printf("Usage: /path/to/clrac-register server_ip server_port user_name user_password\nExample: ./clrac-messages 1.2.3.4 42666 user password\n");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    message_to_server[0] = 0x03;
    strcpy(&message_to_server[1], argv[3]);
    message_to_server[strlen(argv[3]) + 1] = '\n';
    strcpy(&message_to_server[strlen(argv[3]) + 2], argv[4]);
    send(sock, message_to_server, strlen(&message_to_server[1]) + 1, 0);
    read_bytes = recv(sock, buffer, 1024, 0);
    buffer[read_bytes] = '\0';
    if (buffer[0] == 0x01) {
        printf("This user already exists!\n");
    }
    close(sock);

    return 0;
}
