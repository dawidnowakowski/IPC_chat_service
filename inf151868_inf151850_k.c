#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define clear() printf("\033[H\033[J")

struct msgbuf
{
 // LOGIN QUEUE
    //  1 - log in attempt
    //  2 - server's respond to log in attempt   
    //      text: 1-logged in,
    //           0-bad password
    //           2-blocked access-reached limit of failed attempts
    //           3-user already logged in
    //           4-username not found
    // USER QUEUE
    //  1 - request logout
    //  2 - server's respond for logout
    //      text: 1 - logged out
    //            0 - something went wrong
    //  3 - request join group
    //  4 - server's respond for join group
    //      text: 1 - added to group
    //            0 - group is full, sorry;
    //            2 - group not found 
    //  5 - request  to leave from group
    //  6 - server's respond for join group
    //      text: 1 - deleted from group
    //            0 - you are not in this group
    //            2 - group not found
    //  7 - request to send message
    //  8 - server's respond to sending message
    //      text: 1 - message sent
    //            0 - wrong PID
    //            2 - Reciever is not logged in
    //            3 - Sender is the only user in group or group has no other members
    //            4 - Sender can't send message to himself
    //  9 - recieve message (user site only, messages from users has type=9)
    // 10 - request for online user list
    // 11 - server's respond with user list
    //      text: 0 - no other active users
    //            else: PID = num of active users
    //                  text format "username PID\n"
    // 12 - request for list of users in group 
    // 13 - server's respond with users of group list
    //      text: 0 - no users in that group
    //            else: text format "groupname\n"
    // 14 - request for available groups
    // 15 - server's respond with available groups
    //      text: 0 - no available groups
    //            else: text format "groupname\n"
    // 16 - request for ALL groups
    // 17 - server's respond with ALL groups
    //      text: all groups in format "groupname\n"
    long type; 
    int PID;
    char text[1024];
};

struct active_users
{
    char username[10];
    char pid[10];
};

void get_activeUsers(struct active_users users[],struct msgbuf login_message){
    char buf, usernamebuf[10],pidbuf[10];
    int i=0, user_or_pid=0,counter=0;
    for(int x=0;x<strlen(login_message.text);x++){
        buf=login_message.text[x];
        if(buf != ' ' && buf != '\n'){ 
            if (user_or_pid % 2 == 0){
                usernamebuf[i] = buf;
                i++;
            } 
            else{
                pidbuf[i] = buf;
                i++;            
            }                   
        }
        if(buf == ' ' || buf == '\n'){
            if(buf == ' '){
                    usernamebuf[i]='\0';
                    strcpy(users[counter].username, usernamebuf);
                    usernamebuf[0] = '\0';
            }else{
                strcpy(users[counter].pid, pidbuf);
                pidbuf[0] = '\0';
            }

            i=0;
            user_or_pid++;
            if(user_or_pid % 2 == 0){
                counter++;
            }
        }
    }
    strcpy(users[counter].pid, pidbuf);
}

int main(){
    int LOGIN_QUEUE = msgget(9000, 0664 | IPC_CREAT);   
    int PID = getpid();
    struct msgbuf login_message; 

    // USER INTERFACE   

    int user_input=0;
    while(user_input!=-1 && user_input!=7){
        clear();
        printf("What would you like to do?\n1-Log in.\n2-Close the program.\n");
        scanf("%d",&user_input);
        switch(user_input){
            case 1:;
                char response[1024];
                strcpy(response,"-1");
                while(strcmp(response,"1")!=0){ 
                    clear();
                    if(strcmp(response,"0")==0){
                        printf("Wrong password!\n");
                    }
                    else if(strcmp(response,"2")==0){
                        printf("Access denied! Limit of failed attempts reached!\n");
                        user_input=-1;
                        break;
                    }
                    else if(strcmp(response,"3")==0){
                        printf("This user is already logged in!\n");
                    }
                    else if(strcmp(response,"4")==0){
                        printf("Username not found!\n");
                    }
                    char login[200];
                    printf("Please enter login and password separated by a space: \n");
                    scanf(" %[^\n]",login);
                    strcpy(login_message.text, login);
                    login_message.PID = PID;
                    login_message.type = 1; 
                    msgsnd(LOGIN_QUEUE, &login_message, sizeof(int) + strlen(login)+1, 0);
                    msgrcv(LOGIN_QUEUE, &login_message, sizeof(int)+1024, PID, 0);
                    strcpy(response,login_message.text);                       
                }
                if(strcmp(response,"2")!=0){
                printf("You logged in succesfully\n");
                sleep(1);
                user_input=7;                
                }
                break;
            case 2:;
                clear();
                user_input=-1;
                break;
        }
    }

    int MY_QUEUE = msgget(PID, 0664 | IPC_CREAT);

    while(user_input!=-1){
        int selected_group,n;
        clear();
        printf("What would you like to do?\n1-Send a message to the user.\n2-Check the list of logged in users.\n3-Send a message to the group.\n4-Sign up for the group.\n5-Leave the group.\n6-Check the list of available groups.\n7-Check the list of all groups.\n8-Check users of the group.\n9-Recive one message.\n10-Log out.\n11-Close the program.\n");
        scanf("%d",&user_input);
        switch(user_input){
            case 1:;
                clear();
                int selected_user=-1;
                char message[1024];
                n=0;

                login_message.type = 10;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 11, 0);
                if(strcmp(login_message.text, "0") == 0){
                    printf("There aren't any other active users.\n");
                    sleep(2);                                 
                }
                else{             
                    for(int x=0;x<strlen(login_message.text);x++){
                        if(login_message.text[x]=='\n'){
                            n++;
                        }
                    }
                    struct active_users users[n]; 
                    get_activeUsers(users,login_message);
                    printf("Active users:\n");
                    for(int x=0;x<n;x++){
                        printf("%d. %s\n",x+1,users[x].username);
                    }  

                    printf("Enter userID: \n");
                    scanf("%d", &selected_user);
                    if(selected_user>0 && selected_user<=n){
                        login_message.type = 7;
                        login_message.PID = atoi(users[selected_user-1].pid);
                        printf("Enter message: \n");
                        scanf(" %[^\n]",message);
                        strcpy(login_message.text,message);
                        msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                        msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 8, 0);
                        if(strcmp(login_message.text, "1") == 0){
                            printf("Message sent correctly.\n");
                        }
                        else if (strcmp(login_message.text, "2") == 0){
                            printf("Reciever is no longer logged in\n");
                        }
                        else{
                            printf("Something went wrong.\n");
                        }
                    }
                    else{
                        printf("This user doesn't exist.\n");
                    }
                    sleep(2);
                }
                break;
            case 2:;
                clear();
                login_message.type = 10;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 11, 0);
                if(strcmp(login_message.text, "0") == 0){
                    printf("There aren't any other active users.\n");                                   
                }
                else{   
                    n=0;           
                    for(int x=0;x<strlen(login_message.text);x++){
                        if(login_message.text[x]=='\n'){
                            n++;
                        }
                    }
                    struct active_users users[n]; 
                    get_activeUsers(users,login_message);
                    printf("Active users:\n");
                    for(int x=0;x<n;x++){
                        printf("%d. %s\n",x+1,users[x].username);
                    } 
                }
                sleep(3);              
                break;
            case 3:;
                clear();
                n=0;
                login_message.type = 18;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 19, 0);
                if(strcmp(login_message.text, "0") == 0){
                    printf("You are not in any group.\n");
                }
                else{               
                    for(int x=0;x<strlen(login_message.text);x++){
                        if(login_message.text[x]=='\n'){
                            n++;
                        }
                    }
                    printf("Available groups:\n%s \n", login_message.text);
                    printf("Enter group ID: \n");
                    scanf("%d", &selected_group);
                    

                    if(selected_group>0 && selected_group<=n){
                        login_message.type = 7;
                        login_message.PID = selected_group-1;
                        char gmessage[1024];
                        printf("Enter message: \n");
                        scanf(" %[^\n]",gmessage);
                        strcpy(login_message.text,gmessage);
                        msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                        msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 8, 0);
                        if(strcmp(login_message.text, "1") == 0){
                            printf("Message sent correctly.\n");
                        }
                        else if (strcmp(login_message.text, "2") == 0){
                            printf("Reciever is no longer logged in.\n");
                        }
                        else if (strcmp(login_message.text, "3") == 0){
                            printf("You are the only person in that group or other members are no longer logged in.\n");
                        }
                        else{
                            printf("Something went wrong.\n");
                        }
                    }
                    else{
                        printf("This group doesn't exist.\n");
                    }
                    sleep(2);                    
                }             
                break;
            case 4:;
                clear();
                printf("Which group would you like to join?\n");
                login_message.type = 16;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 17, 0);
                printf("%s \n", login_message.text);
                scanf("%d",&selected_group);
                login_message.type = 3;
                login_message.PID = selected_group-1;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 4, 0);
                if(strcmp(login_message.text, "1") == 0){
                    printf("You joined in succesfully.\n");
                    sleep(1);
                }
                else if(strcmp(login_message.text, "0") == 0){
                    printf("That group is full, sorry.\n");
                    sleep(1);
                }
                else if(strcmp(login_message.text, "2") == 0){
                    printf("There is no such group.\n");
                    sleep(1);
                }
                else if(strcmp(login_message.text, "3") == 0){
                    printf("You already in this group.\n");
                    sleep(1);;
                }             
                break;
            case 5:;
                clear();                
                printf("Which group would you like to leave?\n");
                login_message.type = 16;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 17, 0);
                printf("%s \n", login_message.text);
                scanf("%d",&selected_group);
                login_message.type = 5;
                login_message.PID = selected_group-1;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 6, 0);
                if(strcmp(login_message.text, "1") == 0){
                    printf("You have left succesfully.\n");
                    sleep(1);
                }
                else if(strcmp(login_message.text, "0") == 0){
                    printf("You are not in this group.\n");
                    sleep(1);
                }
                else if(strcmp(login_message.text, "2") == 0){
                    printf("There is no such group.\n");
                    sleep(1);
                }               
                break;
            case 6:;
                clear();
                login_message.type = 14;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 15, 0);
                if(strcmp(login_message.text, "0") == 0){
                    printf("There is no available groups.\n");
                }
                else{
                    printf("Available groups:\n%s \n", login_message.text);
                }
                sleep(2);             
                break;
            case 7:;
                clear();
                login_message.type = 16;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 17, 0);
                printf("All groups:\n%s \n", login_message.text);
                sleep(3);
                break;
            case 8:;
                clear();               
                printf("Users of which group would you like check?\n");
                login_message.type = 16;
                msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 17, 0);
                printf("All groups:\n%s \n", login_message.text);
                scanf("%d",&selected_group);
                if(selected_group>0 && selected_group<4){   
                    login_message.type = 12;
                    login_message.PID = selected_group-1;
                    msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
                    msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 13, 0);
                    if(strcmp(login_message.text, "0") == 0){
                        printf("There is no users in that group.\n");
                    }
                    else{
                        printf("Group users:\n%s \n", login_message.text);
                    }
                }
                else{
                    printf("There is no group with that id.\n");
                }
                sleep(3);
                break;
            case 9:;
                clear();
                if(msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 9, IPC_NOWAIT)==-1){
                    printf("There is no message to recive.\n");                    
                }
                else{
                    if(login_message.PID<3){
                        printf("Message from group %d: %s\n",login_message.PID+1,login_message.text); 
                    } 
                    else{
                        printf("Message from user with PID %d: %s\n",login_message.PID,login_message.text); 
                    }
                }
                sleep(3);          
                break;
            case 10:;      
                clear();    
                login_message.type = 2;
                msgsnd(MY_QUEUE, &login_message, sizeof(int) + strlen(login_message.text)+1, 0);
                msgrcv(MY_QUEUE, &login_message, sizeof(int) + 1024, 2, 0);
                if(strcmp(login_message.text, "1") == 0){
                    printf("Logged out succesfully.\n");
                    msgctl(MY_QUEUE, IPC_RMID, NULL);
                    user_input=-1;
                }
                else{
                    printf("Somthing went wrong.\n");
                    }                         
                break;
            case 11:;
                clear();
                user_input=-1;
        }
    }
}
