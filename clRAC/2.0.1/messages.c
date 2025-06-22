#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int recv_all(int sock, char *buffer, int len, int flags);

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in addr;
    char data_0x00 = 0x00;
    char data_0x01 = 0x01;
    char buffer[1024];
    char *messages;
    int read_bytes;
    int first_try_to_get_messages = 1;
    int cached_messages_len;
    int messages_len_from_server;
    int update_time;
    char cached_messages_len_str_to_server[1024];

    if (argc < 4) {
        printf("Usage: /path/to/clrac-messages server_ip server_port update_time_in_seconds\nExample: ./clrac-messages 1.2.3.4 42666 1\n");
        return 0;
    }

    update_time = atoi(argv[3]);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    
        send(sock, &data_0x00, 1, 0);
        read_bytes = recv(sock, buffer, 1024, 0);
        buffer[read_bytes] = '\0';
        messages_len_from_server = atoi(buffer);
    
        if (first_try_to_get_messages == 1) {
            first_try_to_get_messages = 0;
            cached_messages_len = messages_len_from_server;
            memset(buffer, 0, 1024);
            send(sock, &data_0x01, 1, 0);
            messages = malloc(messages_len_from_server + 1);
            read_bytes = recv_all(sock, messages, messages_len_from_server, 0);
            messages[read_bytes] = '\0';
            system("clear");
            printf("%s", messages);
            free(messages);
            close(sock);
        } else {
            if (messages_len_from_server == cached_messages_len) {
                close(sock);
            } else {
                memset(buffer, 0, 1024);
                memset(cached_messages_len_str_to_server, 0, 1024);
                cached_messages_len_str_to_server[0] = 0x02;
                sprintf(&cached_messages_len_str_to_server[1], "%d", cached_messages_len);
                send(sock, cached_messages_len_str_to_server, strlen(&cached_messages_len_str_to_server[1]) + 1, 0);
                messages = malloc(messages_len_from_server - cached_messages_len + 1);
                read_bytes = recv_all(sock, messages, messages_len_from_server - cached_messages_len, 0);
                messages[read_bytes] = '\0';
                printf("%s", messages);
                cached_messages_len = messages_len_from_server;
                free(messages);
                close(sock);
            }
        }
        sleep(update_time);
    }

    return 0;
}

int recv_all(int sock, char *buffer, int len, int flags) {
    int total = 0;
    int n;
    while(total < len) {
        n = recv(sock, buffer + total, len - total, flags);
        if(n == -1) {
            break;
        }
        total += n;
    }
    return (n == -1 ? -1 : total);
}
