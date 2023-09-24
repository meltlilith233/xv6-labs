#include "kernel/types.h"
#include "user/user.h"

void primes(int pipefd[]){
    int prime;
    // 若管道无数据可读，则直接返回
    if(read(pipefd[0], &prime, sizeof(int))<=0){
        return;
    }
    printf("prime %d\n", prime);

    int fd[2];
    pipe(fd);
    if(fork()==0){
        // child process
        close(fd[1]);
        primes(fd);
        close(fd[0]);
    }else{
        // parent process
        close(fd[0]);
        int num;
        while(read(pipefd[0], &num, sizeof(int))){
            if(num%prime!=0){
                write(fd[1], &num, sizeof(int));
            }
        }
        close(fd[1]);
        wait(0);
    }
}

int main(int argc, char* argv[]){
    
    // 文件描述符的打开与关闭遵循一个原则：哪里打开就从哪里关闭
    int fd[2];
    pipe(fd);

    int i;
    for(i=2; i<=35; i++){
        write(fd[1], &i, sizeof(int));
    }

    if(fork()==0){
        close(fd[1]);
        primes(fd);
        close(fd[0]);
    }else{
        close(fd[0]);
        close(fd[1]);
        wait(0);
    }

    exit(0);
}