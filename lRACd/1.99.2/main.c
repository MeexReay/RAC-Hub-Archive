#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int get_size_of_file(FILE *file);

int main(int argc, char **argv) {
    int sock;
    int listener;
    int opt_val = 1;
    struct sockaddr_in addr;
    int fork_result;
    char buffer[1024];
    int read_bytes;
    FILE *messages_file;
    int messages_file_size;
    char messages_file_size_str[1024];
    char *messages_from_file;
    char message_to_write[1024];
    time_t my_time;
    struct tm *now;
    char my_time_str[1024];
    char src_ip[INET_ADDRSTRLEN];
    struct sockaddr remote_addr;
    struct sockaddr_in *remote_addr_in;
    int remote_addrlen;
    int messages_len_from_client;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, SOMAXCONN);

    remote_addrlen = sizeof(struct sockaddr_in);
    while (1) {
        signal(SIGCHLD, SIG_IGN);
        sock = accept(listener, &remote_addr, &remote_addrlen);

        fork_result = fork();
        if (fork_result == 0) {
            read_bytes = recv(sock, buffer, 1024, 0);
            buffer[read_bytes] = '\0';
            remote_addr_in = (struct sockaddr_in *)&remote_addr;
            inet_ntop(AF_INET, &(remote_addr_in->sin_addr), src_ip, sizeof(src_ip));
            if (buffer[0] == 0x00) {
                messages_file = fopen("messages.txt", "rb");
                memset(messages_file_size_str, 0, 1024);
                messages_file_size = get_size_of_file(messages_file);
                sprintf(messages_file_size_str, "%d", messages_file_size);
                messages_file_size_str[strlen(messages_file_size_str)] = 0;
                send(sock, messages_file_size_str, 1024, 0);
                memset(buffer, 0, 1024);
                read_bytes = recv(sock, buffer, 1024, 0);
                buffer[read_bytes] = '\0';
                if (read_bytes == 0 || buffer[0] == 0x00) {
                    close(sock);
                } else if (buffer[0] == 0x01) {
                    messages_from_file = malloc(messages_file_size);
                    fread(messages_from_file, 1, messages_file_size, messages_file);
                    send(sock, messages_from_file, messages_file_size, 0);
                    free(messages_from_file);
                } else if (buffer[0] == 0x02) {
                    messages_len_from_client = atoi(&buffer[1]);
                    messages_from_file = malloc(messages_file_size);
                    fread(messages_from_file, 1, messages_file_size, messages_file);
                    messages_from_file[messages_file_size] = '\n';
                    send(sock, &messages_from_file[messages_len_from_client], messages_file_size - messages_len_from_client, 0);
                    free(messages_from_file);
                }
                fclose(messages_file);
            } else if (buffer[0] == 0x01) {
                buffer[read_bytes] = '\0';

                time(&my_time);
                now = localtime(&my_time);
                strftime(my_time_str, sizeof(my_time_str), "%Y-%m-%d %H:%M:%S", now);

                sprintf(message_to_write, "[%s] {%s} %s\n", my_time_str, src_ip, &buffer[1]);

                messages_file = fopen("messages.txt", "a");
                fwrite(message_to_write, 1, strlen(message_to_write), messages_file);
                fclose(messages_file);
            }
            memset(buffer, 0, 1024);
            close(sock);
            exit(0);
        }
        close(sock);
    }

    close(listener);
    return 0;
}

int get_size_of_file(FILE *f) {
    int size_of_file;

    fseek(f, 0, SEEK_END);
    size_of_file = ftell(f);
    fseek(f, 0, SEEK_SET);

    return size_of_file;
}
