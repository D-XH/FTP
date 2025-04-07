#include "stack.h"

int dir_stack_init(dir_stack_t *S)
{
    memset(S, 0, sizeof(dir_stack_t));
    S->topIdx = 0;
    return 0;
}

char *getTop(dir_stack_t *S)
{
    return S->data[S->topIdx - 1];
}

int push(dir_stack_t *S, char *filename)
{
    if(S->topIdx == 256){
        return -1;
    }
    char* p = (char*)malloc(strlen(filename)+1);
    memcpy(p, filename, strlen(filename));
    p[strlen(filename)] = 0;
    S->data[S->topIdx++] = p;
    return 0;
}

int pop(dir_stack_t *S)
{
    if(S->topIdx == 0){
        return -1;
    }
    S->topIdx--;
    free(S->data[S->topIdx]);
    return 0;
}

int clearStack(dir_stack_t *S)
{
    for(int i = 0; i < S->topIdx; i++){
        free(S->data[i]);
    }
    S->topIdx = 0;
    return 0;
}

int get_userCwd(dir_stack_t *S, char* cur_cwd)
{
    if(S->topIdx == 0){
        strcat(cur_cwd, "/");
        return 0;
    }
    for(int i = 0; i < S->topIdx; i++){
        strcat(cur_cwd, "/");
        strcat(cur_cwd, S->data[i]);
    }
    return 0;
}

int set_userCwd(dir_stack_t *S, char *cur_cwd)
{
    clearStack(S);
    char* tok = strtok(cur_cwd, "/");
    while(tok != NULL){
        push(S, tok);
        tok = strtok(NULL, "/");
    }
    return 0;
}
