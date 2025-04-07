#ifndef __STACK_H__
#define __STACK_H__

#include "heads.h"

typedef struct dir_stack_s{
    char* data[256];
    int topIdx;
}dir_stack_t;

int dir_stack_init(dir_stack_t* S);
char* getTop(dir_stack_t* S);
int push(dir_stack_t* S, char* filename);
int pop(dir_stack_t*S);
int clearStack(dir_stack_t* S);
int get_userCwd(dir_stack_t* S, char* cur_cwd);
int set_userCwd(dir_stack_t* S, char* cur_cwd);
#endif