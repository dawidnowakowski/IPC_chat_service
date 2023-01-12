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

struct group
{
    int id;
    char groupname[20];
};

struct user
{
    char username[10];
    char password[10];
    int is_logged;
    struct group groups[3]; 
    int PID;
};


struct msgbuf
{
    long type;
    int pid;
    char text[1024];
};


void openFileAndFillUserList(char filename[], struct user users[]){
    int user_file = open("user_list", O_RDONLY);
    if(user_file == -1){
        printf("code: %d\n", errno);
        perror("Can't open file");
        exit(1);
    }
    

    int n;
    char buf, usernamebuf[10], passwordbuf[10];
    int i = 0, user_or_pass = 0, counter=0;
    
    

    while((n=read(user_file, &buf, 1))>0){
        if(buf != ' ' && buf != '\n'){ //if letter then put it in username buffer and password buffer
            if (user_or_pass % 2 == 0){
                usernamebuf[i] = buf;
                i++;
            } else{
                passwordbuf[i] = buf;
                i++;            
            }
        } //if space or new line copy them into struct array and prepare for next iteration
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
                counter++;
            }
        }
    }
    strcpy(users[counter].password, passwordbuf); //last password cant be copied in loop
}

void printAllUsers(struct user users[], int len){
    for(int j=0; j < len; j++){
        printf("%d: %s %s\n", j, users[j].username, users[j].password);
    }
}

void openFileAndFillGroups(struct group groups[], char filename[]){
    int topics = open(filename, O_RDONLY);
    if(topics == -1){
        printf("code: %d\n", errno);
        perror("Can't open file");
        exit(1);
    }

    int id = 0, j=0, n;
    char buf;
    char groupnamebuf[20];
    while((n=read(topics, &buf, 1))>0){
        if(buf != '\n'){
            groupnamebuf[j] = buf;
            j++;
        } else{
            groupnamebuf[j] = '\0';
            strcpy(groups[id].groupname, groupnamebuf);
            groups[id].id = id;
            groupnamebuf[0] = '\0';
            
            id++;
            j=0;
        }
        

    }
    strcpy(groups[id].groupname, groupnamebuf);
    groups[id].id = id;
}

void printAllGroups(struct group groups[], int len){

    for(int j=0; j < len; j++){
        printf("%d: %s %d\n", j, groups[j].groupname, groups[j].id);

    }
}

int main(){
    struct user users[9];
    char filename[] = "user_list";
    openFileAndFillUserList(filename, users);
    int num_of_users = sizeof(users)/sizeof(users[0]);
    printAllUsers(users, num_of_users);

    // struct group groups[3];
    // openFileAndFillGroups(groups, "topic_groups");
    // int num_of_groups = sizeof(groups)/ sizeof(groups[0]);
    // printAllGroups(groups, num_of_groups);

    struct msgbuf login_message;    
    int LOGIN_QUEUE = msgget(9000, 0664 | IPC_CREAT);
    msgrcv(LOGIN_QUEUE, &login_message, sizeof(int)+1024, 1, 0);
    // printf("%d %s\n", login_message.pid, login_message.text);

    char username[10];
    char passwd[10];
    char *token = strtok(login_message.text, " ");


    strcpy(username, token);

    token = strtok(NULL, " ");

    strcpy(passwd, token);


    printf("%s %s\n", username, passwd);

    



}