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
    char password[10];
    int groups[3]; 
    int is_logged; // 0 - not logged, 1 - logged in
    int PID; //PID is key of message queue
    int failed_attempts; //if 3 then don't allow more attempts
    int QUEUEID; //queueid
};

struct group
{
    int id;
    char groupname[20];                 
    int members;
    int members_PID[9];
};

struct msgbuf
{
    long type;
    int PID; //PID will be a message queue key
    char text[1024]; //message
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
                users[counter].is_logged = 0;
                for(int c=0; c<3; c++){
                    users[counter].groups[c] = -1;
                }
                users[counter].PID = 0;
                users[counter].failed_attempts = 0;
                users[counter].QUEUEID = -1;
            }
            i=0;
            user_or_pass++;
            if(user_or_pass % 2 == 0){
                counter++;
            }
        }
    }
    strcpy(users[counter].password, passwordbuf); //last password cant be copied in loop
    users[counter].is_logged = 0;
    users[counter].PID = 0;
    users[counter].failed_attempts = 0;
    users[counter].QUEUEID = -1;
    for(int c=0; c<3; c++){
        users[counter].groups[c] = -1;
    }
}

void printUserInfo(struct user user){
    printf("username: %s password: %s is_logged: %d PID: %d failed_attempts: %d, QUEUEID: %d\n", user.username, user.password, user.is_logged, user.PID, user.failed_attempts, user.QUEUEID);
}

void printAllUsers(struct user users[], int len){
    for(int j=0; j < len; j++){
        printf("%d: %s %s\n", j, users[j].username, users[j].password);
    }
}

void printUserGroups(struct user user){
    printf("Groups:\n");
    for(int c=0; c<3; c++){
        printf("%d\n", user.groups[c]);
    }
}

void printAllUsersInfo(struct user users[], int len){
    for(int j=0; j < len; j++){
        // printf("%d: username: %s password: %s is_logged: %d PID: %d failed_attempts: %d\n", j, users[j].username, users[j].password, users[j].is_logged, users[j].PID, users[j].failed_attempts);
        printUserInfo(users[j]);
        printUserGroups(users[j]);
        printf("\n");
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
            for(int c=0; c<9; c++){
                groups[id].members_PID[c] = -1;
            }
            groups[id].members = 0;
            groupnamebuf[0] = '\0';
            
            id++;
            j=0;
        }
        

    }
    strcpy(groups[id].groupname, groupnamebuf);
    groups[id].id = id;
    for(int c=0; c<9; c++){
            groups[id].members_PID[c] = -1;
        }
    groups[id].members = 0;
}

void printAllGroups(struct group groups[], int len){

    for(int j=0; j < len; j++){
        printf("%d: %s %d\n", j, groups[j].groupname, groups[j].id);
        printf("Members %d:\n", groups[j].members);
        for(int c=0; c<9; c++){
            printf("%d\n", groups[j].members_PID[c]);
        }


    }
}

void handleLogIn(int LOGIN_QUEUE, int num_of_users, struct msgbuf message, struct user users[], int* ACTIVE_USERS_COUNTER){
    char username[10];
    char passwd[10];
    char *token = strtok(message.text, " ");    
    strcpy(username, token);
    token = strtok(NULL, " ");
    strcpy(passwd, token);
    // printf("PID: %d username: %s password: %s\n", message.PID, username, passwd);

    int i=0;
    for(i=0; i<num_of_users; i++){
        if(strcmp(username, users[i].username) == 0){ //if username is found in user list
            // printf("%s %s\n", username, users[i].username);
            //if limit of wrong attempts is not reached and user is not logged in
            if(users[i].failed_attempts < 3 && users[i].is_logged == 0){ 
                //if correct password send message and open message queue with PID as a key
                //else increment failed attempts counter
                if(strcmp(passwd, users[i].password) == 0){ //good passwd
                    users[i].is_logged = 1; //switch user status
                    users[i].PID = message.PID;
                    users[i].QUEUEID = msgget(message.PID, 0664 | IPC_CREAT);
                    message.type = message.PID;
                    strcpy(message.text, "1");
                    msgsnd(LOGIN_QUEUE, &message, sizeof(int) + strlen("1")+1, 0);
                    printf("LOGGED IN %s %s\n", users[i].username, users[i].password);
                    
                
                    
                    
                } else{ //bad passwd
                    message.type = message.PID;
                    strcpy(message.text, "0");
                    msgsnd(LOGIN_QUEUE, &message, sizeof(int) + strlen("0")+1, 0);
                    users[i].failed_attempts++;
                    printf("Bad password\n");
                    
                }
            } else { //failed attempts max or user logged in
                if(users[i].failed_attempts >= 3){ //limit login attempts
                    message.type = message.PID;
                    strcpy(message.text, "2");
                    msgsnd(LOGIN_QUEUE, &message, sizeof(int) + strlen("2")+1, 0);
                    printf("Reached the limit of failed attempts, user is now blocked\n");
                    
                }
                else if (users[i].is_logged == 1){ //user logged in
                    message.type = message.PID;
                    strcpy(message.text, "3");
                    msgsnd(LOGIN_QUEUE, &message, sizeof(int) + strlen("3")+1, 0);
                    printf("User is already logged in another process\n");
                    
                }
            }
            return;
        }
    }
    
    message.type = message.PID;
    strcpy(message.text, "4");
    msgsnd(LOGIN_QUEUE, &message, sizeof(int) + strlen("4")+1, 0);
    printf("Username not found!\n");
    return;
}

void handleLogOut(struct msgbuf message, struct user *user){
    user->is_logged = 0;
    user->PID = 0;
    user->failed_attempts = 0;
    message.type = 2;
    msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("1")+1, 0);
    // msgctl(user->QUEUEID, IPC_RMID, NULL);
    user->QUEUEID = -1;
    printf("Logged out %s\n", user->username);
}

void handleJoinGroup(struct msgbuf message, struct user *user, struct group groups[]){
    //message.PID is the id of the group
    //if group exists
    if(message.PID >=0 && message.PID <=2){
        for(int g=0; g<3; g++){
            if(user->groups[g] == message.PID){
                message.type = 4;
                strcpy(message.text, "3");
                printf("User already in that group\n");
                msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
                return;
            }
        }
        for(int c=0; c<3; c++){
            //if group is not full
            
            if(groups[c].id == message.PID && groups[c].members<=8){
                for(int i=0; i<9; i++){
                    //find right spot in an array
                    if (groups[c].members_PID[i] == -1){
                        groups[c].members_PID[i] = user->PID;
                        groups[c].members++; 
                        for(int k=0; k<3; k++){
                            if(user->groups[k] == -1){
                                user->groups[k] = message.PID;
                                break;
                            }
                        }
                        
                        message.type = 4;
                        strcpy(message.text, "1");
                        //added to group
                        printf("Added %s to group ID: %d\n", user->username, message.PID);
                        msgsnd(user->QUEUEID, &message, sizeof(int) +strlen("1")+1, 0);
                        return;
                    } 
                }
            }             
        }
        //group is full
        strcpy(message.text, "0");
        message.type = 4;
        printf("Group is full\n");
        msgsnd(user->QUEUEID,&message, sizeof(int)+strlen("0")+1, 0);
        return;
    }
    //group not found
    strcpy(message.text, "2");
    message.type = 4;
    printf("Group not found\n");
    msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("0")+1, 0);
    return;

}

void handleLeaveGroup(struct msgbuf message, struct user *user, struct group groups[]){
    //message.PID is group ID
    if(message.PID >=0 && message.PID <=2){
        for(int c=0; c<3; c++){
            if(groups[c].id == message.PID){
                for(int i=0; i<9; i++){
                    //find right spot in an array
                    if (groups[c].members_PID[i] == user->PID){
                        groups[c].members_PID[i] = -1;
                        groups[c].members--; 
                        for(int k=0; k<3; k++){
                            if(user->groups[k] == message.PID){
                                user->groups[k] = -1;
                                break;
                            }
                        }
                        
                        message.type = 6;
                        strcpy(message.text, "1");
                        //deleted from group
                        printf("User %s deleted from group ID: %d\n", user->username, message.PID);
                        msgsnd(user->QUEUEID, &message, sizeof(int) +strlen("1")+1, 0);
                        return;
                    } 
                }
                // you are not in that group
                message.type = 6;
                strcpy(message.text, "0");
                printf("You are not in that group\n");
                msgsnd(user->QUEUEID, &message, sizeof(int) +strlen("1")+1, 0);
                return;
            } 

            

        }
    }
    //group not found
    strcpy(message.text, "2");
    message.type = 6;
    printf("Group not found\n");
    msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("0")+1, 0);
    return;
}

void handleSendMessage(struct msgbuf message, struct user *user, struct group groups[], struct user users[]){
    //PROGRAM ASSUMES THAT USER CANT CHOOSE GROUP WHICH HE IS NOT A MEMBER OF
    if(message.PID == user->PID){ // SENDING TO YOURSELF
        message.type = 8;
        strcpy(message.text, "4");
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("4")+1, 0);
        printf("Sender can't send message to himself\n");

        return;
    }

    if(message.PID >= 0){
        //SEND TO GROUP OR USER
        if(message.PID < 3){ //GROUP
            int sent_messages = 0;
            for(int u=0; u<9; u++){ //for users
                for(int g=0; g<3; g++){ //for groups in user struct
                    if(users[u].groups[g] == message.PID && users[u].is_logged == 1){ //IF USER IN THAT GROUP AND IS LOGGED IN SEND MESSAGE
                        struct msgbuf copiedmsg;
                        copiedmsg.type = 9;
                        strcpy(copiedmsg.text, message.text);
                        copiedmsg.PID = user->PID;
                        msgsnd(users[u].QUEUEID, &copiedmsg, sizeof(int)+strlen(copiedmsg.text)+1, 0);
                        sent_messages++;
                    }
                }
            }
            // CONFIRMATION TO SENDER
            if(sent_messages == 0){ // NO MESSAGES SENT
                message.type = 8;
                strcpy(message.text, "3");
                msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("3")+1, 0);
                printf("Sender is the only member or other members are offline\n");
                return;
            } else{
                message.type = 8;
                strcpy(message.text, "1");
                msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("1")+1, 0);
                printf("Message send correctly to online groupID %d users\n",message.PID);
                return;
            }

        } else{ //USER
            for(int u=0; u<9; u++){
                if(users[u].PID == message.PID){
                    if(users[u].is_logged == 1){ //IF LOGGED IN SEND MESSAGE
                        //COPY MSG AND SEND IT TO RECIEVER
                        struct msgbuf copiedmsg;
                        copiedmsg.type = 9;
                        strcpy(copiedmsg.text, message.text);
                        copiedmsg.PID = user->PID;
                        msgsnd(users[u].QUEUEID, &copiedmsg, sizeof(int)+strlen(copiedmsg.text)+1, 0);
                        //SEND CONFIRMATION TO SENDER
                        message.type = 8;
                        strcpy(message.text, "1");
                        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("1")+1, 0);
                        printf("Message send correctly from %s to %s\n", user->username, users[u].username);

                    } else{ //RECIEVER NOT LOGGED IN
                        message.type = 8;
                        strcpy(message.text, "2");
                        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("2")+1, 0);
                        printf("Reciever is not logged in, message not send");
                    }
                    return;
                }
            }
            //DIDNT FIND USER
            printf("Wrong PID or group ID\n");
            message.type = 8;
            strcpy(message.text, "0");
            msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("0")+1, 0);
            return;
        }
    } else{ // WRONG PID
        printf("Wrong PID or group ID\n");
        message.type = 8;
        strcpy(message.text, "0");
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("0")+1, 0);
        return;
    }
}


int main(){  
    //open users file and fill array with users
    struct user users[9];
    char filename[] = "user_list";
    openFileAndFillUserList(filename, users);
    int num_of_users = sizeof(users)/sizeof(users[0]);
    int ACTIVE_USERS_COUNTER=0;
    // printAllUsers(users, num_of_users);
    // printAllUsersInfo(users, num_of_users);

    // open groups file and fill array with groups
    struct group groups[3];
    openFileAndFillGroups(groups, "topic_groups");
    int num_of_groups = sizeof(groups)/ sizeof(groups[0]);
    // printAllGroups(groups, num_of_groups);

    struct msgbuf message;    
    int LOGIN_QUEUE = msgget(9000, 0664 | IPC_CREAT);  
    // msgrcv(LOGIN_QUEUE, &message, sizeof(int)+1024, 1, 0);


    int run = 1;
    //MAIN LOOP
    while(run){
        //LOGIN ATTEMPT
        if(msgrcv(LOGIN_QUEUE, &message, sizeof(int)+1024, 1, IPC_NOWAIT) != -1){
            printf("Recieved login request on %s\n", message.text);
            handleLogIn(LOGIN_QUEUE, num_of_users, message, users, &ACTIVE_USERS_COUNTER);
            // for(int x=0; x<num_of_users; x++){
            //     if(users[x].is_logged == 1){
            //         printf("%s\n", users[x].username);
            //     }
            // }
            printf("\n");
        }

        //LOOK FOR REQUESTS FROM ACTIVE USERS
        for(int x=0; x<num_of_users; x++){
            if (users[x].is_logged == 1){
                //LOGOUT
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 2, IPC_NOWAIT) != -1){
                    printf("Recieved logout request from %s\n", users[x].username);
                    handleLogOut(message, &users[x]);
                    printf("\n");

                }      
                //JOIN GROUP
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 3, IPC_NOWAIT) != -1){
                    printf("Recieved join group request from %s\n", users[x].username);
                    run=0;
                    break;
                    handleJoinGroup(message, &users[x], groups);
                    // printAllGroups(groups, num_of_groups);
                    // printAllUsersInfo(users, num_of_users);
                    printf("\n");
                }
                //LEAVE FROM GROUP
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 5, IPC_NOWAIT) != -1){
                    printf("Recieved leave group request from %s\n", users[x].username);
                    handleLeaveGroup(message, &users[x], groups);
                    // printAllGroups(groups, num_of_groups);
                    printf("\n");
                
                }

                // msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 7, 0);
                // printf("Recieved send message request from %s\n", users[x].username);
               
            }
        }
    }
    // printAllGroups(groups, num_of_groups);
    // printAllUsersInfo(users, num_of_users);

    //TESTING PLAYGROUND    
    // for(int x=0; x<num_of_users; x++){
    //     // ACTIVE_USERS[x] = NULL;
    //     ACTIVE_USERS[x] = &users[x];
    //     printf("%s\n", ACTIVE_USERS[x]->username);
    // }
    // printAllUsersInfo(users, num_of_users);
    // for(int x=0; x<num_of_users; x++){
    //     ACTIVE_USERS[x] = NULL;
    //     if(ACTIVE_USERS[x]==NULL){
    //         printf("TEST\n");
    //     }
    //     // ACTIVE_USERS[x] = &users[x];
    //     printf("%s\n", ACTIVE_USERS[x]->username);
    // }
    // printAllUsersInfo(users, num_of_users);

    // for(int xd=0; xd<15; xd++){
    //     QUEUES[xd]=xd;
    // }
    // test(&QUEUES_COUNTER, QUEUES);
    // for(int xd=0; xd<15; xd++){
    //     printf("%d\n", QUEUES[xd]);
    // }


}