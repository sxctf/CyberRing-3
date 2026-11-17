#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>

#define MAX_NAME 64
#define MAX_PASS 64
#define MAX_DATA 64

struct data {
  char name[MAX_NAME];
  int level;
  char author[MAX_NAME];
  char data[MAX_DATA];
};

struct users {
  char name[MAX_NAME];
  int level;
  unsigned long long password;
};

struct shared {
  char name[MAX_NAME];
  int level;
  char author[MAX_NAME];
};

typedef union {
  struct shared s;
  struct users u;
  struct data d;
} ENTITY;

ENTITY* select_name(FILE* db, const char* name, int len, int* offset);
ENTITY* select_level(FILE* db, int level, int len, int* offset);
ENTITY* select_author(FILE* db, const char* author, int len, int* offset);

int insert(FILE* db, const ENTITY* entity, int len);
int update(FILE* db, const ENTITY* entity, int len);
int delete(FILE* db, const char* name, int len);

#endif
