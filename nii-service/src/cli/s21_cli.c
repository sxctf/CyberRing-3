#define _POSIX_C_SOURCE 200809L  // POSIX.1-2008/2017
#define _ISOC11_SOURCE           // C11 функции
#define _DEFAULT_SOURCE          // Расширения Linux/glibc
#define _GNU_SOURCE              // GNU-специфичные функции

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../data_base_lib/data_base_lib.h"

#define USER_DB "../../data/user_db"
#define DATA_DB "../../data/data_db"

typedef enum {
  ERR_OK = 0,         // Успех
  ERR_RESET_DB,       // Базы данных сброшены
  ERR_NO_DB,          // Нет баз данных
  ERR_INVALID_PARAM,  // Неверный параметр
  ERR_OUT_OF_MEMORY,  // Нахватка памяти
} cli_status;

typedef struct {
  char login[64];
  char password[64];
  int level;
  int web;
  FILE* users_db;
  FILE* data_db;
  cli_status err;
} cli_data;

unsigned long long hash_psw(const char* psw);
void init_db(cli_data* ops);
void login(cli_data* ops);
void query_db(cli_data* ops);
int select_data(cli_data* ops);
int insert_data(cli_data* ops);
int update_data(cli_data* ops);
int delete_data(cli_data* ops);

int select_user(cli_data* ops, int me);
int insert_user(cli_data* ops);
int update_user(cli_data* ops);
int delete_user(cli_data* ops);

void print_data(cli_data* ops, struct data* row);
void print_user(cli_data* ops, struct users* row);
void print_msg(cli_data* ops, const char* msg);

int main(int argc, char* argv[]) {
  cli_data ops = {0};
  ops.users_db = fopen(USER_DB, "rb+");
  ops.data_db = fopen(DATA_DB, "rb+");

  if (!ops.users_db || !ops.data_db) {
    ops.users_db = fopen(USER_DB, "wb+");
    ops.data_db = fopen(DATA_DB, "wb+");
    if (!ops.users_db || !ops.data_db) {
      ops.err = ERR_NO_DB;
      printf("Error: Database missing!\n");
    }
  }
  chmod(USER_DB, 0666);
  chmod(DATA_DB, 0666);

  init_db(&ops);

  if (ops.err == ERR_OK && argc == 2) {
    if (!strcmp(argv[1], "w")) {
      ops.web = 1;
    } else {
      ops.err = ERR_INVALID_PARAM;
      printf("Error: Invalid parameter!\n");
    }
  }

  if (ops.err == ERR_OK) {
    login(&ops);
    query_db(&ops);
  }

  if (ops.users_db) fclose(ops.users_db);
  if (ops.data_db) fclose(ops.data_db);

  return ops.err;
}

unsigned long long hash_psw(const char* psw) {
  unsigned long long hash = 14695981039346656037ULL;

  while (psw && *psw) {
    hash ^= (unsigned long long)((unsigned char)*psw);
    hash *= 1099511628211ULL;
    psw++;
  }

  return hash;
}

void init_db(cli_data* ops) {
  if (ops->err == ERR_OK) {
    fseek(ops->users_db, 0, SEEK_END);
    if (!ftell(ops->users_db)) {
      int fd = fileno(ops->users_db);
      ftruncate(fd, 0);
      rewind(ops->users_db);

      ENTITY user = {0};

      strcpy(user.u.name, "admin");
      user.u.level = 4;
      user.u.password = hash_psw("admin");
      insert(ops->users_db, &user, sizeof(struct users));

      user = (ENTITY){0};
      strcpy(user.u.name, "checker");
      user.u.level = 4;
      user.u.password = hash_psw(getenv("CHECKER_PSW"));
      insert(ops->users_db, &user, sizeof(struct users));

      user = (ENTITY){0};
      strcpy(user.u.name, "user");
      user.u.level = 2;
      user.u.password = hash_psw("user");
      insert(ops->users_db, &user, sizeof(struct users));

      fd = fileno(ops->data_db);
      ftruncate(fd, 0);
      rewind(ops->data_db);
    }

    fseek(ops->data_db, 0, SEEK_END);
    if (!ftell(ops->data_db)) {
      ENTITY sec = {0};

      strcpy(sec.d.name, "n1");
      sec.d.level = 4;
      strcpy(sec.d.author, "admin");
      strcpy(sec.d.data, "main_sec_data");
      insert(ops->data_db, &sec, sizeof(struct data));

      sec = (ENTITY){0};
      strcpy(sec.d.name, "k5");
      sec.d.level = 2;
      strcpy(sec.d.author, "user");
      strcpy(sec.d.data, "kaholice");
      insert(ops->data_db, &sec, sizeof(struct data));

      sec = (ENTITY){0};
      strcpy(sec.d.name, "n6");
      sec.d.level = 2;
      strcpy(sec.d.author, "user");
      strcpy(sec.d.data, "ninetalw");
      insert(ops->data_db, &sec, sizeof(struct data));

      sec = (ENTITY){0};
      strcpy(sec.d.name, "o7");
      sec.d.level = 1;
      strcpy(sec.d.author, "user");
      strcpy(sec.d.data, "orphansi");
      insert(ops->data_db, &sec, sizeof(struct data));

      sec = (ENTITY){0};
      strcpy(sec.d.name, "lvl");
      sec.d.level = 1;
      strcpy(sec.d.author, "checker");
      strcpy(sec.d.data, "minor_sec_data");
      insert(ops->data_db, &sec, sizeof(struct data));
    }
  }
}

void login(cli_data* ops) {
  int offset = 0;
  if (!ops->web) printf("Enter your login: ");
  scanf("%s", ops->login);
  if (!ops->web) printf("Enter your password: ");
  scanf("%s", ops->password);
  ENTITY* result =
      select_name(ops->users_db, ops->login, sizeof(struct users), &offset);
  if (result && result->u.password == hash_psw(ops->password)) {
    ops->level = result->u.level;
  } else {
    print_msg(ops, "Error: Login or password incorrect!");
  }
  free(result);
}

/*
select level 4
select level 3
select level 2
select level 1
select level all
select name data_name
select author user_login

insert data_name level sec_data
update data_name level sec_data
delete data_name
*/

void query_db(cli_data* ops) {
  while (ops->level) {
    char cmd[32] = {0};
    int cmd_result = 0;

    print_msg(ops, "Enter your query:");
    scanf("%s", cmd);
    if (ops->web) printf("[\n");

    if (!strcmp("select", cmd)) {
      cmd_result = select_data(ops);
    } else if (!strcmp("insert", cmd)) {
      cmd_result = insert_data(ops);
    } else if (!strcmp("update", cmd)) {
      cmd_result = update_data(ops);
    } else if (!strcmp("delete", cmd)) {
      cmd_result = delete_data(ops);
    } else if (!strcmp("sel_user", cmd)) {
      cmd_result = select_user(ops, 0);
    } else if (!strcmp("sel_me", cmd)) {
      cmd_result = select_user(ops, 1);
    } else if (!strcmp("add_user", cmd)) {
      cmd_result = insert_user(ops);
    } else if (!strcmp("upd_user", cmd)) {
      cmd_result = update_user(ops);
    } else if (!strcmp("del_user", cmd)) {
      cmd_result = delete_user(ops);
    } else if (!strcmp("exit", cmd)) {
      ops->level = 0;
    } else {
      cmd_result = 1;
    }
    if (ops->web) ops->level = 0;
    if (cmd_result) print_msg(ops, "Error: Incorrect command!");

    if (ops->web) printf("]\n");
  }
}

int select_data(cli_data* ops) {
  int offset = 0, err = 0;
  char cmd[32] = {0};
  char name[MAX_NAME] = {0};
  scanf("%s", cmd);

  if (!strcmp("name", cmd)) {
    scanf("%s", name);
    ENTITY* result =
        select_name(ops->data_db, name, sizeof(struct data), &offset);
    if (result) {
      if (result->d.level <= ops->level) {
        print_data(ops, &result->d);
      } else {
        print_msg(ops, "Your access level is not suitable");
      }
      free(result);
    } else {
      print_msg(ops, "No data found!");
    }

  } else if (!strcmp("level", cmd)) {
    scanf("%s", name);
    if (!strcmp("1", name) || !strcmp("2", name) || !strcmp("3", name) ||
        !strcmp("4", name)) {
      int level = *name - '0', stop = 0, found = 0;
      if (level <= ops->level) {
        while (!stop) {
          ENTITY* result =
              select_level(ops->data_db, level, sizeof(struct data), &offset);
          if (result) {
            if (ops->web && found) printf(",\n");
            print_data(ops, &result->d);
            found = 1;
            free(result);
          } else {
            stop = 1;
          }
        }
        if (!found) print_msg(ops, "No data found!");

      } else {
        print_msg(ops, "Your access level is not suitable");
      }
    } else if (!strcmp("all", name)) {
      int found = 0;

      for (int level = 1; level <= ops->level && level <= 4; ++level) {
        int stop = 0, offset = 0;
        while (!stop) {
          ENTITY* result =
              select_level(ops->data_db, level, sizeof(struct data), &offset);
          if (result) {
            if (ops->web && found) printf(",\n");
            print_data(ops, &result->d);
            found = 1;
            free(result);
          } else {
            stop = 1;
          }
        }
      }
      if (!found) print_msg(ops, "No data found!");

    } else {
      err = 1;
    }

  } else if (!strcmp("author", cmd)) {
    scanf("%s", name);
    int stop = 0, offset = 0, found = 0;

    while (!stop) {
      ENTITY* result =
          select_author(ops->data_db, name, sizeof(struct data), &offset);
      if (result) {
        if (result->d.level <= ops->level) {
          if (ops->web && found) printf(",\n");
          found = 1;
          print_data(ops, &result->d);
        }
        free(result);
      } else {
        stop = 1;
      }
    }
    if (!found) print_msg(ops, "No data found!");

  } else {
    err = 1;
  }

  return err;
}

int insert_data(cli_data* ops) {
  int err = 0;
  ENTITY data = {0};

  if (scanf("%s %d %s", data.d.name, &data.d.level, data.d.data) == 3) {
    memcpy(data.d.author, ops->login, strlen(ops->login));

    if (data.d.level > 0 && data.d.level <= ops->level) {
      if (!insert(ops->data_db, &data, sizeof(struct data))) {
        print_msg(ops, "Data inserted");
      } else {
        print_msg(ops, "Data not inserted - already exist");
      }
    } else {
      print_msg(ops, "Your access level is not suitable");
    }
  } else {
    err = 1;
  }

  return err;
}

int update_data(cli_data* ops) {
  int err = 0;
  ENTITY data = {0};

  if (scanf("%s %d %s", data.d.name, &data.d.level, data.d.data) == 3) {
    if (data.d.level > 0 && data.d.level <= ops->level) {
      int offset = 0;
      ENTITY* res =
          select_name(ops->data_db, data.d.name, sizeof(struct data), &offset);

      if (res) {
        if (res->d.level <= ops->level) {
          memcpy(data.d.author, ops->login, strlen(ops->login));
          update(ops->data_db, &data, sizeof(struct data));
          print_msg(ops, "Data updated");
        } else {
          print_msg(ops, "Your access level is not suitable");
        }
        free(res);
      } else {
        print_msg(ops, "Data not exist");
      }
    } else {
      print_msg(ops, "Your access level is not suitable");
    }
  } else {
    err = 1;
  }

  return err;
}

int delete_data(cli_data* ops) {
  int err = 0;
  ENTITY data = {0};

  if (scanf("%s", data.d.name) == 1) {
    int offset = 0;
    ENTITY* res =
        select_name(ops->data_db, data.d.name, sizeof(struct data), &offset);

    if (res) {
      if (res->d.level <= ops->level) {
        delete (ops->data_db, data.d.name, sizeof(struct data));
        print_msg(ops, "Data deleted");
      } else {
        print_msg(ops, "Your access level is not suitable");
      }
      free(res);
    } else {
      print_msg(ops, "Data not exist");
    }

  } else {
    err = 1;
  }

  return err;
}

/*
add_user name3452 3 passghjkh4523   (только админ, либо пользователя ниже
                                    уровнем)
del_user name4565                   (только себя или ниже уровнем, если я 4
                                    уровень)
upd_user name3452 2 padhdyus        (только себя или ниже уровнем, если я 4
                                    уровень)
sel_user                            (только себя или всех ниже 4 уровня)
*/

int select_user(cli_data* ops, int me) {
  int err = 0;
  int found = 0;

  for (int level = 1; level <= ops->level && level <= 4; ++level) {
    int stop = 0, offset = 0;
    while (!stop) {
      ENTITY* result =
          select_level(ops->users_db, level, sizeof(struct users), &offset);
      if (result) {
        if ((!me && ((level < ops->level && result->u.level < 4) ||
                     !strcmp(ops->login, result->u.name))) ||
            (me && !strcmp(ops->login, result->u.name))) {
          if (ops->web && found) printf(",\n");
          print_user(ops, &result->u);
          found = 1;
        }
        free(result);
      } else {
        stop = 1;
      }
    }
  }

  if (!found) print_msg(ops, "No users found!");

  return err;
}

int insert_user(cli_data* ops) {
  int err = 0;
  ENTITY user = {0};
  char psw[MAX_PASS] = {0};

  if (scanf("%s %d %s", user.u.name, &user.u.level, psw) == 3) {
    if (ops->level == 4 ||
        (ops->level < 4 && user.u.level > 0 && user.u.level < ops->level)) {
      user.u.password = hash_psw(psw);
      if (!insert(ops->users_db, &user, sizeof(struct users))) {
        print_msg(ops, "User added");
      } else {
        print_msg(ops, "User not added - already exist");
      }
    } else {
      print_msg(ops, "Your access level is not suitable");
    }
  } else {
    err = 1;
  }

  return err;
}

int update_user(cli_data* ops) {
  int err = 0;
  ENTITY user = {0};
  char psw[MAX_PASS] = {0};

  if (scanf("%s %d %s", user.u.name, &user.u.level, psw) == 3) {
    int offset = 0;
    ENTITY* exist_user =
        select_name(ops->users_db, user.u.name, sizeof(struct users), &offset);
    int old_level = exist_user ? exist_user->u.level : 0;

    if ((ops->level == 4 && user.u.level > 0 && user.u.level < 4 &&
         (old_level ? old_level < 4 : 1)) ||
        (!strcmp(user.u.name, ops->login) && user.u.level == ops->level)) {
      user.u.password = hash_psw(psw);
      if (!update(ops->users_db, &user, sizeof(struct users))) {
        print_msg(ops, "User updated");
      } else {
        print_msg(ops, "User not exist");
      }
    } else {
      print_msg(ops, "Your access level is not suitable");
    }
    free(exist_user);
  } else {
    err = 1;
  }

  return err;
}

int delete_user(cli_data* ops) {
  int err = 0;
  ENTITY user = {0};

  if (scanf("%s", user.u.name) == 1) {
    int offset = 0;
    ENTITY* exist_user =
        select_name(ops->users_db, user.u.name, sizeof(struct users), &offset);
    user.u.level = exist_user ? exist_user->u.level : 0;

    if ((ops->level == 4 && user.u.level < 4) ||
        (!strcmp(user.u.name, ops->login) && user.u.level == ops->level)) {
      if (!delete (ops->users_db, user.u.name, sizeof(struct users))) {
        print_msg(ops, "User deleted");
      } else {
        print_msg(ops, "User not exist");
      }
    } else {
      print_msg(ops, "Your access level is not suitable");
    }
    free(exist_user);
  } else {
    err = 1;
  }

  return err;
}

void print_data(cli_data* ops, struct data* row) {
  if (ops->web) {
    printf("{\n");
    printf("\t\"name\": \"%s\",\n", row->name);
    printf("\t\"level\": %d,\n", row->level);
    printf("\t\"author\": \"%s\",\n", row->author);
    printf("\t\"data\": \"%s\"\n", row->data);
    printf("}\n");
  } else {
    printf("data: %s %d %s %s\n", row->name, row->level, row->author,
           row->data);
  }
}

void print_user(cli_data* ops, struct users* row) {
  if (ops->web) {
    printf("{\n");
    printf("\t\"name\": \"%s\",\n", row->name);
    printf("\t\"level\": %d\n", row->level);
    printf("}\n");
  } else {
    printf("user: %s %d\n", row->name, row->level);
  }
}

void print_msg(cli_data* ops, const char* msg) {
  if (ops->web) {
    printf("{\n\t\"message\": \"%s\"\n}\n", msg);
  } else {
    printf("%s\n", msg);
  }
}
