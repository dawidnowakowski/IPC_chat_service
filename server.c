#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

struct user
{
    char username[10];
    char password[10] ;
};


struct msgbuf
{
    long type;
    int liczba;
} my_msg;



int main(){



    int user_file = open("user_list", O_RDONLY);
    if(user_file == -1){
        printf("code: %d\n", errno);
        perror("Can't open file");
        exit(1);
    }
    

    int n;
    char buf, usernamebuf[10], passwordbuf[10];
    int i = 0, user_or_pass = 0, counter=0;
    struct user users[9];
    

    while((n=read(user_file, &buf, 1))>0){
        if(buf != ' ' && buf != '\n'){
            if (user_or_pass % 2 == 0){
              
                usernamebuf[i] = buf;
                // printf("%c", usernamebuf[i]);

                i++;
            }             
            else{
                passwordbuf[i] = buf;
                // printf("%c", passwordbuf[i]);
                i++;            
            }
        } 
        if(buf == ' ' || buf == '\n'){
            if(buf == ' '){
                usernamebuf[i]='\0';
                strcpy(users[counter].username, usernamebuf);
                usernamebuf[0] = '\0';
            } else{
                strcpy(users[counter].password, passwordbuf);
                passwordbuf[0] = '\0';
            }
            i=0;
            user_or_pass++;
            if(user_or_pass % 2 == 0){
                // printf("username: %s password: %s\n", usernamebuf, passwordbuf);
                counter++;
            }
        }
    }
    strcpy(users[counter].password, passwordbuf);
    for(int j=0; j<9; j++){
        printf("%d: %s %s\n", j, users[j].username, users[j].password);
    }


}