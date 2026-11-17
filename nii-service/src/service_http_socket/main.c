#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "socket.h"
#define HTTP_SERVER_PORT 8080

int main(int argc, char** argv) {
  int client_sock = 0;
  struct timeval ts;
  struct timespec tsl;
  struct sockaddr_in name;
  socklen_t len_2 = 20;

  // ------------
  // аргумент командной строки - путь к папке с html
  if (argc != 2) {
    printf("Internal Error: Config error - no dir specified.\n");
    return -1;
  }
  if (chdir(argv[1]) < 0) {
    printf("Internal Error: Config error - couldn't chdir().\n");
    return -1;
  }

  // -------- socket
  client_sock =
      rx_open(HTTP_SERVER_PORT);  // создать сокет (socket, bind, listen)
  ts.tv_sec = 1;  // время максимального ожидания если сокет не готов
  ts.tv_usec = 0;
  tsl.tv_sec = 0;  // время задержки проверки входящих соединений
  tsl.tv_nsec = 50000;

  while (1) {
    fd_set fds;
    FD_ZERO(&fds);
    int maxfd = 0;

    if (client_sock != 0) {  // добавить сокет в проверку
      FD_SET(client_sock, &fds);
      maxfd = (client_sock > maxfd) ? client_sock : maxfd;
    }

    ts.tv_sec = 1;
    ts.tv_usec = 0;

    errno = 0;
    int nready = select(maxfd + 1, &fds, (fd_set*)0, (fd_set*)0,
                        &ts);  // проверка готовности сокета

    if (nready < 0) {
      fprintf(stderr, "Error: select nready %d, errno %d\n", nready, errno);
    } else if (nready == 0) {  // нет готовых сокетов
      nanosleep(&tsl, NULL);
    } else if (client_sock != 0 &&
               FD_ISSET(client_sock, &fds)) {  // сокет готов
      int n;
      if ((n = accept(client_sock, (struct sockaddr*)&name, &len_2)) <
          0) {  // ожидание соединения
        char tmp[200];
        sprintf(tmp, "accept call error %d", errno);
        printf("Error_accept %s\n", tmp);
        continue;
      }
      rx_packet(n);  // обработка входящего сообщения и отправка ответа в сокет
      // close(n);
    }
  }

  return 0;
}
