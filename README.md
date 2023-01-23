# IPC_chat_service
aby zkompilować pliki .c należy użyć komendy 
make all

do wyczyszczenia plików po kompilacji należy użyć komendy
make clear

aby uruchomić program użytkownika w kilku terminalach odpalamy usera komendą
./client

aby uruchomić program serwera używamy komendy
./server

w jednym folderze z plikiem server.o musza znajdowac sie pliki topic_groups oraz user_list. Użyte struktury znajdują się w protokole.


Użytkownik obsługiwany jest za pomocą interaktywnego menu, które przy użyciu funckji wysyła wiadomości do kolejki komunikatów. Na te komunikaty odpowiada w odpowiedni sposób serwer i wysyła informacje zwrotną. Możliwości klienta są opisane szczegółowo w protokole. Tak samo jak cały protokół komunikacji między użytkownikami a serwerem.


void openFileAndFillUserList(char filename[], struct user users[]) 

– otwiera plik oraz wypełnia danymi z pliku tablice users[] 

 

void openFileAndFillGroups(struct group groups[], char filename[]) 

– otwiera plik oraz wypełnia danymi z pliku tablice groups[] 

 

void handleLogIn(int LOGIN_QUEUE, int num_of_users, struct msgbuf message, struct user users[], int *ACTIVE_USERS_COUNTER) 

- funkcja sprawdza dane wysłane przez użytkownika, wykonuje potrzebne operacje jak dodanie go do listy użytkowników, zwiększanie licznika aktywnych użytkowników oraz wysyła informację zwrotną 

 

void handleLogOut(struct msgbuf message, struct user *user, int *ACTIVE_USERS_COUNTER) 

- funkcja wylogowuje użytkownika i “sprząta” po nim, ustawia wartości na defaultowe i wysyła informację zwrotną 

 

void handleJoinGroup(struct msgbuf message, struct user *user, struct group groups[]) 

- funkcja sprawdza poprawność danych wysłanych przez użytkownika oraz dodaje do go grupy jeśli wszystkie się zgadzają. Wysyła informację zwrotną. 

 

void handleLeaveGroup(struct msgbuf message, struct user *user, struct group groups[]) 

- funkcja wyrzuca użytkownika z grupy jeśli to możliwe i informuje go o rezultacie w informacji zwrotnej 

 

void handleSendMessage(struct msgbuf message, struct user *user, struct group groups[], struct user users[]) 

- funkcja zajmuje się przesyłaniem wiadomości do grupy lub do użytkownika. Jeśli wszystkie dane do wysyłania są poprawne, to wiadomość zostanie wysłana tylko i wyłącznie do użytkowników, którzy są zalogowani na serwerze! Wysyła informacje o rezultacie nadawcy wiadomości. 

 

void handleSendLoggedUsersList(struct msgbuf message, struct user *user, struct user users[], int ACTIVE_USERS_COUNTER) 

- funkcja wysyła do usera listę zalogowanych użytkowników 

 

void handleSendUsersOfGroup(struct msgbuf message, struct user *user, struct group groups[], struct user users[], int num_of_users) 

- funkcja wysyła do usera listę użytkowników należących do danej grupy 

 

void handleSendListOfAvailableGroups(struct msgbuf message, struct user *user, struct group groups[], int num_of_groups, int num_of_users) 

- funkcja wysyła listę grup, do których może dołączyć użytkownik 

 

void handleSendListOfAllGroups(struct msgbuf message, struct user *user, struct group groups[], int num_of_groups) 

- funkcja wysyła listę wszystkich grup 

 

void handleSendListOfUserGroups(struct msgbuf message, struct user *user, struct group groups[], int num_of_groups, int num_of_users) 

- funkcja wysyła listę grup, do których należy użytkownik 
