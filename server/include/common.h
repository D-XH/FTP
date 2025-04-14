#ifndef __TRANS_FILE__
#define __TRANS_FILE__

#include "heads.h"
#include "stack.h"
#include <openssl/aes.h>
#include <openssl/md5.h>

int encrypt_str(unsigned char* in, unsigned char* out, unsigned char* userkey);
int decrypt_str(unsigned char* in, ssize_t in_len, unsigned char* out, unsigned char* userkey);
int send_resp(int net_fd, int code, char* resp, ssize_t resp_size);
int recv_one_data(int net_fd, void* data);
int recv_username_token(int net_fd, char* username, char* token);
ssize_t recvn(int net_fd, void* data, ssize_t n);

int str_replace(char* str, char old_c, char new_c);
int filepath_ch(int net_fd, char* old_cwd, char* new_cwd, char* path, dir_stack_t* cwd);
void md5_to_hex_string(const unsigned char *md, int len, char* hex_str);
int trans_recv(int net_fd, int file_fd, off_t file_size, unsigned char* ser_md5);
int trans_send(int net_fd, int file_fd, off_t file_off);
#endif