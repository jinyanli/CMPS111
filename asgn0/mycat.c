#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

void printfile(char* file);

int main(int argc, char* argv[]){
    int i;
    char buffer[1];
    if(argc>1){
       for(i=1; i<argc; i++){
          printfile(argv[i]);
       }
    }
    else{
        int eof;
         do{
           eof=read(0, buffer, 1);
           if(eof==0){
             if(buffer[0]=='\n'){
                if(eof==0)
                break;
             }
             if(read(0, buffer, 1)!=0){
               write(1, buffer, 1);
               continue;
             }else{
               break;
             }
           }
           write(1, buffer, 1);
         } while(1);
    }
    return 0;
}

/*
I looked at this page to study system calls
http://www.di.uevora.pt/~lmr/syscalls.html
*/
void printfile(char* file){
     int fd;
     char buffer[1];
     fd=open(file, O_RDONLY, 0);
     if(fd!=-1){
        while(read(fd, buffer, 1)!=0)
        write(1, buffer, 1);
     }else{
        perror(file);
     }
     close(fd);
}
