#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
// #include <dirent.h>
#include <sys/stat.h>
// #include <netinet/in.h>
// #include <errno.h>
// #include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 1024
#define SERVER_NAME "pauk_httpd"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define MAXERR 1000

void send_headers(int status, char* title, char* extra_header, char* mime_type,
                  off_t length, time_t mod, int sock) {
  time_t now;
  char timebuf[100];
  char line[MAXERR];
  char* ptr = line;

  sprintf(line, "%s %d %s\015\012Server: %s\015\012", PROTOCOL, status, title,
          SERVER_NAME);    // HTTP/1.0 200 ok - pauk_http
  now = time((time_t*)0);  // получить время
  strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
  ptr = line + strlen(line);
  sprintf(ptr, "Date: %s\015\012", timebuf);  // дата

  if (extra_header != (char*)0) {
    ptr = line + strlen(line);
    sprintf(ptr, "%s\015\012", extra_header);
  }
  if (mime_type != (char*)0) {  // Content-Type: text/html
    ptr = line + strlen(line);
    sprintf(ptr, "Content-Type: %s\015\012", mime_type);
  }
  if (length >= 0) {  // -1
    ptr = line + strlen(line);
    sprintf(ptr, "Content-Length: %lld\015\012", (long long int)length);
  }
  if (mod != (time_t)-1) {  // -1
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&mod));
    ptr = line + strlen(line);
    sprintf(ptr, "Last-Modified: %s\015\012", timebuf);
  }
  ptr = line + strlen(line);
  sprintf(ptr, "Connection: close\015\012\015\012");  // Connection: close

  write(sock, line, strlen(line));
}

void send_error(int status, char* title, char* extra_header, char* text,
                int sock) {  // вывод ошибки в браузер
  char line[MAXERR];

  send_headers(status, title, extra_header, "text/html", -1, -1, sock);
  sprintf(line,
          "<!DOCTYPE html><html><head><title>%d %s</title></head>\n<body "
          "bgcolor=\"#cc9999\"><h4>%d %s</h4>\n%s\n<br/></body></html>\n",
          status, title, status, title, text);
  write(sock, line, strlen(line));
}

static unsigned char buf[BUFSIZE + 1];
void send_file(const char* fname, const char* type,
               int sock) {  // вывод файла .html
                            // check if the file is installed and get the length
  struct stat s;

  if (stat(fname, &s) == 0) {  // вайл есть
    int size = (int)s.st_size;
    if (size == 0)  // не пустой
      return;

    // open the file
    int fd = open(fname, O_RDONLY);  // открыть
    if (fd == -1) {
      return;
    }

    // send the file
    send_headers(200, "Ok", (char*)0, (char*)type, size, -1,
                 sock);  // заголоок http

    int len;
    while ((len = read(fd, buf, BUFSIZE)) > 0) {  // чтение из файла
      int v = write(sock, buf, len);  // запись на выход
      (void)v;  // освободить место в памяти
    }
    close(fd);
  } else
    send_error(404, "Not Found", (char*)0, "Bad filename.", sock);  // файла нет
}
