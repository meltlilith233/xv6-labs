#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]){

    int ping[2];
    int pong[2];
    pipe(ping);
    pipe(pong);

    int size=10;
    char buf[size];

    if(fork()!=0){// parent
        close(ping[0]);
        close(pong[1]);
        // write in ping
        if (write(ping[1], "p", 1)<1)
        {
            fprintf(2, "ping: write error\n");
            exit(1);
        }
        close(ping[1]);

        wait(0);

        // read from pong
        if (read(pong[0], buf, size)<0)
        {
            fprintf(2, "ping: write error\n");
            exit(1);
        }
        fprintf(1, "%d: received pong\n", getpid());
        close(pong[0]);
    }else{// child
        close(pong[0]);
        close(ping[1]);
        // write in pong
        if (write(pong[1], "c", 1)<1)
        {
            fprintf(2, "ping: write error\n");
            exit(1);
        }
        close(pong[1]);

        // read from ping
        if (read(ping[0], buf, size)<0)
        {
            fprintf(2, "ping: write error\n");
            exit(1);
        }
        fprintf(1, "%d: received ping\n", getpid());
        close(ping[0]);
    }

    // all process exit
    exit(0);
}