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

void handleLogIn(int LOGIN_QUEUE, int num_of_users, struct msgbuf message, struct user users[], int *ACTIVE_USERS_COUNTER){
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
                    *ACTIVE_USERS_COUNTER += 1;
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

void handleLogOut(struct msgbuf message, struct user *user, int *ACTIVE_USERS_COUNTER){
    *ACTIVE_USERS_COUNTER -= 1;
    user->is_logged = 0;
    user->PID = 0;
    user->failed_attempts = 0;
    message.type = 2;
    strcpy(message.text,"1");
    msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("1")+1, 0);
    msgctl(user->QUEUEID, IPC_RMID, NULL);
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

void handleSendMessage(struct msgbuf message, struct user *user, struct group groups[], struct user users[], int num_of_groups, int num_of_users){
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
            // printf("TO WHO? %d\n", message.PID);
            // printf("num of groups: %d\n", num_of_groups);
            //CHECK IF SENDER IN THAT GROUP
            int in_group = 0;
            for(int g=0; g<num_of_groups; g++){
                // printf("%d = %d\n", groups[g].id, message.PID);
                if(groups[g].id == message.PID){
                    
                    for(int u=0; u<num_of_users; u++){
                        // printf("%s %d, %d\n", groups[g].groupname, groups[g].members_PID[u], user->PID);
                        if(groups[g].members_PID[u] == user->PID){
                            
                            in_group = 1;
                        }
                    }
                    break;
                }
            }
            
            if(in_group == 0){
                printf("You are not in that group\n");
                message.type = 8;
                strcpy(message.text, "5");
                msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("5")+1, 0);
                return;
            }
            
            int sent_messages = 0;
            for(int u=0; u<9; u++){ //for users
                for(int g=0; g<3; g++){ //for groups in user struct
                    if(users[u].groups[g] == message.PID && users[u].is_logged == 1 && users[u].PID!=user->PID){ //IF USER IN THAT GROUP AND IS LOGGED IN SEND MESSAGE
                        struct msgbuf copiedmsg;
                        copiedmsg.type = 9;
                        strcpy(copiedmsg.text, message.text);
                        copiedmsg.PID = message.PID;
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

void handleSendLoggedUsersList(struct msgbuf message, struct user *user, struct user users[], int ACTIVE_USERS_COUNTER){
    printf("Active users: %d\n", ACTIVE_USERS_COUNTER);
    if(ACTIVE_USERS_COUNTER == 1){
        strcpy(message.text, "0");
        message.type = 11;
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen("0")+1, 0);
        printf("There are no other active users\n");
        return;
    } else{
        char respond[1024] = "";

        int i;
        char temp_PID[5];
        for(int u=0; u<9; u++){
            if(users[u].is_logged == 1 && users[u].PID != user->PID){
                i = u;
                sprintf(temp_PID, "%d", users[i].PID);
                strcat(respond, users[u].username);
                strcat(respond, " ");
                strcat(respond, temp_PID);
                strcat(respond, "\n");
            }
        }
        strcpy(message.text, respond);
        message.type = 11;
        message.PID = ACTIVE_USERS_COUNTER - 1;
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        printf("List of active users sent\n");
        return;
    }
}

void handleSendUsersOfGroup(struct msgbuf message, struct user *user, struct group groups[], struct user users[], int num_of_users){
    //ASSUME GROUP EXISTS (USER SITE VERIFICATION)
    int membs = 0;
    char respond[1024];
    respond[0]='\0';
    for(int u=0; u<num_of_users; u++){
        for(int g=0; g<3; g++){
            if (users[u].groups[g] == message.PID){
                strcat(respond, users[u].username);
                strcat(respond, "\n");
                membs++;
            }
        }
    } 
    if(membs == 0){
        message.type = 13;
        strcpy(message.text, "0");
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        printf("There are no users in that group\n");
        return;
    } else{
        message.type = 13;
        strcpy(message.text, respond);
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        printf("List of users in group sent\n");
        return;
    }

}

void handleSendListOfAvailableGroups(struct msgbuf message, struct user *user, struct group groups[], int num_of_groups, int num_of_users){
    char respond[1024]="";
    for(int g=0; g<num_of_groups; g++){
        int found=0;
        for(int u=0; u<num_of_users; u++){
            if(groups[g].members_PID[u] == user->PID){
                found=1;
            }
        }
        if(found == 0){
            int temp=g+1;
            char number[5];
            sprintf(number, "%d. ", temp);   
            strcat(respond, number);
            strcat(respond, groups[g].groupname);
            strcat(respond, "\n");
        }
    }
    if(strcmp(respond, "") == 0){
        printf("There are no more available groups for this user\n");
        message.type = 15;
        strcpy(message.text, "0");
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        return;
    } else{
        message.type = 15;
        strcpy(message.text, respond);
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        printf("List of available groups sent\n");
        return;
    }
}

void handleSendListOfAllGroups(struct msgbuf message, struct user *user, struct group groups[], int num_of_groups){
    char respond[1024]="";
    for(int g=0; g<num_of_groups; g++){  
        int temp=g+1;
        char number[5];
        sprintf(number, "%d. ", temp);   
        strcat(respond, number);
        strcat(respond, groups[g].groupname);
        strcat(respond, "\n");
    }

    message.type = 17;
    strcpy(message.text, respond);
    msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
    printf("List of available groups sent\n");
    return;
    
}
void handleSendListOfUserGroups(struct msgbuf message, struct user *user, struct group groups[], int num_of_groups, int num_of_users){
    char respond[1024]="";
    for(int g=0; g<num_of_groups; g++){
        int found=0;
        for(int u=0; u<num_of_users; u++){
            if(groups[g].members_PID[u] == user->PID){
                found=1;
            }
        }
        if(found == 1){
            int temp=g+1;
            char number[5];
            sprintf(number, "%d. ", temp);   
            strcat(respond, number);
            strcat(respond, groups[g].groupname);
            strcat(respond, "\n");
        }
    }
    if(strcmp(respond, "") == 0){
        printf("You are not in any of groups\n");
        message.type = 19;
        strcpy(message.text, "0");
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        return;
    } else{
        message.type = 19;
        strcpy(message.text, respond);
        msgsnd(user->QUEUEID, &message, sizeof(int)+strlen(message.text)+1, 0);
        printf("List of user's groups sent\n");
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
            // printf("ACTIVE USERS: %d\n",ACTIVE_USERS_COUNTER);
            handleLogIn(LOGIN_QUEUE, num_of_users, message, users, &ACTIVE_USERS_COUNTER);
            // printf("ACTIVE USERS: %d\n",ACTIVE_USERS_COUNTER);
         
            printf("\n");
        }

        //LOOK FOR REQUESTS FROM ACTIVE USERS
        for(int x=0; x<num_of_users; x++){
            if (users[x].is_logged == 1){
                //LOGOUT
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 1, IPC_NOWAIT) != -1){
                    printf("Recieved logout request from %s\n", users[x].username);
                    // printf("ACTIVE USERS: %d\n",ACTIVE_USERS_COUNTER);
                    handleLogOut(message, &users[x], &ACTIVE_USERS_COUNTER);
                    // printf("ACTIVE USERS: %d\n",ACTIVE_USERS_COUNTER);
                    printf("\n");

                }      
                //JOIN GROUP
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 3, IPC_NOWAIT) != -1){
                    printf("Recieved join group request from %s\n", users[x].username);
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
                //FORWARD MESSAGE
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 7, IPC_NOWAIT) != -1){
                    printf("Recieved send message request from %s\n", users[x].username);
                    handleSendMessage(message, &users[x], groups, users, num_of_groups, num_of_users);
                    printf("\n");
                }
                //REQUEST USERS LIST
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 10, IPC_NOWAIT) != -1){
                    // printAllUsersInfo(users, num_of_users);
                    printf("Recieved send user list request from %s\n", users[x].username);
                    handleSendLoggedUsersList(message, &users[x], users, ACTIVE_USERS_COUNTER);
                    printf("\n");
                }
                //REQUEST LIST OF USERS IN GROUP
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 12, IPC_NOWAIT) != -1){
                    printf("Recieved send users of group request from %s\n", users[x].username);
                    handleSendUsersOfGroup(message, &users[x], groups, users, num_of_users);
                    printf("\n");
                }
                //REQUEST LIST OF AVAILABLE GROUPS
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 14, IPC_NOWAIT) != -1){
                    printf("Recieved send available groups request from %s\n", users[x].username);
                    handleSendListOfAvailableGroups(message, &users[x], groups, num_of_groups, num_of_users);
                    printf("\n");
                }
                //REQUEST LIST OF ALL GROUPS
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 16, IPC_NOWAIT) != -1){
                    printf("Recieved send list of all groups request from %s\n", users[x].username);
                    handleSendListOfAllGroups(message, &users[x], groups, num_of_groups);
                    printf("\n");
                }
                //REQUEST LIST OF USER'S GROUPS
                if(msgrcv(users[x].QUEUEID, &message, sizeof(int)+1024, 18, IPC_NOWAIT) != -1){
                    printf("Recieved send list of user's groups request from %s\n", users[x].username);
                    handleSendListOfUserGroups(message, &users[x], groups, num_of_groups, num_of_users);
                    printf("\n");
                }
            }
        }
    }
    // printAllGroups(groups, num_of_groups);
    // printAllUsersInfo(users, num_of_users);


}