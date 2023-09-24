#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define BUF_SIZE 1024

int main(int argc, char *argv[]){
    if(argc<2){
        fprintf(2, "usage: xagrs [command] [param1] ...");
        exit(1);
    }

    char *args[MAXARG];

    int i;
    for(i=1; i<argc; i++){
        args[i-1]=argv[i];
    }

    char buf[BUF_SIZE];
    int size;
    while((size=read(0, buf, BUF_SIZE))>0){
        char *p=buf;
        int argSize=0;
        int argIdx=argc-1;
        for(i=0; i<size; i++){
            argSize++;
            // printf("argsSize=%d, i=%d, buf[i]=%c, *p=%s\n", argSize, i, buf[i], p);
            // 如果遇到换行或者末尾，则fork并exec
            if(buf[i]=='\n' || buf[i]=='\0'){
                // 找到单个参数
                buf[i]='\0';
                args[argIdx]=p;

                // 测试：输出找到的参数：
                // printf("%s\n",args[argIdx]);

                // 跳过这个参数
                p=p+argSize;
                // 重置参数长度
                argSize=0;

                if(fork()==0){
                    // child process
                    exec(argv[1], args);
                    // 子进程退出循环
                    break;
                }
                wait(0);

                // 重置args数组，注意，这里可能有个bug，因为我并没有覆盖所有参数
                argIdx=argc-1;
            }

            // 如果是空格，则分割这个参数
            if(buf[i]==' '){
                // 找到单个参数
                buf[i]='\0';
                args[argIdx]=p;

                // 测试：输出找到的参数：
                // printf("%s\n",args[argIdx]);

                // 跳过这个参数
                p=p+argSize;

                // args索引++，重置参数长度
                argIdx++;
                argSize=0;
            }
        }
    }

    exit(0);
}