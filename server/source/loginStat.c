#include "loginStat.h"

void lRotate(tree_node_t** pRoot){
    tree_node_t* root = *pRoot;
    tree_node_t* raw_root = root;
    root = raw_root->rchild;
    tree_node_t* raw_rl = root->lchild;

    root->lchild = raw_root;
    raw_root->rchild = raw_rl;

    int ll_height = raw_root->lchild?raw_root->lchild->high: 0;
    int lr_height = raw_root->rchild?raw_root->rchild->high: 0;
    raw_root->high = MAX(lr_height, ll_height) + 1;

    int r_height = root->rchild? root->rchild->high: 0;
    root->high = MAX(raw_root->high, r_height) + 1;
    *pRoot = root;
}

void rRotate(tree_node_t** pRoot){
    tree_node_t* root = *pRoot;
    tree_node_t* raw_root = root;
    root = raw_root->lchild;
    tree_node_t* raw_lr = root->rchild;

    root->rchild = raw_root;
    raw_root->lchild = raw_lr;

    int rr_height = raw_root->rchild?raw_root->rchild->high: 0;
    int rl_height = raw_root->lchild?raw_root->lchild->high: 0;
    raw_root->high = MAX(rl_height, rr_height) + 1;

    int l_height = root->lchild? root->lchild->high: 0;
    root->high = MAX(raw_root->high, l_height) + 1;
    *pRoot = root;
}

void lrRotate(tree_node_t** pRoot){
    tree_node_t* root = *pRoot;
    lRotate(&root->lchild);
    rRotate(pRoot);
}

void rlRotate(tree_node_t** pRoot){
    tree_node_t* root = *pRoot;
    rRotate(&root->rchild);
    lRotate(pRoot);
}

void llRotate(tree_node_t** pRoot){
    rRotate(pRoot);
}

void rrRotate(tree_node_t** pRoot){
    lRotate(pRoot);
}

int insertTree(tree_node_t **pRoot, tree_node_t *add_node)
{
    tree_node_t* root = *pRoot;
    if(root == NULL){
        *pRoot = add_node;
        return 0;
    }

    if(strncmp(root->token, add_node->token, sizeof(add_node->token)) < 0){
        insertTree(&root->rchild, add_node);
    }else{
        insertTree(&root->lchild, add_node);
    }

    int lHeight = root->lchild? root->lchild->high: 0;
    int rHeight = root->rchild? root->rchild->high: 0;

    root->high = MAX(lHeight, rHeight) + 1;

    int BF = lHeight - rHeight;
    if(BF > 1){
        int llHeight = root->lchild->lchild? root->lchild->lchild->high: 0;
        int lrHeight = root->lchild->rchild? root->lchild->rchild->high: 0;
        if(llHeight < lrHeight){
            lrRotate(pRoot);
        }else{
            llRotate(pRoot);
        }
    }else if(BF < -1){
        int rlHeight = root->rchild->lchild? root->rchild->lchild->high: 0;
        int rrHeight = root->rchild->rchild? root->rchild->rchild->high: 0;
        if(rlHeight > rrHeight){
            rlRotate(pRoot);
        }else{
            rrRotate(pRoot);
        }
    }
    
    return 0;
}

int delFromTree(tree_node_t** pRoot, char* token){
    tree_node_t* root = *pRoot;
    if(root == NULL){
        return 0;
    }
    int cmp_res = strncmp(root->token, token, sizeof(root->token));
    if(cmp_res == 0){
        if(root->lchild == NULL && root->rchild == NULL){
            *pRoot = NULL;
            free(root);
            return 0;
        }else if(root->rchild == NULL){
            *pRoot = root->lchild;
            free(root);
        }else if(root->lchild == NULL){
            *pRoot = root->rchild;
            free(root);
        }else if(root->rchild->lchild == NULL){
            root->rchild->lchild = root->lchild;
            *pRoot = root->rchild;
            free(root);
        }else{
            tree_node_t* pre = root->rchild;
            tree_node_t* p = root->rchild->lchild;
            while(p->lchild){
                pre = p;
                p = p->lchild;
            }
            tree_node_t* t = p->rchild;
            int t_Height = p->high;
            p->lchild = root->lchild;
            p->rchild = root->rchild;
            p->high = root->high;
            *pRoot = p;
            pre->lchild = root;
            root->lchild = NULL;
            root->rchild = t;
            root->high = t_Height;
            delFromTree(&p->rchild, token);
        }
    }else if(cmp_res < 0){
        delFromTree(&root->rchild, token);
    }else{
        delFromTree(&root->lchild, token);
    }

    root = *pRoot;
    int lHeight = root->lchild? root->lchild->high: 0;
    int rHeight = root->rchild? root->rchild->high: 0;

    root->high = MAX(lHeight, rHeight) + 1;

    int BF = lHeight - rHeight;
    if(BF > 1){
        int llHeight = root->lchild->lchild? root->lchild->lchild->high: 0;
        int lrHeight = root->lchild->rchild? root->lchild->rchild->high: 0;
        if(llHeight < lrHeight){
            lrRotate(pRoot);
        }else{
            llRotate(pRoot);
        }
    }else if(BF < -1){
        int rlHeight = root->rchild->lchild? root->rchild->lchild->high: 0;
        int rrHeight = root->rchild->rchild? root->rchild->rchild->high: 0;
        if(rlHeight > rrHeight){
            rlRotate(pRoot);
        }else{
            rrRotate(pRoot);
        }
    }
}

tree_node_t* find(tree_node_t *root, char* token){
    if(root == NULL || strncmp(root->token, token, sizeof(root->token)) == 0){
        return root;
    }
    if(strncmp(root->token, token, sizeof(root->token)) < 0){
        return find(root->rchild, token);
    }else{
        return find(root->lchild, token);
    }
}

int changeFromTree(tree_node_t* root, int fd, int uid, char* token){
    tree_node_t* node = find(root, token);
    node->fd = fd;
    node->uid = uid;
    return 0;
}


void printTree(tree_node_t *root){
    if(root == NULL){
        return;
    }
    printf("node fd: %d, height: %d, uid: %d, token: %s\n", root->fd, root->high, root->uid, root->token);
    printTree(root->lchild);
    printTree(root->rchild);
}

int statTree_init(statTree_t *tree)
{
    tree->root = NULL;
    tree->size = 0;
    return 0;
}

tree_node_t* add_login_user(statTree_t *tree, int fd, int uid, char* token, time_t login_time, int time_round_idx)
{
    tree_node_t* node = (tree_node_t*)malloc(sizeof(tree_node_t));
    memset(node, 0, sizeof(tree_node_t));
    memcpy(node->token, token, strlen(token));
    node->fd = fd;
    node->high = 1;
    node->uid = uid;
    node->lchild = node->rchild = NULL;
    node->login_time = login_time;
    node->timeRound_idx = time_round_idx;
    if(find(tree->root, token) == NULL){
        insertTree(&tree->root, node);
        tree->size++;
    }else{
        changeFromTree(tree->root, fd, uid, token);
    }
    return node;
}

int del_login_user(statTree_t *tree, char* token){
    delFromTree(&tree->root, token);
    tree->size--;
}

tree_node_t* se_login_user(statTree_t *tree, char* token)
{
    return find(tree->root, token);
}
