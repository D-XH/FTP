#include "common.h"

int encrypt_str(unsigned char* in, unsigned char* out, unsigned char* userkey){
    AES_KEY key;
    AES_set_encrypt_key(userkey, 128, &key);
    unsigned char iv[AES_BLOCK_SIZE] = {0};

    int in_size = strlen(in);
    int pad_size = AES_BLOCK_SIZE - (in_size % AES_BLOCK_SIZE);;
    int total_size = in_size + pad_size;
    if(pad_size != 0){
        memset(in+in_size, pad_size, pad_size);
    }
    AES_cbc_encrypt(in, out, total_size, &key, iv, AES_ENCRYPT);
    return total_size;
}

int decrypt_str(unsigned char* in, ssize_t in_len, unsigned char* out, unsigned char* userkey){
    AES_KEY key;
    AES_set_decrypt_key(userkey, 128, &key);
    unsigned char iv[AES_BLOCK_SIZE] = {0};
    AES_cbc_encrypt(in, out, 1024, &key, iv, AES_DECRYPT);

    int pad_size = out[in_len - 1];
    if(pad_size < 1 || pad_size > AES_BLOCK_SIZE){
        printf("padding err\n");
        memset(out, 0, in_len);
        return -1;
    }

    int real_len = in_len - pad_size;
    memset(out + real_len, 0, pad_size);
    return 0;
}

ssize_t recvn(int net_fd, void* data, ssize_t n){
    char* p = (char*)data;
    ssize_t cnt = 0;
    while(cnt < n){
        ssize_t rsize = recv(net_fd, p, n, 0);
        if(rsize == 0){
            return 0;
        }
        cnt += rsize;
    }
    return cnt;
}

int recv_one_data(int net_fd, void* data){
    char* p = (char*)data;

    data_t _data;
    memset(&_data, 0, sizeof(_data));
    recvn(net_fd, &_data.size, sizeof(_data.size));
    ssize_t rsize = recvn(net_fd, _data.buf, _data.size);
    memcpy(p, _data.buf, _data.size);
    return rsize;
}

int recv_username_token(int net_fd, char* username, char* token){
    char encrypt_name_and_token[1024] = {0};
    char name_and_token[1024] = {0};
    int len_of_name_and_token = recv_one_data(net_fd, encrypt_name_and_token);
    if(len_of_name_and_token <= 0){
        return 0;
    }

    char userkey[16] = "deng";
    if(decrypt_str(encrypt_name_and_token, len_of_name_and_token, name_and_token, userkey) == -1){
        return 0;
    }

    // printf("name and token: %s\n", name_and_token);
    char* tok = strtok(name_and_token, " ");
    if(tok == NULL){return 0;}
    memcpy(username, tok, strlen(tok));
    tok = strtok(NULL, " ");
    if(tok == NULL){return 0;}
    memcpy(token, tok, strlen(tok));
    return 0;
}

int send_data(int net_fd, data_t* data, int type){
    data_t msg_data;
    memset(&msg_data, 0, sizeof(msg_data));

    int msg_type = type;
    msg_data.size = sizeof(msg_type);
    memcpy(msg_data.buf, &msg_type, sizeof(int));
    send(net_fd, &msg_data, sizeof(msg_data.size)+msg_data.size, MSG_NOSIGNAL);

    if(send(net_fd, data, sizeof(data->size)+data->size, MSG_NOSIGNAL) == -1){
        perror("send");
    }
    return 0;
}

// 200: ok 
// 220: register successful
// 230: login successful --> resp name_and_token
// 231: exit successful
// 257: resp of pwd --> pwd string
// 258: resp of ls --> ls string
// 259: upload successful
// 350: 
// 501: arg err
// 502: upload failed
// 503: val failed
// 504: multi login
int send_resp(int net_fd, int code, char* resp, ssize_t resp_size){
    //
    data_t data;
    memset(&data, 0, sizeof(data));
    data.size = sizeof(code);
    memcpy(data.buf, &code, data.size);
    send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);

    memset(&data, 0, sizeof(data));
    data.size = resp_size == -1? strlen(resp):resp_size;
    memcpy(data.buf, resp, data.size);
    send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
}

int str_replace(char* str, char old_c, char new_c){
    char* p = strchr(str, old_c);
    while(p != NULL){
        *p = new_c;
        p = strchr(str, old_c);
    }
    return 0;
}

int filepath_ch(int net_fd, char* old_cwd, char* new_cwd, char* path, dir_stack_t* cwd){
    get_userCwd(cwd, old_cwd);
    if(path[0] == '/'){
        clearStack(cwd);
    }
    char* tok = strtok(path, "/");
    while(tok != NULL){
        if(strlen(tok) == 1 && tok[0] == '.'){
            
        }else if(strlen(tok) == 2 && memcmp(tok, "..", 2) == 0){
            if(cwd->topIdx == 0){
                set_userCwd(cwd, old_cwd);
                send_resp(net_fd, 501, "no such file or directory!", -1);
                return 0;
            }else{
                pop(cwd);
            }
        }else{
            push(cwd, tok);
        }
        tok = strtok(NULL, "/");
    }
    get_userCwd(cwd, new_cwd);
    
    return 0;
}

void md5_to_hex_string(const unsigned char *md, int len, char* hex_str) {
    for (int i = 0; i < len; i++) {
        sprintf(hex_str + i * 2, "%02x", md[i]); // 小写
    }
    hex_str[len * 2] = '\0';
}

int trans_recv(int net_fd, int file_fd, off_t file_size, unsigned char* ser_md5){
    MD5_CTX ctx;
    MD5_Init(&ctx);
    if(file_size > 100*1024*1024){
        // mmap
        ftruncate(file_fd, file_size);
        char* p = (char*)mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
        char* t = p;
        recvn(net_fd, p, file_size);
        MD5_Update(&ctx, t, file_size);
        munmap(p, file_size);
    }else{
        // normal
        int cnt = 0;
        while(cnt < file_size){
            data_t data;
            memset(&data, 0, sizeof(data));
            recvn(net_fd, &data.size, sizeof(data.size));
            recvn(net_fd, data.buf, data.size);
            if(data.size == 0) break;
            MD5_Update(&ctx, data.buf, data.size);
            write(file_fd, data.buf, data.size);
            cnt += data.size;
        }
    }
    unsigned char _ser_md5[256] = {0};
    MD5_Final(_ser_md5, &ctx);
    md5_to_hex_string(_ser_md5, strlen(_ser_md5), ser_md5);
}

// int trans_send(int net_fd, int file_fd, off_t file_size, off_t file_off){
//     if(file_size > 100*1024*1024){
//         // mmap
//         ftruncate(file_fd, file_size);
//         char* p = (char*)mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
//         send(net_fd, p+file_off, file_size-file_off, MSG_NOSIGNAL);
//         munmap(p, file_size);
//     }else{
//         // normal
//         int cnt = file_off;
//         data_t data;
//         lseek(file_fd, cnt, SEEK_SET);
//         while(cnt < file_size){
//             memset(&data, 0, sizeof(data));
//             ssize_t rsize = read(file_fd, data.buf, sizeof(data.buf));
//             data.size = rsize;
//             send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
//             if(rsize == 0){
//                 break;
//             }
//             cnt += rsize;
//         }
//     }
// }

int trans_send(int net_fd, int file_fd, off_t file_off){
    
    // normal
    data_t data;
    struct tcp_info info;
    socklen_t len = sizeof(info);
    lseek(file_fd, file_off, SEEK_SET);
    while(1){
        memset(&data, 0, sizeof(data));
        ssize_t rsize = read(file_fd, data.buf, sizeof(data.buf));
        data.size = rsize;

        size_t send_size = sizeof(data.size)+data.size;
        getsockopt(net_fd, IPPROTO_TCP, TCP_INFO, &info, &len);
        if (info.tcpi_rcv_space - send_size < info.tcpi_rcv_ssthresh) {
            // 接收空间接近阈值
            sleep(0.001);
        }

        ssize_t ssize = send(net_fd, &data, send_size, MSG_NOSIGNAL);
        if(rsize == 0){
            break;
        }
    }
}