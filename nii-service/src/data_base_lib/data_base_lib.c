#define _POSIX_C_SOURCE 200809L
#include "data_base_lib.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static void write_record_in_file(FILE* pfile, const ENTITY* record_to_write,
                                 int index, int len);
static ENTITY read_record_from_file(FILE* pfile, int index, int len);
static int get_file_size_in_bytes(FILE* pfile);
static int get_records_count_in_file(FILE* pfile, int len);

/* select */
ENTITY* select_name(FILE* db, const char* name, int len, int* offset) {
  if (db == NULL || name == NULL) return NULL;
  ENTITY* out = NULL;
  int count_st = get_records_count_in_file(db, len);

  for (int found = 0; (*offset) < count_st && !found; (*offset)++) {
    ENTITY e = read_record_from_file(db, *offset, len);
    if (strncmp(e.s.name, name, MAX_NAME) == 0) {
      found = 1;
      if ((out = calloc(1, sizeof(ENTITY))) != NULL) *out = e;
    }
  }
  return out;
}

ENTITY* select_level(FILE* db, int level, int len, int* offset) {
  if (db == NULL) return NULL;
  ENTITY* out = NULL;
  int count_st = get_records_count_in_file(db, len);

  for (int found = 0; (*offset) < count_st && !found; (*offset)++) {
    ENTITY e = read_record_from_file(db, *offset, len);
    if (e.s.level == level) {
      found = 1;
      if ((out = calloc(1, sizeof(ENTITY))) != NULL) *out = e;
    }
  }
  return out;
}

ENTITY* select_author(FILE* db, const char* author, int len, int* offset) {
  if (db == NULL || author == NULL) return NULL;
  ENTITY* out = NULL;
  int count_st = get_records_count_in_file(db, len);

  for (int found = 0; (*offset) < count_st && !found; (*offset)++) {
    ENTITY e = read_record_from_file(db, *offset, len);
    if (strncmp(e.s.author, author, MAX_NAME) == 0) {
      found = 1;
      if ((out = calloc(1, sizeof(ENTITY))) != NULL) *out = e;
    }
  }
  return out;
}
/* delete */
int delete(FILE* db, const char* name, int len) {
  if (db == NULL || name == NULL) return -1;
  int error = 0;
  int offset = 0;
  ENTITY* e = select_name(db, name, len, &offset);

  if (e != NULL) {
    free(e);
    int count_st = get_records_count_in_file(db, len);

    for (int i = offset; i < count_st; i++) {
      ENTITY x = read_record_from_file(db, i, len);
      write_record_in_file(db, &x, i - 1, len);
    }

    int fd = fileno(db);
    ftruncate(fd, (off_t)((count_st - 1) * len));
  } else
    error = -1;

  return error;
}

int insert(FILE* db, const ENTITY* entity, int len) {
  if (db == NULL || entity == NULL) return -1;
  int error = 0;
  int offset = 0;
  ENTITY* e = select_name(db, entity->s.name, len, &offset);

  if (e == NULL) {
    int count_st = get_records_count_in_file(db, len);
    write_record_in_file(db, entity, count_st, len);
  } else {
    free(e);
    error = -1;
  }
  return error;
}

int update(FILE* db, const ENTITY* entity, int len) {
  if (db == NULL || entity == NULL) return -1;
  int offset = 0;
  int error = 0;
  ENTITY* e = select_name(db, entity->d.name, len, &offset);

  if (e != NULL) {
    free(e);
    write_record_in_file(db, entity, offset - 1, len);
  } else
    error = -1;
  return error;
}

/* read/write */
ENTITY read_record_from_file(FILE* pfile, int index, int len) {
  int offset = index * len;
  fseek(pfile, offset, SEEK_SET);
  ENTITY record;
  fread(&record, len, 1, pfile);
  rewind(pfile);
  return record;
}
void write_record_in_file(FILE* pfile, const ENTITY* record_to_write, int index,
                          int len) {
  int offset = index * len;
  fseek(pfile, offset, SEEK_SET);
  fwrite(record_to_write, len, 1, pfile);
  fflush(pfile);
  rewind(pfile);
}
/* len */
int get_records_count_in_file(FILE* pfile, int len) {
  return get_file_size_in_bytes(pfile) / len;
}

int get_file_size_in_bytes(FILE* pfile) {
  if (pfile == NULL) return -1;
  int size = 0;
  fseek(pfile, 0, SEEK_END);
  size = ftell(pfile);
  rewind(pfile);
  return size;
}
