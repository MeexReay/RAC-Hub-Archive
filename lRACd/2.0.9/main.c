#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int get_size_of_file(FILE *f);

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
    int messages_len_from_client;
    int i;
    char name[1024];
    char password[1024];
    int write_to_name = 1;
    char path_to_user_directory[1024];
    char path_to_password_file[1024];
    FILE *password_file;
    int stat_result;
    struct stat stats;
    char data_0x01 = 0x01;
    char data_0x02 = 0x02;
    char message[1024];
    char password_from_file[1024];
    int password_file_size;
#ifdef IP
    struct sockaddr_in src_ip;
    socklen_t src_ip_len;
#endif

    if (argc < 3) {
        printf("Usage: /path/to/lracd bind_ip bind_port\nExample: ./lracd 0.0.0.0 42666\n");
        return 0;
    }

    listener = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, SOMAXCONN);

    while (1) {
        signal(SIGCHLD, SIG_IGN);
#ifdef IP
        sock = accept(listener, (struct sockaddr *)&src_ip, &src_ip_len);
#else
        sock = accept(listener, NULL, NULL);
#endif

        fork_result = fork();
        if (fork_result == 0) {
            read_bytes = recv(sock, buffer, 1024, 0);
            buffer[read_bytes] = '\0';
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
                if (buffer[0] == 0x00) {
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
                    send(sock, &messages_from_file[messages_len_from_client], messages_file_size - messages_len_from_client, 0);
                    free(messages_from_file);
                }
                fclose(messages_file);
            } else if (buffer[0] == 0x01) {
                time(&my_time);
                now = localtime(&my_time);
                strftime(my_time_str, sizeof(my_time_str), "%Y-%m-%d %H:%M:%S", now);

#ifdef IP
                sprintf(message_to_write, "[%s] {%s} (UNAUTHENTICATED) %s\n", my_time_str, inet_ntoa(src_ip.sin_addr), &buffer[1]);
#else
                sprintf(message_to_write, "[%s] (UNAUTHENTICATED) %s\n", my_time_str, &buffer[1]);
#endif

                messages_file = fopen("messages.txt", "a");
                fwrite(message_to_write, 1, strlen(message_to_write), messages_file);
                fclose(messages_file);
            } else if (buffer[0] == 0x02) {
                for (i = 1; i <= strlen(&buffer[1]); ++i) {
                    if (buffer[i] != '\n') {
                        if (write_to_name == 1) {
                            name[i - 1] = buffer[i];
                        } else if (write_to_name == 0) {
                            password[i - 2 - strlen(name)] = buffer[i];
                        } else if (write_to_name == 2) {
                            message[i - 3 - strlen(password) - strlen(name)] = buffer[i];
                        }
                    } else {
                        if (write_to_name == 1) {
                            name[i - 1] = '\0';
                            write_to_name = 0;
                        } else if (write_to_name == 0) {
                            password[i - 2 - strlen(name)] = '\0';
                            write_to_name = 2;
                        }
                    }
                }
                message[i - 3 - strlen(password) - strlen(name)] = '\0';

                sprintf(path_to_user_directory, "users/%s", name);
                stat_result = stat(path_to_user_directory, &stats);
                if (!(stat_result == 0 && S_ISDIR(stats.st_mode))) {
                    send(sock, &data_0x01, 1, 0);
                } else {
                    sprintf(path_to_password_file, "%s/password.txt", path_to_user_directory);
                    password_file = fopen(path_to_password_file, "rb");
                    password_file_size = get_size_of_file(password_file);
                    fread(password_from_file, 1, password_file_size, password_file);
                    fclose(password_file);
                    password_from_file[password_file_size] = '\0';
                    if (strcmp(password, password_from_file) != 0) {
                        send(sock, &data_0x02, 1, 0);
                    } else {
                        time(&my_time);
                        now = localtime(&my_time);
                        strftime(my_time_str, sizeof(my_time_str), "%Y-%m-%d %H:%M:%S", now);
#ifdef IP
                        sprintf(message_to_write, "[%s] {%s} <%s> %s\n", my_time_str, inet_ntoa(src_ip.sin_addr), name, message);
#else
                        sprintf(message_to_write, "[%s] <%s> %s\n", my_time_str, name, message);
#endif
                        messages_file = fopen("messages.txt", "a");
                        fwrite(message_to_write, 1, strlen(message_to_write), messages_file);
                        fclose(messages_file);
                    }
                }
            } else if (buffer[0] == 0x03) {
                for (i = 1; i <= strlen(&buffer[1]); ++i) {
                    if (buffer[i] != '\n') {
                        if (write_to_name == 1) {
                            name[i - 1] = buffer[i];
                        } else {
                            password[i - 2 - strlen(name)] = buffer[i];
                        }
                    } else {
                        name[i - 1] = '\0';
                        write_to_name = 0;
                    }
                }
                password[i - 2 - strlen(name)] = '\0';

                sprintf(path_to_user_directory, "users/%s", name);
                stat_result = stat(path_to_user_directory, &stats);
                if (stat_result == 0 && S_ISDIR(stats.st_mode)) {
                    send(sock, &data_0x01, 1, 0);
                } else {
                    mkdir(path_to_user_directory, 0777 & ~umask(umask(0)));

                    sprintf(path_to_password_file, "%s/password.txt", path_to_user_directory);
                    password_file = fopen(path_to_password_file, "w");
                    fwrite(password, 1, strlen(password), password_file);
                    fclose(password_file);
                }
            }
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
