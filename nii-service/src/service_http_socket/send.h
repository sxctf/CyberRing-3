#ifndef SEND_H
#define SEND_H

void send_headers(int status, char* title, char* extra_header, char* mime_type,
                  off_t length, time_t mod, int sock);
void send_error(int status, char* title, char* extra_header, char* text,
                int sock);
void send_file(const char* fname, const char* type, int sock);

#endif