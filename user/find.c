#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// 返回一个不含路径的文件名
char* fname(char* path){
    char *p=path+strlen(path);
    // 找到第一个'/'
    while(p>=path && *p!='/')p--;
    p++;
    return p;
}

void find(char *path, char *filename){
    char buf[512], *p;

    // stat结构体用于存储文件的状态信息
    struct stat st;

    // Directory is a file containing a sequence of dirent structures.
    // 即文件夹本质就是一个包含很多个dirent结构体的文件
    // 因此，如果我们想遍历一个文件夹中所有的文件，只需要不断地read dirent结构体即可
    struct dirent de;

    int fd;
    if((fd=open(path, O_RDONLY)) < 0){
        fprintf(2, "find: %s isn't exist\n", path);
        return;
    }

    // 获取文件状态：stat和fstat的唯一区别在于是通过文件描述符还是文件名获取状态
    if (fstat(fd, &st)<0){
        fprintf(2, "find: can not stat %s\n", path);
        close(fd);
        return;
    }

    // 根据stat的type属性判断文件类型
    switch(st.type){
        // 如果是普通文件
        case T_FILE:
            // 比较文件名，如果相同则说明找到了！
            if(strcmp(fname(path), filename)==0){
                printf("%s\n", path);
            }
            break;
        
        // 如果是目录文件
        case T_DIR:
            // DIRSIZ是一个文件名的最大长度
            if(strlen(path)+1+DIRSIZ+1>sizeof(buf)){
                fprintf(2, "find: path too long\n");
            }
            strcpy(buf, path);
            p=buf+strlen(buf);
            *p++='/';
            // 遍历当前目录下的所有文件
            while(read(fd, &de, sizeof(de))==sizeof(de)){
                // 跳过空文件
                if(de.inum==0)continue;

                // 递归时一定要忽视当前目录.和上级目录..
                if(strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0)continue;
                // printf("%s\n", de.name);// 测试：打印当前目录所有文件名
                // memmove: 从de.name复制DIRSIZ个字符到p后面
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ]=0;
                // 递归查找
                find(buf, filename);
            }
            break;
    }
    close(fd);
}

int main(int argc, char* argv[]){
    if(argc<3){
        fprintf(2, "usage: find [path] [filename]\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}