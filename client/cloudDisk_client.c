#include <stdio.h>

int main(int argc, char* argv[]){
    if(argc != 3){
        perror("main args");
        return -1;
    }
    
    return 0;
}