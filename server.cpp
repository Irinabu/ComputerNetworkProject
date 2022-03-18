#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sqlite3.h>
#include <iostream>

using namespace std;

/*
 Bibliografie:
 [1] SQLite C parameterized queries: https://zetcode.com/db/sqlitec/
 [2] Model client-server concurent in functia main : https://profs.info.uaic.ro/~gcalancea/lab7/servTcpConc.c
 [3] Parcurgere rezultat tranzactie sql : https://gist.github.com/jsok/2936764
 [4] Deschidere baza de date si compilare stmt si bind cu parametri: https://renenyffenegger.ch/notes/development/databases/SQLite/c-interface/basic/index
 */

#define PORT 2053

extern int errno;

void lastId();
int lastId_message(int id_transmitter, int id_receiver);
void registerr(char *user, char *password, int client);
int verifyExistenceOfIdUser(int id);
int verifyExistenceofIdMessage(int id, int id_transmitter, int id_receiver);
void changeInfoClient(int client, char msgrasp[], char msg[]);
void commands(int client, char comand[], int id);
void updateStatus(int id, int status);
char *searchAfterId(int id);
void returnNotifications(int id_receiver, char notifications[]);
void seeUsers(int client, int id_transmitter);
void showConversation(int client, int id_transmitter, int id_receiver);
void sendMessage(int client, int id_message, int id_transmitter, int id_receiver, char msg[], int id_reply);
void seenMessages(int id_send, int id_received);

int id_count; // voi retine id-ul ultimului user inregistrat

char msgrasp[400];
char msg[400];
sqlite3 *db;

void connect(int client) {

    bool found = false;
    sqlite3_stmt *stmt;

    char user[100];

    strcat(msgrasp, "Introdu username-ul(pentru autentificare sau inregistrare) : ");
    changeInfoClient(client, msgrasp, user);

    if (sqlite3_open("proiect", &db)) {
        printf("eroare la deschiderea bazei de date\n");
        exit(-1);
    }//[4]

    sqlite3_prepare_v2(db, "select * from Users", -1, &stmt, NULL);

    const char *pass;
    int id = 0;

    while (sqlite3_step(stmt) != SQLITE_DONE && !found) {
        const char *database_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        id = sqlite3_column_int(stmt, 0);
            if (strcmp(database_user, user) == 0) {
                found = true;
                pass = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

                strcat(msgrasp, "Am gasit utilizatorul. Introduceti parola: ");
                changeInfoClient(client, msgrasp, msg);

                while (strcmp(pass, msg)) {


                    strcat(msgrasp, "Parola incorecta(introdu iar parola sau scrie 'stop': ");
                    changeInfoClient(client, msgrasp, msg);

                    if (strcmp(msg, "stop") == 0)
                        connect(client);

                }
                if (strcmp(pass, msg) == 0) {
                    sqlite3_finalize(stmt);

                    updateStatus(id, 1);
                    strcat(msgrasp,
                           "Esti conectat.Poti face urmatoarele comenzi :- 1 vezi lista utilizatorilor; - 2 deconectare: ");

                    changeInfoClient(client, msgrasp, msg);

                    commands(client, msg, id);


                }
            }
    }//[3]


    if (!found) {

        strcat(msgrasp, "Nu am gasit utilizatorul. Doriti sa va inregistrati cu acest user? D/N : ");

        changeInfoClient(client, msgrasp, msg);

        while (strcmp(msg, "D") && strcmp(msg, "d") && strcmp(msg, "N") && strcmp(msg, "n")) {
            strcat(msgrasp, "Comanda nerecunoscuta. Introduceti D/N : ");
            changeInfoClient(client, msgrasp, msg);
        }

        if (strcmp(msg, "D") == 0 || strcmp(msg, "d") == 0) {

            char pass1[100];
            strcat(msgrasp, "Introduceti parola pentru a va inregistra: ");
            changeInfoClient(client, msgrasp, pass1);

            strcat(msgrasp, "Confirmati parola : ");
            changeInfoClient(client, msgrasp, msg);

            if (strcmp(pass1, msg) == 0) { registerr(user, msg, client); }
            else connect(client);


        } else if (strcmp(msg, "N") == 0 || strcmp(msg, "n") == 0) {
            connect(client);
        }

        exit(2);

    }


}

void updateStatus(int id, int status) {

    sqlite3_stmt *stmt1;
    string sql = "update Users set Status=? where Id_user=?";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt1, NULL);

    if (stmt1 != NULL) {

        sqlite3_bind_int(stmt1, 1, status);
        sqlite3_bind_int(stmt1, 2, id);
        sqlite3_step(stmt1);
        sqlite3_finalize(stmt1);

    }

}

// va prelua ultimul id inregistrat in baza, folosit in continuare pt a nu inregistra un alt user cu acealsi id
void lastId() {

    if (sqlite3_open("proiect", &db)) {
        printf("eroare la deschiderea bazei de date\n");
        exit(-1);
    }

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "select Id_user from Users", -1, &stmt, NULL);


    while (sqlite3_step(stmt) != SQLITE_DONE) {

        id_count = sqlite3_column_int(stmt, 0);

    }//[3]
    sqlite3_finalize(stmt);


}

int lastId_message(int id_transmitter, int id_receiver) {

    int last_message = 0; //voi retine id-ul ultimului mesaj inregistrat

    if (sqlite3_open("proiect", &db)) {
        printf("Eroare la deschiderea bazei de date\n");
        exit(-1);
    }
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "select Id_message from Messages where (Id_transmitter=? and Id_receiver=?) or (Id_receiver=? and Id_transmitter=?)", -1, &stmt, NULL);

    if (stmt != NULL) {

        sqlite3_bind_int(stmt, 1, id_transmitter);
        sqlite3_bind_int(stmt, 2, id_receiver);
        sqlite3_bind_int(stmt, 3, id_transmitter);
        sqlite3_bind_int(stmt, 4, id_receiver);
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {


        last_message = sqlite3_column_int(stmt, 0);

    }
    sqlite3_finalize(stmt);
    return last_message;
}

int verifyExistenceOfIdUser(int id){
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "select Id_user from Users where Id_user=?", -1, &stmt, NULL);

    if (stmt != NULL) {

        sqlite3_bind_int(stmt, 1, id);
    } else {

        fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
    }

    int step = sqlite3_step(stmt);

    if (step == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(stmt);
    return 0;
}//[1]

int verifyExistenceofIdMessage(int id, int id_transmitter, int id_receiver){

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "select Id_message from Messages where Id_message=? and ((Id_transmitter=? and Id_receiver=?) or (Id_receiver=? and Id_transmitter=?))", -1, &stmt, NULL);

    if (stmt != NULL) {

        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_int(stmt, 2, id_transmitter);
        sqlite3_bind_int(stmt, 3, id_receiver);
        sqlite3_bind_int(stmt, 4, id_transmitter);
        sqlite3_bind_int(stmt, 5, id_receiver);
    } else {

        fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
    }

    int step = sqlite3_step(stmt);

    if (step == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(stmt);
    return 0;
}//[1]

void registerr(char *user, char *password, int client) {

    sqlite3_stmt *stmt;
    lastId();
    id_count++;
    string sql;
    sql = "INSERT INTO Users VALUES (?,?,?,1);";

    sqlite3_prepare(db, sql.c_str(), -1, &stmt, NULL);

    if (stmt != NULL) {
        sqlite3_bind_int(stmt, 1, id_count);
        sqlite3_bind_text(stmt, 2, user, strlen(user), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, password, strlen(password), SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    strcat(msgrasp,
           "Esti conectat.Poti face urmatoarele comenzi :- 1 vezi lista utilizatorilor; - 2 deconectare: ");
    changeInfoClient(client, msgrasp, msg);

    commands(client, msg, id_count);
    sqlite3_close(db);
}

char *searchAfterId(int id){
    sqlite3_stmt *stmt2;
    string sql = "select Username from Users where Id_user=?";

    sqlite3_prepare(db, sql.c_str(), -1, &stmt2, NULL);

    char *username = NULL;

    if (stmt2 != NULL) {

        sqlite3_bind_int(stmt2, 1, id);
    } else {

        fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
    }

    int step = sqlite3_step(stmt2);

    if (step == SQLITE_ROW) {

        username = strdup((const char *) sqlite3_column_text(stmt2, 0));

    }

    sqlite3_finalize(stmt2);

    return username;
}//[1]


void returnNotifications(int id_receiver, char notifications[]) {

    int i;
    lastId();

    int dim = id_count+1;
    int fr[dim];

    for(i = 1; i <= id_count; i ++)
        fr[i] = 0;

    if (sqlite3_open("proiect", &db)) {
        printf("Eroare la deschiderea bazei de date\n");
        exit(-1);
    }

    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "select Id_transmitter from Messages where Id_receiver=? and Seen=0", -1, &stmt, NULL);

    if (stmt != NULL) {

        sqlite3_bind_int(stmt, 1, id_receiver);
    } else {

        fprintf(stderr, "Eroare la executarea interogarii: %s\n", sqlite3_errmsg(db));
    }

    int step = sqlite3_step(stmt);

    while (step == SQLITE_ROW) {

        fr[sqlite3_column_int(stmt,0)] ++;
        fflush(stdout);
        step = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt); //[1]

    char * username;
    for(i = 1; i <= id_count; i ++)
    if(fr[i])
        {

            username = searchAfterId(i);
            strcat(notifications, "** Ai ");
            char id_sir[5];
            sprintf(id_sir, "%d", fr[i]);
            strcat (notifications, id_sir);
            strcat(notifications, " mesaje noi de la ");
            strcat(notifications, username);
            strcat(notifications, " ** \n");
        }


}

void seeUsers(int client, int id_transmitter) {

    char notifications[250]="";
    returnNotifications(id_transmitter, notifications);

    if (sqlite3_open("proiect", &db)) {
        printf("Eroare la deschiderea bazei de date\n");
        exit(-1);
    }


    strcat(msgrasp, "Lista utilizatorilor :\n");
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db, "select * from Users", -1, &stmt, NULL);


    int status = 0;

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        const char *database_user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const char *id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));

        status = sqlite3_column_int(stmt, 3);

        strcat(msgrasp, id);
        strcat(msgrasp, ") ");
        strcat(msgrasp, database_user);
        strcat(msgrasp, " : ");
        if (status) strcat(msgrasp, "activ");
        else strcat(msgrasp, "inactiv");
        strcat(msgrasp, "\n");


    }
    sqlite3_finalize(stmt);

    // strcat la mesajul catre client cu notificarile
    strcat(msgrasp, notifications);
    strcat(msgrasp, "Scrie id-ul unui utilizator pentru a deschide conversatia cu acesta\n(sau back pt. a te intoarce la comenzile principale  : ");
    changeInfoClient(client, msgrasp, msg);

    if(strcmp(msg, "back") == 0){
        strcat(msgrasp,
               "Esti conectat.Poti face urmatoarele comenzi :- 1 vezi lista utilizatorilor; - 2 deconectare: ");

        changeInfoClient(client, msgrasp, msg);

        commands(client, msg, id_transmitter);
    }
    int id_receiver = atoi(msg);

    while(verifyExistenceOfIdUser(id_receiver) == 0){
        strcat(msgrasp, "Scrie id-ul unui utilizator existent pentru a deschide conversatia cu acesta : ");

        changeInfoClient(client, msgrasp, msg);
        id_receiver = atoi(msg);

    }

    showConversation(client, id_transmitter, id_receiver);


}

void sendMessage(int client, int id_message, int id_transmitter, int id_receiver, char msg[], int id_reply) {

    if (sqlite3_open("proiect", &db)) {
        printf("Eroare la deschiderea bazei de date\n");
        exit(-1);
    }

    sqlite3_stmt *stmt;
    string sql = "INSERT INTO Messages VALUES (?,?,?,?,?,0);";

    sqlite3_prepare(db, sql.c_str(), -1, &stmt, NULL);

    if (stmt != NULL) {
        sqlite3_bind_int(stmt, 1, id_message);
        sqlite3_bind_int(stmt, 2, id_transmitter);
        sqlite3_bind_int(stmt, 3, id_receiver);
        sqlite3_bind_text(stmt, 4, msg, strlen(msg), SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, id_reply);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    strcat(msgrasp, "Mesaj trimis cu succes. Poti : - 1 trimite alt mesaj; - 2 intoarce la lista de utilizatori: ");
    changeInfoClient(client, msgrasp, msg);

    while(strcmp(msg, "2") && strcmp(msg, "1")){
        strcat(msgrasp, "Comanda nerecunoscuta. Introduceti 1/2 : ");
        changeInfoClient(client, msgrasp, msg);

    }

    if (strcmp(msg, "1") == 0)
            showConversation(client, id_transmitter, id_receiver);
    if (strcmp(msg, "2") == 0)
            seeUsers(client, id_transmitter);

}

// marchez in tabel faptul ca mesajele au fost citite prin deschiderea converstiei
// la un anumit moment
void seenMessages(int id_send, int id_received) {

    if (sqlite3_open("proiect", &db)) {
        printf("Eroare la deschiderea bazei de date\n");
        exit(-1);
    }

    sqlite3_stmt * stmt;
    string sql = "update Messages set Seen=1 where Id_transmitter=? and Id_receiver=?";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

    if (stmt != NULL) {

        sqlite3_bind_int(stmt, 1, id_received);
        sqlite3_bind_int(stmt, 2, id_send);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

}

void showConversation(int client, int id_transmitter, int id_receiver) {

    if (sqlite3_open("proiect", &db)) {
        printf("Eroare la deschiderea bazei de date\n");
        exit(-1);
    }

    char *username = searchAfterId(id_receiver);
    bool empty_conv = true;
    sqlite3_stmt *stmt;
    strcat(msgrasp, "Conversatia : \n");

    sqlite3_prepare_v2(db, "select * from Messages", -1, &stmt, NULL);

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        const char *id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *id_reply = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));


        int id1 = sqlite3_column_int(stmt, 1); // cine a trimis
        int id2 = sqlite3_column_int(stmt, 2); // cui a trimis
        int id3 = sqlite3_column_int(stmt, 4); // replica pentru care mesaje
        int seen = sqlite3_column_int(stmt, 5); // mesajul a fost vazut sau nu

        if (id1 == id_transmitter && id2 == id_receiver) {

            empty_conv = false;
            strcat(msgrasp, "Eu : ");
            strcat(msgrasp, text);
            strcat(msgrasp, " (id mesaj : ");

            strcat(msgrasp, id);

            if (id3) {
                strcat(msgrasp, " este replica pentru mesajul cu id-ul : ");
                strcat(msgrasp, id_reply);

            }
            strcat(msgrasp, " )");
            if(seen) strcat(msgrasp, " - vazut");
            strcat(msgrasp, "\n");


        } else if (id2 == id_transmitter && id1 == id_receiver) {
           empty_conv = false;
            strcat(msgrasp, username);

            strcat(msgrasp, " : ");
            strcat(msgrasp, text);
            strcat(msgrasp, " (id mesaj : ");

            strcat(msgrasp, id);
            if (id3) {
                strcat(msgrasp, " este replica pentru mesajul cu id-ul : ");
                strcat(msgrasp, id_reply);

            }
            strcat(msgrasp, ") ");
            strcat(msgrasp, "\n");

        }

    }

    sqlite3_finalize(stmt);

    // aici fac update la tabelul messages
    seenMessages(id_transmitter, id_receiver);
    strcat(msgrasp, "\n");
    strcat(msgrasp,
           "Acum poti: - 1 trimite un mesaj direct contactului; - 2 poti trimite o replica la un anumit mesaj;\n- 3 intoarce la lista de utilizatori : ");
    changeInfoClient(client, msgrasp, msg);

    while(strcmp(msg, "2") && strcmp(msg, "1") && strcmp(msg, "3")){
        strcat(msgrasp, "Comanda nerecunoscuta. Introduceti 1/2/3 : ");
        changeInfoClient(client, msgrasp, msg);

    }

    if(strcmp(msg, "2") == 0 && empty_conv == true){
        strcat(msgrasp, "Nu exista mesaje. Poti : - 1 trimiti un mesaj direct / - 3 intoarce la lista utilizatorilor: ");
        changeInfoClient(client, msgrasp, msg);
        while(strcmp(msg, "1") && strcmp(msg, "3")) {
            strcat(msgrasp, "Comanda nerecunoscuta. Introduceti 1/3 : ");
            changeInfoClient(client, msgrasp, msg);
        }

    }


    int last_message = lastId_message(id_transmitter,id_receiver);

    if (strcmp(msg, "1") == 0) {
        strcat(msgrasp, "Scrie mesajul : ");
        last_message ++;
        changeInfoClient(client, msgrasp, msg);
        sendMessage(client, last_message, id_transmitter, id_receiver, msg, 0);
    }
    if (strcmp(msg, "2") == 0) {
        strcat(msgrasp, "Scrie id-ul mesajului la care vrei sa raspunzi : ");

        changeInfoClient(client, msgrasp, msg);
        int id_msg_reply = atoi(msg);

        while(verifyExistenceofIdMessage(id_msg_reply, id_transmitter, id_receiver) == 0){
            strcat(msgrasp, "Scrie id-ul unui mesaj existent la care vrei sa raspunzi : ");

            changeInfoClient(client, msgrasp, msg);
            id_msg_reply = atoi(msg);
        }

        strcat(msgrasp, "Scrie mesajul : ");
        last_message++;
        changeInfoClient(client, msgrasp, msg);
        sendMessage(client, last_message, id_transmitter, id_receiver, msg, id_msg_reply);

    }
    if (strcmp(msg, "3") == 0) {
        seeUsers(client, id_transmitter);
    }
}


void commands(int client, char msg[], int id) {

    while(strcmp(msg, "2") && strcmp(msg, "1")){
        strcat(msgrasp, "Comanda nerecunoscuta. Introduceti 1/2 : ");
        changeInfoClient(client, msgrasp, msg);

    }

    if (strcmp(msg, "2") == 0) {
        updateStatus(id, 0);
        printf("[server]S-a deconectat un utilizator...\n");
        fflush(stdout);

        connect(client);

    }
    if (strcmp(msg, "1") == 0) {
        seeUsers(client, id);
    }
}

void changeInfoClient(int client, char msgrasp[], char msg[]) {
    int length_answer = strlen(msgrasp);
    int length_comand;
    msgrasp[length_answer - 1] = '\0';

    if(write(client, &length_answer, sizeof(int)) <= 0){
        perror("[server]Eroare la write() catre client.\n");

    }

    if (write(client, msgrasp, length_answer) <= 0) {
        perror("[server]Eroare la write() catre client.\n");

    }

    if (-1 == read(client, &length_comand, sizeof(int))) {
        perror("Eroare la Read:");
        printf("[server]S-a deconectat un client...");
        close(client);
    }


    bzero(msg, 200);
    if (-1 == read(client, msg, length_comand) <= 0) {
        perror("[server]Eroare la read() de la client.\n");
        close(client);
    }


    msg[strlen(msg) - 1] = '\0';

    bzero(msgrasp, 400);
}

int main() {
    struct sockaddr_in server;
    struct sockaddr_in client;
    int optval = 1;

    int sd;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // pregatirea structurilor de date
    bzero(&server, sizeof(server));
    bzero(&client, sizeof(client));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);


    if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    // serverul este deschis sa receptioneze conexiuni de la clienti
    if (listen(sd, 1) == -1) {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    // servirea cererilor venite de la clienti in mod concurent
    while (1) {
        int client;
        socklen_t length = sizeof(client);

        printf("[server]Se asteapta la portul %d...\n", PORT);
        fflush(stdout);

        // se accepta un client (stare blocanta pina la realizarea conexiunii)
        client = accept(sd, (struct sockaddr *) &client, &length);

        /* eroare la acceptarea conexiunii de la un client */
        if (client < 0) {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        int pid;
        if ((pid = fork()) == -1) {
            close(client);
            continue;
        } else if (pid > 0) {
            // parinte
            close(client);
            while (waitpid(-1, NULL, WNOHANG));
            continue;
        } else if (pid == 0) {
            // copil
            close(sd);

            bzero(msg, 100);

            printf("[server]A venit un client...\n");

            connect(client);

            close(client);

            exit(0);
        }

    }//[2]
}
