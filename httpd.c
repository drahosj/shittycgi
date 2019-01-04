#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char ** argv)
{
      uint16_t port = 8080;
      if (argv[1]) {
            port = atoi(argv[1]);
      }

      int listensock = socket(AF_INET, SOCK_STREAM, 0);
      if (listensock < 0) {
            perror("socket");
            return -1;
      }

      struct sockaddr_in sa;
      memset(&sa, 0, sizeof(sa));

      sa.sin_family = AF_INET;
      inet_aton("0.0.0.0", &sa.sin_addr);
      sa.sin_port = htons(port);

      if (bind(listensock, (struct sockaddr *) &sa, sizeof(sa))) { 
            perror("bind");
            return -1;
      }

      if (listen(listensock, 8)) {
            perror("listen");
            return -1;
      }

      while (1) {
            struct sockaddr_in sa_conn;
            socklen_t ssize;
            int sock_conn = accept(
                        listensock, 
                        (struct sockaddr *) &sa_conn, 
                        &ssize);

            if (sock_conn < 0) {
                  perror("accept");
            }

            pid_t pid = fork();
            if (pid) {
                  close(sock_conn);
            } else {
                  dup2(sock_conn, STDOUT_FILENO);

                  static char buf[512];
                  ssize_t len;
                  char cgi[128] = "";
                  char * cgi_argv[3] = {NULL, NULL, NULL};
                  static char buf_last[1024];
                  size_t bllen = 0;
                  do {
                        len = read(sock_conn, buf, 512);
                        if (len < 0) {
                              perror("read");
                              return -1;
                        }

                        if (bllen >= 128) {
                              memmove(buf_last, buf_last + bllen - 128, 128);
                              bllen = 128;
                        }

                        memcpy(buf_last + bllen, buf, len);
                        bllen += len;

                        if (strstr(buf_last, "\r\n\r\n")) {
                              len = 0;
                        }

                        if (!cgi[0]) {
                              strtok(buf, " ");

                              strncpy(cgi, strtok(NULL, " "), 128);
                        }

                  } while (len);

                  cgi_argv[0] = strtok(cgi + 1, "?");
                  cgi_argv[1] = strtok(NULL, "?");

                  if (cgi_argv[1]) {
                        setenv("QUERY_STRING", cgi_argv[1], 1);
                  } else {
                        setenv("QUERY_STRING", "", 1);
                  }
                  execv(cgi + 1, cgi_argv);
            }
      }

      return 0;
}
