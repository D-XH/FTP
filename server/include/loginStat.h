#ifndef __LOGIN_STAT__
#define __LOGIN_STAT__

#define MAX(a,b) ((a)>(b)?(a):(b))

#include "heads.h"
#include "stack.h"

typedef struct tree_node_s{
    char token[256];
    int fd;
    int uid;
    dir_stack_t cwd;
    time_t login_time;
    int timeRound_idx;
    int high;
    struct tree_node_s* lchild;
    struct tree_node_s* rchild;
}tree_node_t;

typedef struct statTree_s{
    tree_node_t* root;
    int size;
}statTree_t;

void printTree(tree_node_t *root);
int statTree_init(statTree_t* tree);
tree_node_t* add_login_user(statTree_t* tree, int fd, int uid, char* token, time_t login_time, int time_round_idx);
int del_login_user(statTree_t *tree, char* token);
tree_node_t* se_login_user(statTree_t *tree, char* token);

#endif