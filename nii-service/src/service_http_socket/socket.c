#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "send.h"

#define KB 1024
#define MAXDBG (10 * KB)
#define DEBUG_ON
#define DEBUG_FILE "../../data/log"
#define USER_IP_FILE "../../data/user_ip"
#define USER_IP_FILE_TMP "../../data/user_ip.tmp"
#define LOGIN_FILE "./login.html"
#define STYLE_FILE "./style.css"
#define CLI "../../build/s21_cli.app"
#define DATA_DB "data_db"
#define USER_IP "user_ip"

#define MAXIN (10 * KB)
#define MAXBUF KB
#define MAXCLI (10 * KB * KB)

static int hex_to_int(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static void url_decode(char* str) {
  char* src = str;
  char* dst = str;

  while (src && *src) {
    if (*src == '%') {
      if (src[1] && src[2]) {
        int high = hex_to_int(src[1]);
        int low = hex_to_int(src[2]);

        if (high != -1 && low != -1) {
          *dst++ = (char)((high << 4) | low);
          src += 3;
          continue;
        }
      }
    } else if (*src == '+') {
      *dst++ = ' ';
      src++;
      continue;
    }
    *dst++ = *src++;
  }
  if (dst) *dst = '\0';
}

int rx_open(uint16_t port) {
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf("Error = Socket_1");
    return 0;
  }

  int on = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) <
      0) {  // SO_REUSEADDR = 1 (разрешить повторное подключение к сокету)
    printf("Error = Socket_2");
    close(sock);
    return 0;
  }

  struct sockaddr_in s;
  s.sin_family = AF_INET;
  s.sin_port = htons(port);  // порт 80
  s.sin_addr.s_addr = htonl(INADDR_ANY);  // подключение с любого адреса
  // s.sin_addr.s_addr = htonl(INADDR_LOOPBACK/*0xc0a8e043*/);

  if (bind(sock, (struct sockaddr*)&s, sizeof(s)) < 0) {
    perror("binding DNS socket");  // it could fail if the port is in use by
                                   // somebody else
    close(sock);
    return 0;
  }

  if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {  // не блокировать дискриптор
    printf("Error = Socket_3");
    close(sock);
    return 0;
  }

  if (listen(sock, 10) < 0) {
    fprintf(stderr, "Error: cannot listen on communication socket\n");
  }

  return sock;
}

int rx_packet(int sock) {
  char line[MAXBUF] = {0}, method[MAXBUF] = {0}, path[MAXBUF] = {0},
       protocol[MAXBUF] = {0};
  int content_len = 0;
  char* file = NULL;

  size_t len;

  if (!sock) {
    printf("Not socket\n");
    return 0;
  }
  FILE* fp = fdopen(sock, "r");
  if (!fp) {
    close(sock);
    printf("Invalid open sock\n");
    return 0;
  }

  // --------- Обработка заголовка -------------
  // первая строка http заголовка - получить метод имя файла html и протокол
  // (GET /index.html HTTP/1.1)
  if (fgets(line, sizeof(line), fp) == (char*)0) {
    // printf("Bad Request: No request found\n");
    return 0;
  }
  if (sscanf(line, "%999[^ ] %999[^ ] %999[^ ]", method, path, protocol) != 3) {
    send_error(400, "Bad Request", (char*)0, "Can't parse request.", sock);
    fclose(fp);
    return 0;
  }

  // пятая строка http заголовка получаем размер контента ( Content-Length: 185
  // )
  while (fgets(line, sizeof(line), fp) != (char*)0) {
    if (strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0 ||
        strcmp(line, "\0") == 0 || strcmp(line, "\r") == 0) {
      break;
    }
    char type[MAXBUF], value[MAXBUF];
    if (sscanf(line, "%999[^ ] %999[^ ]", type, value) == 2) {
      if (strcasecmp(type, "Content-Length:") == 0) content_len = atoi(value);
    }
  }

  // ------------ method
  int get = 0;  // анализируем метод
  if (strcasecmp(method, "post") == 0)
    get = 0;  // сравниваем без учета регистра
  else if (strcasecmp(method, "get") == 0)
    get = 1;
  else {
    printf("Not Implemented: That method is not implemented\n");
    send_error(501, "Not Implemented", (char*)0,
               "That method is not implemented.", sock);
    fclose(fp);
    return 0;
  }
  char* p = NULL;
  if (get) {  // не принемаем данных методом GET
    p = strstr(path, "?");
    if (p != NULL) *p = '\0';
  }

  // ---------- file
  if (path[0] != '/') {  // анализируем путь к файлу html
    send_error(400, "Bad Request", (char*)0, "Bad filename.", sock);
    fclose(fp);
    return 0;
  }
  file = &(path[1]);
  if (file[0] == '\0')
    file = "login.html";  // если нет пути - то стандартная страница
  len = strlen(file);
  if (file[0] == '/' || strcmp(file, "..") == 0 ||
      strncmp(file, "../", 3) == 0 || strstr(file, "/../") != (char*)0 ||
      strcmp(&(file[len - 3]), "/..") == 0) {
    send_error(400, "Bad Request", (char*)0, "Illegal filename.", sock);
    fclose(fp);
    return 0;
  }

  /* extract sender ip address */
  struct sockaddr_in from;
  int fromlen = sizeof(from);
  if (getpeername(sock, (struct sockaddr*)&from, (socklen_t*)&fromlen) < 0) {
    send_error(404, "Not Found", (char*)0, "Internal error.", sock);
    fclose(fp);
    return 0;
  }
  if (from.sin_family != AF_INET) {
    send_error(404, "Not Found", (char*)0, "Internal error.", sock);
    fclose(fp);
    return 0;
  }
  uint32_t ip = ntohl(from.sin_addr.s_addr);

  char surrent_ip[MAXBUF] = {0};
  snprintf(surrent_ip, MAXBUF - 1, "%x", ip);

  /* Check ip in user_ip files */
  int authorization = 0;
  char *ptf_auth, *login = " ", *password = " ";
  const char* ptr_end_part;
  char str_ip_file[MAXBUF] = {0};

#ifdef DEBUG_ON
  int fd_debug = open(DEBUG_FILE, O_RDWR | O_APPEND | O_CREAT,
                      S_IRWXU | S_IRWXG | S_IRWXO);
  if (lseek(fd_debug, 0, 2) > MAXDBG) {
    close(fd_debug);
    fd_debug = open(DEBUG_FILE, O_RDWR | O_TRUNC | O_CREAT,
                    S_IRWXU | S_IRWXG | S_IRWXO);
  }
#endif

#ifdef DEBUG_ON
  time_t now = time(NULL);  // текущее время как time_t
  struct tm* local =
      localtime(&now);  // преобразуем в локальное время (struct tm)
  char date_time[64];
  strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", local);
  write(fd_debug, date_time, strlen(date_time));

  write(fd_debug, " ", 1);
  write(fd_debug, file, strlen(file));
  write(fd_debug, " ", 1);
  write(fd_debug, method, strlen(method));
  write(fd_debug, " ", 1);
  write(fd_debug, surrent_ip, strlen(surrent_ip));
  write(fd_debug, " ", 1);
#endif
  /* Logout */
  if (!strcmp(file, "logout.html")) {
    FILE* file = fopen(USER_IP_FILE, "r");
    FILE* temp = fopen(USER_IP_FILE_TMP, "w");
    char buffer[MAXBUF] = {0};
    while (fgets(buffer, MAXBUF, file) != NULL) {
      if (strstr(buffer, surrent_ip) == NULL) {
        fputs(buffer, temp);
      }
    }
    fclose(file);
    fclose(temp);

    file = fopen(USER_IP_FILE, "w");
    temp = fopen(USER_IP_FILE_TMP, "r");
    while (fgets(buffer, MAXBUF, temp) != NULL) {
      fputs(buffer, file);
    }
    fclose(file);
    fclose(temp);
#ifdef DEBUG_ON
    write(fd_debug, "\n", 1);
    close(fd_debug);
#endif
    send_headers(302, "Found", "Location: /login.html", "text/html", -1, -1,
                 sock);
    fclose(fp);
    return 0;
  }

  /* Get password from ip */
  int fb_user_ip = open(USER_IP_FILE, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fb_user_ip == -1) {
    send_error(400, "Bad Request", (char*)0, "Bad file user_ip.", sock);
    fclose(fp);
#ifdef DEBUG_ON
    close(fd_debug);
#endif
    return 0;
  }

  read(fb_user_ip, str_ip_file, sizeof(str_ip_file));
  close(fb_user_ip);
  if ((ptf_auth = strstr(str_ip_file, surrent_ip)) != NULL) {
    authorization = 1;
    /* Get login and password in user_ip file */
    if ((login = strstr(ptf_auth, "login=")) != NULL)
      login += strlen("login=");
    else {
      send_error(400, "Bad Request", (char*)0, "Bad user name in file user_ip.",
                 sock);
      fclose(fp);
#ifdef DEBUG_ON
      close(fd_debug);
#endif
      return 0;
    }
    if ((password = strstr(ptf_auth, "password=")) != NULL)
      password += strlen("password=");
    else {
      send_error(400, "Bad Request", (char*)0, "Bad password in file user_ip.",
                 sock);
      fclose(fp);
#ifdef DEBUG_ON
    close(fd_debug);
#endif
      return 0;
    }
    ptr_end_part = strtok(ptf_auth, " \n");
    if (ptr_end_part != NULL)
      while (strtok(NULL, " \n") != NULL);
  }

  // ------------ get
  if (get) {
#ifdef DEBUG_ON
    write(fd_debug, "\n", 1);
    close(fd_debug);
#endif
    if (authorization == 0) {
      if (strstr(file, ".html") != 0)
        send_file(LOGIN_FILE, "text/html", sock);
      else if (strstr(file, ".css") != 0)
        send_file(file, "text/css", sock);
      else if (strstr(file, ".js") != 0)
        send_file(file, "application/x-javascript", sock);
      fclose(fp);
      return 0;
    }
    if (strstr(file, ".html") != 0)
      send_file(file, "text/html", sock);
    else if (strstr(file, ".css") != 0)
      send_file(file, "text/css", sock);
    else if (strstr(file, ".js") != 0)
      send_file(file, "application/x-javascript", sock);
    else {
      send_error(404, "Not Found", (char*)0, "Bad filename.", sock);
      fclose(fp);
      return 0;
    }
    fclose(fp);
    return 0;
  }

  // ----------- POST - run command and return file ---------------
  char web_command[520] = {0};
  char *command, *arg1, *arg2, *arg3, *arg4, *app, *app_arg;

  /* Get data */
  if ((content_len + 1) > MAXBUF) {
    send_error(400, "Bad Request", (char*)0, "Invalid rquest command.", sock);
    fclose(fp);
#ifdef DEBUG_ON
    close(fd_debug);
#endif
    return 0;
  }
  fgets(line, content_len + 1, fp);

  /* Check availability login */
  char *login_post, *password_post;
  if ((login_post = strstr(line, "login=")) != NULL)
    login_post += strlen("login=");

  if ((password_post = strstr(line, "password=")) != NULL)
    password_post += strlen("password=");

  if (login_post != NULL) {
    if (strlen(login_post) != 0) {
      login = login_post;
      if (password_post != NULL)
        password = password_post;
      else {
        send_error(404, "Not Found", (char*)0, "Bad password.", sock);
        fclose(fp);
#ifdef DEBUG_ON
        close(fd_debug);
#endif
        return 0;
      }
    } else {
      send_file(LOGIN_FILE, "text/html", sock);
      fclose(fp);
#ifdef DEBUG_ON
      close(fd_debug);
#endif
      return 0;
    }
  } else if (authorization == 0) {
    send_file(LOGIN_FILE, "text/html", sock);
    fclose(fp);
#ifdef DEBUG_ON
    close(fd_debug);
#endif
    return 0;
  }
  /* Form command and arguments */
  if ((command = strstr(line, "command=")) != NULL)
    command += strlen("command=");
  if ((arg1 = strstr(line, "arg1=")) != NULL) arg1 += strlen("arg1=");
  if ((arg2 = strstr(line, "arg2=")) != NULL) arg2 += strlen("arg2=");
  if ((arg3 = strstr(line, "arg3=")) != NULL) arg3 += strlen("arg3=");
  if ((arg4 = strstr(line, "arg4=")) != NULL) arg4 += strlen("arg4=");

  if ((app = strstr(line, "app=")) != NULL) app += strlen("app=");
  if ((app_arg = strstr(line, "app_arg=")) != NULL)
    app_arg += strlen("app_arg=");
  if (app_arg) {
    if ((strstr(app_arg, DATA_DB) != NULL) ||
        (strstr(app_arg, USER_IP) != NULL)) {
      send_error(404, "Not Found", (char*)0, "There is no access", sock);
      fclose(fp);
#ifdef DEBUG_ON
      close(fd_debug);
#endif
      return 0;
    }
  }

  /* End str instead & */
  ptr_end_part = strtok(line, "&");
  if (ptr_end_part != NULL)
    while (strtok(NULL, "&") != NULL);

  url_decode(app);
  url_decode(app_arg);
  url_decode(login);
  url_decode(password);

  /* Form command for run */
  if (command != NULL) {
    url_decode(command);
    strncat(web_command, command, 100);
    strncat(web_command, " ", 2);
  }
  if (arg1 != NULL) {
    url_decode(arg1);
    strncat(web_command, arg1, 100);
    strncat(web_command, " ", 2);
  }
  if (arg2 != NULL) {
    url_decode(arg2);
    strncat(web_command, arg2, 100);
    strncat(web_command, " ", 2);
  }
  if (arg3 != NULL) {
    url_decode(arg3);
    strncat(web_command, arg3, 100);
    strncat(web_command, " ", 2);
  }
  if (arg4 != NULL) {
    url_decode(arg4);
    strncat(web_command, arg4, 100);
    strncat(web_command, " ", 2);
  }
  strncat(web_command, "\n", 2);
#ifdef DEBUG_ON
  write(fd_debug, command, strlen(command));
  write(fd_debug, "\n", 1);
  close(fd_debug);
#endif
  /* Create child */
  int pfd_child_in[2];
  int pfd_child_out[2];
  if (pipe2(pfd_child_in, O_CLOEXEC) == -1) {
    send_error(404, "Not Found", (char*)0, "Bad pipe.", sock);
    fclose(fp);
    return 0;
  }
  if (pipe2(pfd_child_out, O_CLOEXEC) == -1) {
    send_error(404, "Not Found", (char*)0, "Bad pipe.", sock);
    fclose(fp);
    return 0;
  }
  pid_t childPid = fork();
  if (childPid == -1) {
    close(pfd_child_in[0]);
    close(pfd_child_in[1]);
    close(pfd_child_out[0]);
    close(pfd_child_out[1]);
    send_error(404, "Not Found", (char*)0, "Bad fork.", sock);
    fclose(fp);
    return 0;
  }
  if (childPid == 0) {
    close(sock);
    close(pfd_child_out[0]);
    close(pfd_child_in[1]);
    if (pfd_child_in[0] != STDIN_FILENO) {
      if (dup2(pfd_child_in[0], STDIN_FILENO) == -1) {
        close(pfd_child_in[0]);
        send_error(404, "Not Found", (char*)0, "Bad dup child.", sock);
        fclose(fp);
        return 0;
      }
      close(pfd_child_in[0]);
    }
    if (pfd_child_out[1] != STDOUT_FILENO) {
      if (dup2(pfd_child_out[1], STDOUT_FILENO) == -1) {
        close(pfd_child_out[1]);
        send_error(404, "Not Found", (char*)0, "Bad dup child.", sock);
        fclose(fp);
        return 0;
      }
      close(pfd_child_out[1]);
    }
    execlp(app, app, app_arg, NULL);
    printf("Error cli!!!\n");
    exit(EXIT_FAILURE);
  }

  /* Parent */
  close(pfd_child_out[1]);
  close(pfd_child_in[0]);

  /* Enter command */
  write(pfd_child_in[1], login, strlen(login));
  write(pfd_child_in[1], "\n", strlen("\n"));
  write(pfd_child_in[1], password, strlen(password));
  write(pfd_child_in[1], "\n", strlen("\n"));
  write(pfd_child_in[1], web_command, strlen(web_command));

  close(pfd_child_in[1]);

  /* get answer */
  // char buf_cli_mas[MAXCLI] = {0};
  // char* buf_cli = buf_cli_mas;
  char* buf_cli_mas = calloc(MAXCLI, 1);
  if (buf_cli_mas == NULL) {
    close(pfd_child_out[0]);
    send_error(500, "Internal Server Error", (char*)0,
               "Memory allocation failed", sock);
    fclose(fp);
    return 0;
  }
  char* buf_cli = buf_cli_mas;

  int total_read = 0;
  int bytes_read = 0;
  while (total_read < MAXCLI) {
    bytes_read =
        read(pfd_child_out[0], buf_cli + total_read, MAXCLI - total_read);
    if (bytes_read > 0) {
      total_read += bytes_read;
    } else if (bytes_read == 0) {
      // Ребенок закрыл pipe (завершил работу).
      break;
    } else {
      // bytes_read < 0: реальная ошибка чтения (например, EINTR)
      if (errno == EINTR) {
        continue;  // Прервано сигналом, пробуем снова
      }
      // perror("read from pipe failed");
      break;
    }
  }
  int len_buf_cli = total_read;

  buf_cli[MAXCLI - 1] = '\0';

  close(pfd_child_out[0]);

  int status;
  waitpid(childPid, &status, 0);

  int state_show = 0;
  char *ptr = NULL, *msg = "{\n\t\"message\": \"Enter your query:\"\n}\n";
  if ((ptr = strstr(buf_cli, msg)) != NULL) {
    if (strstr(web_command, "exit") == NULL) {
      buf_cli = ptr + strlen(msg);
    } else {
      buf_cli = msg;
    }
    len_buf_cli = strlen(buf_cli);
    state_show = 1;
  }

  /* Wrong login or password */
  if (state_show == 0) {
    authorization = 0;
  } else if (authorization == 0) {
    fb_user_ip =
        open(USER_IP_FILE, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (fb_user_ip == -1) {
      free(buf_cli_mas);
      send_error(400, "Bad Request", (char*)0, "Bad file user_ip.", sock);
      fclose(fp);
      return 0;
    }
    write(fb_user_ip, surrent_ip, strlen(surrent_ip));
    write(fb_user_ip, " login=", strlen(" login="));
    write(fb_user_ip, login, strlen(login));
    write(fb_user_ip, " password=", strlen(" password="));
    write(fb_user_ip, password, strlen(password));
    write(fb_user_ip, "\n", strlen("\n"));
    close(fb_user_ip);
  }
  /* send file */
  char tmp_name[L_tmpnam];
  tmpnam(tmp_name);
  int fb_temp = open(tmp_name, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);

  write(fb_temp, buf_cli, len_buf_cli);
  send_file(tmp_name, "text/json", sock);

  close(fb_temp);
  remove(tmp_name);
  free(buf_cli_mas);
  fclose(fp);

  return 0;
}
