#include "timeRound.h"

/////////////////////////////////////////////// link list func ///////////////////////////////////////////////

int insert_head(linkLisk_node_t** head_ptr, linkLisk_node_t* add_node){
    linkLisk_node_t* head = *head_ptr;
    if(head == NULL){
        *head_ptr = add_node;
        return 0;
    }
    add_node->next = head;
    *head_ptr = add_node;
    return 0;
}

int del_one(linkLisk_node_t** head_ptr, char* token){
    linkLisk_node_t* head = *head_ptr;
    if(head == NULL){
        return 0;
    }
    if(strncmp(head->token, token, strlen(token)) == 0){
        // del
        *head_ptr = head->next;
        head->token = NULL;
        free(head);
        return 0;
    }
    del_one(&head->next, token);
    return 0;
}

/////////////////////////////////////////////// link list func ///////////////////////////////////////////////

int timeRound_init(timeRound_t* tr, size_t time){
    memset(tr, 0, sizeof(timeRound_t));
    tr->cur_idx = 0;
    tr->total_time = time;
    return 0;
}

int timeRound_add(timeRound_t* tr, char* token){
    linkLisk_node_t* add_node = (linkLisk_node_t*)malloc(sizeof(linkLisk_node_t));
    add_node->next = NULL;
    add_node->token = token;
    if(token == NULL){
        printf("token is NULL\n");
    }
    insert_head(&tr->round[tr->cur_idx], add_node);
    return 0;
}

int timeRound_del(timeRound_t* tr, size_t idx, char* token){
    del_one(&tr->round[idx], token);
    return 0;
}

linkLisk_node_t* timeRound_step(timeRound_t* tr){
    tr->cur_idx = (tr->cur_idx + 1) % tr->total_time;
    linkLisk_node_t* p = tr->round[tr->cur_idx];
    return p;
}
