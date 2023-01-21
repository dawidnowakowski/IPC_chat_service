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
    // 1 - log in attempt
    // 2 - server's respond to log in attempt   
    //     text: 1-logged in,
    //           0-bad password
    //           2-blocked access-reached limit of failed attempts
    //           3-user already logged in
    //           4-username not found
    // USER QUEUE
    // 3 - request join group
    // 4 - server's respond for join group
    //     text: 1 - added to group
    //           0 - group is full, sorry;
    //           2 - group not found 
    // 6 - server's respond for join group
    //           1 - deleted from group
    //           0 - you are not in this group
    //           2 - group not found
    long type; 
    int PID;
    char text[1024];
};

int main(){
    int LOGIN_QUEUE = msgget(9000, 0664 | IPC_CREAT);   
    int PID = getpid();
    struct msgbuf login_message;
    char credits[] = "test5 passwd5";
    // char credits[] = "test3 passwd3";
    strcpy(login_message.text, credits);
    login_message.PID = PID;
    login_message.type = 1;

    msgsnd(LOGIN_QUEUE, &login_message, sizeof(int) + strlen(credits)+1, 0);
    msgrcv(LOGIN_QUEUE, &login_message, sizeof(int)+1024, PID, 0);
    printf("PID:%d text:%s\n", login_message.PID, login_message.text);

    if(strcmp(login_message.text, "1")==0){
        int MY_QUEUE = msgget(PID, 0664 | IPC_CREAT);
        
        // logout
        // login_message.type = 2;
        // msgsnd(MY_QUEUE, &login_message, sizeof(int) + strlen(credits)+1, 0);
        // printf("wyslano logout\n");

        //join group
        login_message.type = 3;
        login_message.PID = 0;
        msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
        msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 4, 0);
        printf("JOINrespond from server: %s\n", login_message.text);

        login_message.type = 5;
        login_message.PID = 0;
        msgsnd(MY_QUEUE, &login_message, sizeof(int)+strlen(login_message.text)+1, 0);
        msgrcv(MY_QUEUE, &login_message, sizeof(int)+1024, 6, 0);
        printf("DELETErespond from server: %s\n", login_message.text);
    }

 






    // USER INTERFACE   
    // int user_input=0;
    // while(user_input!=10 && user_input!=7){
    //     clear();
    //     printf("Co chcesz zrobić?\n1-zalogowac się\n2-wylogowac sie\n3-zamknac program\n");
    //     scanf("%d",&user_input);
    //     switch(user_input){
    //         case 1:;
    //             clear();
    //             char login[200];
    //             char password[20];
    //             printf("Podaj login:");
    //             scanf("%s",login);
    //             printf("Podaj haslo:");
    //             scanf("%s", password);                
    //             //Kolejka                

    //             user_input=7;
    //             break;
    //         case 2:;
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 3:;                
    //             clear();
    //             user_input=10;
    //             break;
    //     }
    // }
    // while(user_input!=10){
    //     clear();
    //     printf("Co chcesz zrobić?\n1-wyslac wiadomosc do użytkownika\n2-sprawdzic liste zalogowanych uzytkownikow\n3-wyslac wiadomosc do grupy\n4-zapisac sie do grupy\n5-wypisac sie z grupy\n6-sprawdzic liste dostepnych grup\n7-odebrac wiadomosc\n8-wylogowac sie\n9-zamknac program\n");
    //     scanf("%d",&user_input);
    //     switch(user_input){
    //         case 1:;
    //             clear();
    //             char user[20];
    //             char message[1024];
    //             printf("Podaj nazwę użytkownika do którego chcesz wysłać wiadomość: ");
    //             scanf("%s", user);
    //             printf("Podaj wiadomość: ");
    //             scanf(" %[^\n]",message);
    //             printf("%s",message);
    //             //Kolejka
    //             break;
    //         case 2:;
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 3:;
    //             clear();
    //             char group[20];
    //             char gmessage[1024];
    //             printf("Podaj nazwę grupy do której chcesz wysłać wiadomość: ");
    //             scanf("%s", group);
    //             printf("Podaj wiadomość: ");
    //             scanf(" %[^\n]",gmessage);
    //             printf("%s",gmessage);
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 4:;
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 5:;
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 6:;
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 7:;
    //             clear();
    //             //Kolejka                
    //             break;
    //         case 8:;      
    //             clear();                           
    //             break;
    //         case 9:;
    //             clear();
    //             user_input=10;
    //     }
    // }
}