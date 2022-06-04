#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void EXIT(int sig){
    printf("Get signal%d, program exit!\n", sig);

    // add free code at here
    
    exit(0);  //exit program
}

int main(){
    for (int ii = 0; ii < 100; ii++) signal(ii, SIG_IGN);

    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    while (1) sleep(10); //died loop

}
