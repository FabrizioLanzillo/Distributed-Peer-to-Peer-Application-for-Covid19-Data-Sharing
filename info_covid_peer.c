#include "info_covid.h"
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h> 
#define __USE_XOPEN
#include <time.h>


#define REQUEST_LEN 5
#define RESPONSE_LEN 31  //OK\0

/********************************************************************
funzione che stampa il messaggio iniziale
parametri:
nessuno
********************************************************************/
void starting_message_peer(){

    printf("---------------------------------------------------------------------------------------\n");
    printf("---------------------------------- PEER STARTED ---------------------------------------\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Digita un comando tra quelli elencati per iniziare: \n\n");
    printf("1) start DS_addr DS_port    ===> Fa registare questo peer presso il DS selezionato \n");
    printf("2) add type quantity        ===> Aggiunge il numero di tamponi/casi odierni \n");
    printf("3) get aggr type period     ===> Calcola e registra dati aggregati su tamponi/casi \n");
    printf("4) stop                     ===> Disconnette il peer dalla rete \n");
    printf("---------------------------------------------------------------------------------------\n");
}

/******************************************************************************
funzione che registra il peer alla rete
parametri:
@ socket_peer è il socket utilizzato per dialogare con il DS usando UDP
@ indirizzo_socket_ds è la struct sockaddr_in relativa al ds
@ boot è la struttura messaggio per inviare i valori per la registrazione
@ ready è il set di descrittori pronti, per il reinvio nel caso di dati 
        persi visto che si utilizza UDP
@ time_boot è il tempo che bisogna aspettare per il reinvio dei dati 
@ fdmax è il valore del descrittore più grande
@ buffer_ports è un buffer per l'ACK
@ addrlen_ds è sizeof(indirizzo_socket_ds)
@ neighbors è un array con il numero di porta dei vicini
******************************************************************************/
void start(int socket_peer, struct sockaddr_in indirizzo_socket_ds, struct message* boot, fd_set ready, struct timeval time_boot, int fdmax, char *buffer_ports, socklen_t addrlen_ds, unsigned short *neighbors){
    
    // variabili di utility
    int ret,j;
    // variabile per raccogliere il neighbor ricevuto
    unsigned short neighbor_received; 
    printf("-------------------------------------------------------------------------\n");
    printf("-------------------- RICHIESTA REGISTRAZIONE INIZIATA -------------------\n");
    printf("-------------------------------------------------------------------------\n");
    
    // invio al ds dell'indirizzo IP del peer
    do{
        do{
            ret = sendto(socket_peer, &(boot->type), INET_ADDRSTRLEN, 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));	
        } while(ret < 0);
        if(boot->type == 'B'){
            printf("[INVIO TIPOLOGIA RICHIESTA]: Registrazione nuovo peer\n");
        }
        else{
            printf("[INVIO TIPOLOGIA RICHIESTA]: Disconnessione peer\n");
        }

    }while(select(fdmax + 1, &ready, NULL, NULL, &time_boot) < 0);  
    
    // invio al ds dell'indirizzo IP del peer
    do{
        do{
            ret = sendto(socket_peer, boot->address, INET_ADDRSTRLEN, 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));	
        } while(ret < 0);
        printf("[   INVIO INDIRIZZO PEER  ]: %s\n", boot->address);

    }while(select(fdmax + 1, &ready, NULL, NULL, &time_boot) < 0);      

    // invio del numero di porta del peer al ds
    do{
        do{
            ret = sendto(socket_peer, &(boot->port), sizeof(uint16_t), 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));
            if(ret < 0){
                perror("[ ATTESA INVIO PORTA PEER ] tentativo di reinvio in corso...\n");	
            }	
        } while(ret < 0);
        printf("[     INVIO PORTA PEER    ]: %hu\n", ntohs(boot->port));

    }while(select(fdmax + 1, &ready, NULL, NULL, &time_boot) < 0);

    // ricezione dell'ACK di corretta registrazione avvenuta
    do{
        ret = recvfrom(socket_peer, buffer_ports, RESPONSE_LEN, 0,  (struct sockaddr *)&indirizzo_socket_ds, &addrlen_ds);	
    }while(ret < 0);
    printf("[       RICEZIONE ACK     ]: %s\n", buffer_ports);   
    // controllo se il ds ha mandato un messaggio di peer già presente, in quel caso non si aspetta
    // l'ivio dei 2 neighbor
    if(strcmp(buffer_ports, "PEER GIA' PRESENTE NELLA RETE") != 0){
        for(j=0; j<2; j++){                   
            do{
                ret = recvfrom(socket_peer, &neighbor_received, sizeof(uint16_t), 0,  (struct sockaddr *)&indirizzo_socket_ds, &addrlen_ds);
                
            }while(ret < 0);
            neighbor_received= ntohs(neighbor_received);
            if(j == 0){
                neighbors[0] = neighbor_received;
                printf("[ RICEZIONE PRIMO VICINO  ]: %hu\n", neighbor_received);    
            }
            else{
                neighbors[1] = neighbor_received;
                printf("[RICEZIONE SECONDO VICINO ]: %hu\n", neighbor_received);    
            }  
        }
    }
    printf("-------------------------------------------------------------------------\n");
    printf("-------------------- RICHIESTA REGISTRAZIONE TERMINATA ------------------\n");
    printf("-------------------------------------------------------------------------\n");
}

/******************************************************************************
funzione che legge tutte le righe di un register
parametri:
@ path è il percorso del file da leggere
@ elements è un array che conterrà tutte le righe lette
******************************************************************************/
bool read_file(char *path, char elements[N_LINES][LINE_SIZE]){
    // variabile per la linea da leggere a quel path
    char *line = NULL;
    // dimensione massima della linea da leggere
    size_t  line_buffer_size = 0;
    // numero di linee da leggere
    int line_count = 0;
    int i;
    // variabile che individua il file da leggere 
    FILE *file = fopen(path, "r");
    if(!file){
        return false;
    }
    
    for(i = 0; i< N_LINES; i++){
        // prendo la prima linea e la copio nel primo slot di elements
        getline(&line, &line_buffer_size, file); 
        strcpy(elements[i], line);
        line_count++;
    }

    // si liberano e chiudono le cose usate
    free(line);
    line = NULL;
    fclose(file);
    return true;

}

/******************************************************************************
funzione che legge n_lines righe di un register
parametri:
@ path è il percorso del file da leggere
@ n_lines è il numero di righe da leggere
@ elements è un array che conterrà tutte le righe lette
******************************************************************************/
bool read_file_var(char *path, int n_lines, char **elements){

    // variabile per la linea da leggere a quel path
    char *line = NULL;
    // dimensione massima della linea da leggere
    size_t  line_buffer_size = 0;
    // numero di linee da leggere
    int line_count = 0;
    int i;
    // variabile che individua il file da leggere 
    FILE *file = fopen(path, "r");
    if(!file){
        return false;
    }
    
    for(i = 0; i< n_lines; i++){
        // prendo la prima linea e la copio nel primo slot di elements
        getline(&line, &line_buffer_size, file); 
        strcpy(elements[i], line);
        line_count++;
    }
    
    // si liberano e chiudono le cose usate
    free(line);
    line = NULL;
    fclose(file);
    return true;

}

/******************************************************************************
funzione che scrive tutte le righe di un register
parametri:
@ path è il percorso del file in cui scrivere
@ elements è un array che conterrà le righe da scrivere
******************************************************************************/
bool write_file(char *path, char elements[N_LINES][LINE_SIZE]){
    // variabile che individua il file in cui scrivere a quel path
    FILE *file = fopen(path, "w+");
    // dimensione della riga da scrivere
    int num[N_LINES];
    // variabile per loop
    int i;

    // loop per scrivere le 2 righe nel file
    for(i=0; i< 4; i++){
        if(!file){
            return false;
        }
        num[i] = strlen(elements[i]);	
   	    fwrite(elements[i], 1, num[i], file);   
    }

    // si chiude il file             
   	fclose(file);    
   	return true;

}

/******************************************************************************
funzione che scrive n_lines righe di un register
parametri:
@ path è il percorso del file in cui scrivere
@ n_lines è il numero di righe da scrivere
@ elements è un array che conterrà le righe da scrivere
******************************************************************************/
bool write_file_aggr(char *path, int n_lines, char **elements){

    // variabile che individua il file in cui scrivere a quel path
    FILE *file = fopen(path, "w+");
    // dimensione della riga da scrivere
    int num[n_lines];
    // variabile per loop
    int i;

    // loop per scrivere le 2 righe nel file
    for(i=0; i< n_lines; i++){
        if(!file){
            return false;
        }
        num[i] = strlen(elements[i]);	
   	    fwrite(elements[i], 1, num[i], file);   
    }

    // si chiude il file             
   	fclose(file);    
   	return true;

}

/******************************************************************************
funzione che aggiunge tamponi o casi al register dellaa data odierna 
se prima delle 18 altrimenti li aggiunge al giorno seguente
parametri:
@ port_peer è il numero di porta di questo peer
@ path_register è il percorso del file in cui scrivere/aggiornare
@ type_to_convert è il tipo di dato da scrivee/aggiornare
@ quantity è il valore da aggiungere
******************************************************************************/
void add(unsigned short port_peer, char *path_register, char *type_to_convert, int quantity){
    // array di appoggio per la costruzione del path completo, per accedere al file
    char path_completo[PATH_COMPLETE];
    char name_file[NAME_LEN];
    char extension[5] = ".txt\0";
    // variabili di controllo di corretta lettura
    bool is_toread = false, is_towrite = false;
    // tipologia di add
    char type = type_to_convert[0];
    // array che contiene i valori delle righe del file
    // sia che si voglia leggere o scrivere
    char elements[N_LINES][LINE_SIZE];
    // definizione delle strutture per il tempo
    time_t rawtime;
    struct tm *timeinfo;
    // variabili per loop
    int j, counter_char = 2;
    // array contenente i 2 numeri delle 2 righe
    int number[N_LINES];
    
    // creazione del path completo, in cui è presente il nome del file
    memset(path_completo, 0, sizeof(path_completo));
    strcat(path_completo, path_register);
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if(timeinfo->tm_hour > 18){
        timeinfo->tm_mday = timeinfo->tm_mday + 1;
    }
    mktime(timeinfo);
    memset(name_file, 0, sizeof(name_file));
    sprintf(name_file, "%d-%d-%d", timeinfo->tm_mday, ((timeinfo->tm_mon) +1), ((timeinfo->tm_year)+1900));
    strcat(name_file, extension);
    strcat(path_completo, name_file);

    // lettura del file a quel determinato path, di passa anche elements
    // in quanto verranno copiate in esso le 2 linee del file
    is_toread = read_file(path_completo, elements);
    // caso in cui file è gia' presente
    if(is_toread){
        for(j=0; j <2; j++){

            number[j] = get_number_entry(j, counter_char, elements);
            counter_char = 2;
        }
        // aggiorno quello che è stato cambiato
        if(type == 'T'){
            sprintf(elements[0], "T:%d;\n", (quantity +  number[0]));
            printf("Entry del register presente e aggiornata con successo\n%s", elements[0]);
        }
        else{
            sprintf(elements[1], "N:%d;\n", (quantity +  number[1]));
            printf("Entry del register presente e aggiornata con successo\n%s", elements[1]);
        }

    }
    // se il file non è presente va inizializzato elements opportunamente
    else{
        // si aggiunge la quantity al tipo scelto con la add
        if(type == 'T'){
            sprintf(elements[0], "T:%d;\n", quantity);
            sprintf(elements[1], "N:0;\n");
            printf("Entry del register creata con successo\n%s", elements[0]);
        }
        else{
            sprintf(elements[0], "T:0;\n");
            sprintf(elements[1], "N:%d;\n", quantity);
            printf("Entry del register creata con successo\n%s", elements[1]);
        }
        // queste variabili servono per il flooding, ma vanno inizializzate
        sprintf(elements[2], "TT:0;\n");
        sprintf(elements[3], "TN:0;");

        
    }

    // si scrive sul file, se non è presente viene creato
    is_towrite = write_file(path_completo, elements);
    if(!is_towrite){      
        printf("Non è stato possibile aggiornare/creare la entry\n");
    }
}

/**********************************************************************************
funzione che crea la cartella di ogni peer 
parametri:
@ port_peer è il numero di porta di questo peer
@ path_register è il percorso che contiene i register del peer
@ path_aggr_t è il percorso che contiene i dati aggregati del peer per i tamponi
@ path_aggr_c è il percorso che contiene i dati aggregati del peer per i casi
***********************************************************************************/
bool create_folders(unsigned short port_peer, char *path_register, char *path_aggr_t, char *path_aggr_c){
    
    int counter = 0;   // per controllare che tutte le cartelle siano state create con successo
    // variabile per controllare se la directory è già presente 
    int status, i;
    // nome della cartella di questo peer
    char dirname[7]; 
    // struct di appoggio per la stat
    struct stat st;
    // percorso della cartella
    char path_peer[PATH_PEER] = "files/\0";
    char path_comand[PATH_NO_FILE];

    for(i = 0; i<5; i++){
        memset(path_comand, 0, sizeof(path_comand));
        if(i == 0){
            // di crea il percorso path
            sprintf(dirname, "%hu", port_peer);
            strcat(path_peer, dirname); 
            strcpy(path_comand, path_peer);
        }
        else if(i == 1){
           strcpy(path_comand, path_peer);
           strcat(path_comand, "/register/");
           strcpy(path_register, path_comand);
        }
        else if(i == 2){
            strcpy(path_comand, path_peer); 
            strcat(path_comand, "/aggr/");
        }
        else if(i == 3){
            strcpy(path_comand, path_peer); 
            strcat(path_comand, "/aggr/tamponi/");
            strcpy(path_aggr_t, path_comand);
        }
        else{
            strcpy(path_comand, path_peer); 
            strcat(path_comand, "/aggr/casi/");
            strcpy(path_aggr_c, path_comand);
        }
        status = stat(path_comand, &st);
        if (status == -1) {
            mkdir(path_comand, 0700);
            counter++;
        }
    }
    if (counter == 5){ 
        return true;
    } 
    else{
        return false;
    }
}

/*****************************************************************************************
funzione che crea un file register da zero 
parametri:
@ peer è il numero di porta di questo peer
@ date è la data relativa al register
@ t è il numero di tamponi registarti da questo peer per questo register
@ n è il numero di casi registarti da questo peer per questo register 
@ tt è il numero complessivo di tamponi registrati da tutti i peer per questo register 
@ tn è il numero complessivo di casi registrati da tutti i peer per questo register 
****************************************************************************************/
void create_file_register(int peer, char *date, int t, int n, int tt, int tn){
  
    // array di appoggio per la costruzione del path completo, per accedere al file
    char path_completo[PATH_COMPLETE];
    char elements[N_LINES][LINE_SIZE];
    bool is_towrite = false;

    //sprintf(path_completo, "files/%d/register/%d-%d-%d.txt", peer, data[0],data[1],data[2]);
    sprintf(path_completo, "files/%d/register/%s", peer, date);
    sprintf(elements[0], "T:%d;\n", t);
    sprintf(elements[1], "N:%d;\n", n);
    sprintf(elements[2], "TT:%d;\n", tt);
    sprintf(elements[3], "TN:%d;", tn);

    // si scrive sul file, se non è presente viene creato
    is_towrite = write_file(path_completo, elements);
    if(!is_towrite){      
        printf("Non è stato possibile aggiornare/creare la entry\n");
    }
}

/*****************************************************************************************
funzione che crea una connessione tcp e si occupa dello scambio dati 
parametri:
@ port è la porta del peer alla quale effettuare la richeista di connessione
@ type è il tipo di richiesta da inviare
@ info_tcp è il tipo di dato da richiedere
@ port_req è la porta del peer attuale che richiede la connessione
@ value_temp è un valore intero che vogliamo inviare
@ payload è un campo dati di tipo stringa che vogliamo inviare
@ entry_from_peers contiene il dato richiesto dalla connessione
@ elements_var è un array dinamico che contiene le variazioni del dato aggr
****************************************************************************************/
bool create_tcp_connection(unsigned short port, char type, char *info_tcp, unsigned short port_req, int value_temp, char *payload, int *entry_from_peers, char **elements_var){

    int comunication;
    struct sockaddr_in indirizzo_socket_neighbor;
    int ret, i;
    // message per inviare messaggi ai peer
    int len = sizeof(struct message);
    struct message *set = (struct message*)malloc(len+1);
    int result = 0;
    char single_var[LINE_SIZE_VAR];
    bool return_value = false;
    char type_extended[10];

    // si crea una connessione tcp con il vicino appena aggiornato
    comunication= socket(AF_INET, SOCK_STREAM, 0);	
    memset(&indirizzo_socket_neighbor, 0, sizeof(indirizzo_socket_neighbor));
    indirizzo_socket_neighbor.sin_family = AF_INET;
    indirizzo_socket_neighbor.sin_port = htons(port);
    indirizzo_socket_neighbor.sin_addr.s_addr = INADDR_ANY;

    if(strcmp(info_tcp, "T") == 0){
        sprintf(type_extended, "%s", "tamponi");
    }
    else{
        sprintf(type_extended, "%s", "casi");
    }

    printf("*************************** SCAMBIO DATI CON GLI ALTRI PEER ****************************\n");
    printf("1) -------------- FASE DI CONNESSIONE -------------------------\n");

    ret = connect(comunication, (struct sockaddr *)&indirizzo_socket_neighbor, sizeof(indirizzo_socket_neighbor));  
    if(ret < 0){                      
        perror("Errore in fase di connesione: \n");
    }
    printf("RICHIESTA CONNESSIONE TCP INVIATA AL PEER %hu\n", port);  

    printf("2) --------------- FASE DI SCAMBIO -----------------------------\n");


    if(type == 'P'){
        printf("Richiesta: numero di entry complessive\nTipologia: %s\nDestinatario: vicini\nData: %s\n", type_extended, payload);
    }
    else if(type == 'T' || type == 'V'){
        printf("Richiesta: AGGREGATO\nTipologia: %s\nDestinatario: vicini\nIntervallo: %s\n", type_extended, payload);
    }
    else if(type == 'S'){
        printf("Invio: entry\nTipologia: %s\nDestinatario: vicini\nData: %s\n", type_extended, payload);
    }
    else{
        printf("Richiesta: numero di entry complessive\nTipologia: %s\nDestinatario: FLOODING\nData: %s\n", type_extended, payload);
    }
    // creazione del messaggio da inviare
    // si deve passare tipo, 
    if(type == 'F' || type == 'S'){
        set = create_message((uint8_t)type, (uint8_t*)info_tcp, (uint16_t)port_req, NULL, (uint32_t)value_temp, (uint8_t*)payload);
    }
    else if(type == 'V'){
        set = create_message((uint8_t)type, (uint8_t*)info_tcp, 0, NULL, (uint32_t)value_temp, (uint8_t*)payload);
    }
    else{
        set = create_message((uint8_t)type, (uint8_t*)info_tcp, 0, NULL, 0, (uint8_t*)payload);
    }

    // si manda la richiesta al server
    ret = send(comunication, &(set->type), sizeof(uint8_t), 0);
    if(ret < 0){
        perror("Errore in fase di invio del tipo di richiesta: \n");	
        exit(0);
    }	

    // si manda la richiesta al server
    ret = send(comunication, (set->info), LEN_INFO_TCP, 0);
    if(ret < 0){
        perror("Errore in fase di invio della tipologia di dato: \n");	
        exit(0);
    }	

  
    // si manda la richiesta al server
    ret = send(comunication, (set->payload), NAME_AGGR_LEN, 0);
    if(ret < 0){
        perror("Errore in fase di invio della data/intervallo: \n");	
        exit(0);
    }	


    if(type == 'F' || type == 'S'){
        // si manda la richiesta al server
        ret = send(comunication, &(set->port), sizeof(uint16_t), 0);
        if(ret < 0){
            perror("Errore in fase di invio della porta di origine: \n");	
            exit(0);
        }	

        // si manda la richiesta al server
        ret = send(comunication, &(set->data), sizeof(uint32_t), 0);
        if(ret < 0){
            perror("Errore in fase di invio del dato temporaneo o del valore da salvare: \n");	
            exit(0);
        }	
    }
    if(type == 'V'){

        // si manda la richiesta al server
        ret = send(comunication, &(set->data), sizeof(uint32_t), 0);
        if(ret < 0){
            perror("Errore in fase di invio del numero di variazioni: \n");	
            exit(0);
        }	
    }

    printf("ATTESA PER RISULTATO...\n");

    if(type != 'V'){

        ret = recv(comunication, &result, LINE_SIZE, 0);
        *entry_from_peers = ntohl(result);
        if(ret < 0){
        
            perror("Errore in fase di ricezione: \n");	
            exit(0);	
        }	

        if(*entry_from_peers == -1){
            printf("DATO NON PRESENTE NEI FILE DI QUEL PEER\n");
            return_value =  false;
        }
        else{
            if(type == 'S'){
                printf("ENTRY PER QUESTO REGISTER INVIATE CORRETTEMENTE\n");
            }
            else{
                printf("DATO DI VALORE: %d RICEVUTO CORRETTEMENTE\n", *entry_from_peers);
            }
            return_value =  true;
        }
    }
    else{
        
        ret = recv(comunication, single_var, LINE_SIZE_VAR, 0);
        if(ret < 0){
            perror("Errore in fase di ricezione: \n");	
            exit(0);	
        }	
        if(strcmp(single_var, "NO") == 0){
            
            return_value = false;
            printf("DATO NON PRESENTE NEI FILE DI QUEL PEER\n");
        }
        else{
            strcpy(elements_var[0], single_var);
            for(i=1; i<value_temp; i++){
                memset(single_var, 0, sizeof(single_var));
                ret = recv(comunication, single_var, LINE_SIZE_VAR, 0);
                if(ret < 0){
                    perror("Errore in fase di ricezione: \n");	
                    exit(0);	
                }	
                strcpy(elements_var[i], single_var);
            }

            printf("DATO RICEVUTO CORRETTEMENTE\n");
            return_value = true;
        }
    }

    close(comunication);
    printf("****************************************************************************************\n\n");
    
    return return_value;
}

/*****************************************************************************************
funzione che calcola i dati aggregati richiesti 
parametri:
@ port è il peer attuale
@ path_register è il percorso che contiene i register del peer
@ path_aggr è il percorso che contiene i dati aggregati del peer
@ aggr è la tipologia di aggregato da calcolare
@ type è il tipo di dato da analizzare 
@ date è l'intervallo su cui calcolare aggr
@ neighbors è un array con il numero di porta dei vicini
****************************************************************************************/
void get(int peer, char *path_register, char *path_aggr, char *aggr, char *type, char *date, unsigned short *neighbors){

    // array di appoggio per la costruzione del path completo, per accedere al file
    // e variabili per accedere alla cartella
    char path_completo_aggr[PATH_COMPLETE];
    char path_completo_register[PATH_COMPLETE];
    char name_file[NAME_AGGR_LEN];
    char extension[5] = ".txt\0";
    int type_toread;
    char info_tcp[LEN_INFO_TCP];
    char date_var[NAME_LEN];
    // variabili per il prelievo di dati dallo stream di ingresso
    char quantity_read[6];  // array temporaneo in cui depositare il segmento di data prelevato
    int data[2][3];         // array contentente i segmenti di data
    char start_date[11];    // array contenente il nome del file completo della data di partenza 
    char end_date[11];      // array contenente il nome del file completo della data di fine
    char toopen_date[11];  
    // array che contengono i dati letti da ogni file o i dati che scrive in ogni file
    char elements[N_LINES][LINE_SIZE];    // questo per prelevare i dati dai register
                                          // o scrivere/leggere il totale
    char **elements_var = NULL;           // questo per le variazioni, vissto che non
                                          // ne conosciamo a priori il numero
    char **elements_tot = NULL;
    // variabili per loop           
    int i;                  // conteggia le righe del vettore data
    int j=0;                // conteggia tutti i caratteri dell'array date che viene passa
                            // dallo stream di ingresso   
    int k=0;                // conteggia gli elementi dell'array quantity_read       
    int w=0;                // conteggia gli elementi dell'array quantity_read
    int n = 0;
    int insert_slot=0;      // serve per ordinare l'array di struct
    int counter_date = 0;   // serve per ordinare l'array di struct
    int numero_file_period = 0;
    // variabili booleane per controllo di flusso
    bool is_toread = false;
    bool is_towrite = false;
    bool not_date_start = false;
    bool not_date_end = false;
    // dato aggregato da restituire e da scrivere nel file
    int *variation = NULL;  
    int totale = 0;
    // strutture e variabili per la gestione del tempo
    struct tm *timeinfo;
    struct tm tm = {0};
    time_t to_open, start, end, today;
    int entry_from_peers = 0;
    bool result_conn_tcp = false;
    int valore_entry_to_add = 0;
    bool already_asked_aggr = false;
    bool is_first = true;


    if(strcmp(type,"T") == 0){
        type_toread = 2;
        strcpy(info_tcp, "T");
    }
    else{
        type_toread = 3;
        strcpy(info_tcp, "N");
    }
    // algoritmo per ricostruire la data iniziale e finale, prelevate dallo stream di ingresso
    for(i = 0; i<2; i++){
        for(w = 0; w<3; w++){
            memset(quantity_read, 0, sizeof(quantity_read));
            k=0;
            if(date[j] == '*'){
                if(i == 0){
                    not_date_start = true;
                    j = 2;
                }
                else{
                    not_date_end = true;
                }
                break;
            }
            while(date[j] != ':'){   
                if((date[j] == '-') || (j == 21)){
                    break;
                }       
                quantity_read[k] = date[j];
                k++;
                j++;
            }          
            
            data[i][w]= atoi(quantity_read);
            if((date[j] == '-') || (j == 21)){
                j++;
                break;
            }
            else{
                j++;
            }                 
        }
    }

    if(not_date_start){
        sprintf(start_date, "%s", "4-3-2020");
    }
    else{
        sprintf(start_date, "%d-%d-%d", data[0][0],data[0][1],data[0][2]);
    }
    if(not_date_end){
        time(&today);
        timeinfo = localtime(&today);
        sprintf(end_date, "%d-%d-%d", timeinfo->tm_mday, ((timeinfo->tm_mon) +1), ((timeinfo->tm_year)+1900));
        
    }   
    else{
        sprintf(end_date, "%d-%d-%d", data[1][0],data[1][1],data[1][2]);
    }

    // qui sto creando solo il nome del file di tipo aggregazione, poi dovrò cercarlo per vedere se esiste
    // se così non fosse devo crearlo e poi andare a prendere i dati per scriverci dentro.
    // creazione del path completo, in cui è presente il nome del file
    memset(path_completo_aggr, 0, sizeof(path_completo_aggr));
    strcat(path_completo_aggr, path_aggr);
    if(strcmp(aggr,"totale") == 0){
        sprintf(name_file, "tot_%s_%s", start_date, end_date);
    }
    else{
        sprintf(name_file, "var_%s_%s", start_date, end_date);
    }
    strcat(name_file, extension);
    strcat(path_completo_aggr, name_file);
    
    // si trasforma la data di partenza in tipo time
    if (strptime(start_date, "%d-%m-%Y", &tm) == NULL){
        printf("errore nella creazione della data\n");
    }
    start = mktime(&tm);
    if(start == -1){
        printf("errore nella conversione della data\n");
    }
    // si fa la stessa cosa per di fine
    if (strptime(end_date, "%d-%m-%Y", &tm) == NULL){
        printf("errore nella creazione della data\n");
    }
    end = mktime(&tm);
    if(end == -1){
        printf("errore nella conversione della data\n");
    }
    // si indetifica ik tipo di operazione aggregata
    if(strcmp(aggr,"totale") == 0){
        // prima si vede se questo aggregato è già presente perchè precedentemente calcolato
        is_toread = read_file(path_completo_aggr, elements);
        // caso in cui file è gia' presente, si restituisce il valore dell'aggregato
        if(is_toread){
            printf("Dato aggregato già presente\nTOTALE: %s\n", elements[0]);
        }
        // se il file non è presente bisogna creare questo file scrivendoci dentro 
        // e per farlo dobbiamo analizzare tutte le entry per tutti i register per ogni data
        // dove il nome di quei file ha una data maggiore di quella di start e minore di quella di stop
        else{

            elements_tot = malloc(1 * sizeof(char*));
            elements_tot[0] = malloc((LINE_SIZE+1) * sizeof(char));

            // creo il percorso per la directory register del peer
            memset(path_completo_register, 0, sizeof(path_completo_register));
            strcat(path_completo_register, path_register);
            

            // si trasforma la data di partenza in tipo time
            if (strptime(start_date, "%d-%m-%Y", &tm) == NULL){
                printf("errore nella creazione della data\n");
            }
            to_open = mktime(&tm);
            if (to_open == -1){
                printf("errore nella mktime\n");
            }  

            // scorriamo tutte le date, finchè non le abbiamo esaminate tutte
            while (true){
                
                if(!is_first){
                    tm.tm_mday = tm.tm_mday +1;
                    to_open = mktime(&tm);
                    if (to_open == -1){
                        printf("errore nella mktime\n");
                    }  
                }
                else{
                    is_first = false;
                } 
                
                sprintf(toopen_date, "%d-%d-%d.txt", tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900);

                // se la data esaminata rientra in quel peroido temporale si legge il valore
                if((difftime(to_open, start) >= 0) && (difftime(end, to_open) >= 0)){
                    // si crea il percorso aggiungendo il nome di quel file e si legge
                    memset(path_completo_register, 0, sizeof(path_completo_register));
                    strcat(path_completo_register, path_register);
                    strcat(path_completo_register, toopen_date);
read_tot:
                    is_toread = read_file(path_completo_register, elements);
                    if(!is_toread){      
                        printf("Non è stato possibile leggere questo file\n");
                        create_file_register(peer, toopen_date, 0, 0, 0, 0);
                        goto read_tot;
                    }

                    if(strcmp(type, "T") == 0){
                        type_toread = 2;
                    }
                    else{
                        type_toread = 3;
                    }


                    valore_entry_to_add = get_number_entry(type_toread, 3, elements);
                    if(valore_entry_to_add == 0){
                        result_conn_tcp = false;
                        // bisogna chiedere il dato aggr ai vicini
                        if(!already_asked_aggr){
                            for(n=0; n < N_NEIGHBOR; n++){
                                result_conn_tcp = create_tcp_connection(neighbors[n], 'T', info_tcp, 0, 0, name_file, &entry_from_peers, elements_var);
                                if(result_conn_tcp){
                                    valore_entry_to_add = entry_from_peers;
                                    break;
                                }
                            }
                            already_asked_aggr = true;
                        }
                        if(result_conn_tcp){
                            totale = valore_entry_to_add;
                            break;
                        }
                        else{
                            // bisogna creare una connessione tcp e prendere questo dato che è il valore complessivo di tutte le entry per quel
                            // particolare register di quel giorno
                            for(n=0; n < N_NEIGHBOR; n++){
                                result_conn_tcp = create_tcp_connection(neighbors[n], 'P', info_tcp, 0, 0, toopen_date, &entry_from_peers, elements_var);
                                if(result_conn_tcp){
                                    valore_entry_to_add = entry_from_peers;
                                    break;
                                }
                            }  
                            if(!result_conn_tcp){
                                
                                if(strcmp(type, "T") == 0){
                                    type_toread = 0;
                                }
                                else{
                                    type_toread = 1;
                                }

                                valore_entry_to_add = get_number_entry(type_toread, 2, elements);
                                result_conn_tcp = create_tcp_connection(neighbors[1], 'F', info_tcp, peer, valore_entry_to_add, toopen_date, &entry_from_peers, elements_var);
                                valore_entry_to_add = entry_from_peers;
                                
                            }
                            if(strcmp(type, "T") == 0){
                                sprintf(elements[2], "TT:%d;\n", valore_entry_to_add);
                            }
                            else{
                                sprintf(elements[3], "TN:%d;", valore_entry_to_add);
                            }
                        }
                        
                        // poi dobbiamo far scrivere questo valore nel registro di questo peer 
                        // perchè va salvato nel register del peer
                        is_towrite = write_file(path_completo_register, elements);
                        if(!is_towrite){      
                            printf("Non è stato possibile aggiornare/creare la entry\n");
                        }
                    }
                    // lo converto in int
                    totale = totale + valore_entry_to_add;        

                             
                }
                else{
                    break;
                }
            }   
            sprintf(elements_tot[0], "%d", totale);
            is_towrite = write_file_aggr(path_completo_aggr, 1, elements_tot);
            if(!is_towrite){      
                printf("Non è stato possibile aggiornare/creare la entry\n");
            }
            printf("Dato aggregato non presente, creato con successo\nTOTALE: %s\n", elements_tot[0]);  
        }
    }
    else{

        // si trasforma la data di partenza in tipo time
        if (strptime(start_date, "%d-%m-%Y", &tm) == NULL){
            printf("errore nella creazione della data\n");
        }
        to_open = mktime(&tm);
        if (to_open == -1){
            printf("errore nella mktime\n");
        }  
        // scorriamo tutte le date, finchè non le abbiamo esaminate tutte
        while (true){
            
            if(!is_first){
                tm.tm_mday = tm.tm_mday +1;
                to_open = mktime(&tm);
                if (to_open == -1){
                    printf("errore nella mktime\n");
                }  
            }
            else{
                is_first = false;
            } 
            // se la data esaminata rientra in quel peroido temporale si legge il valore 
            if((difftime(to_open, start) >= 0) && (difftime(end, to_open) >= 0)){ 
                numero_file_period++;
            }
            else{
                break;
            }
        }
        is_first = true;

        // dobbiamo contare il numero di registri compresi nel periodo indicato nello stream di ingresso
        memset(path_completo_register, 0, sizeof(path_completo_register));
        strcat(path_completo_register, path_register);


        // si allocano un array bidimensionale con un numero di linee pari
        // al numero di differenze, che non è noto a priori in quanto dipende dal period
        elements_var = malloc(numero_file_period * sizeof(char*));
        for(i = 0; i< numero_file_period; i++){
            elements_var[i] = malloc((LINE_SIZE_VAR+1) * sizeof(char));
        }
        // array contenente le variazioni, nel caso debbano essere calcolate
        variation = (int *)malloc(VALUE_LEN * (numero_file_period-1) * sizeof(int));
        // si alloca un array di strutture, che contiene, la differenza con la data di partenza
        // di ogni data corrispondente al register, la data in questione e il numero di casi/tamponi
        struct variation_date *var_date  = malloc(numero_file_period * sizeof(struct variation_date));

        // prima si vede se questo aggregato è già presente perchè precedentemente calcolato
        is_toread = read_file_var(path_completo_aggr, numero_file_period, elements_var);
        // caso in cui file è gia' presente, si restituisce il valore dell'aggregato
        if(is_toread){
            printf("Dato aggregato già presente:\n");
            for(i = 0; i< numero_file_period-1; i++){       
                printf("%s", elements_var[i]);
            }   
        }

        // se il file non è presente bisogna creare questo file scrivendoci dentro 
        // e per farlo dobbiamo leggere tutti i file nella cartella 
        // dove il nome di quei file ha una data maggiore di quella minima e minore di quella massima
        else{

            // scorriamo tutti i file, finchè non li abbiamo esaminati tutti
            i = 0;
            // si trasforma la data di partenza in tipo time
            if (strptime(start_date, "%d-%m-%Y", &tm) == NULL){
                printf("errore nella creazione della data\n");
            }
            to_open = mktime(&tm);
            if (to_open == -1){
                printf("errore nella mktime\n");
            }  

            // scorriamo tutti i file, finchè non li abbiamo esaminati tutti
            while (true){
                
                if(!is_first){
                    tm.tm_mday = tm.tm_mday +1;
                    to_open = mktime(&tm);
                    if (to_open == -1){
                        printf("errore nella mktime\n");
                    }  
                }
                else{
                    is_first = false;
                } 
                
                sprintf(toopen_date, "%d-%d-%d.txt", tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900);

                // se la data esaminata rientra in quel peroido temporale si legge il valore 
                if((difftime(to_open, start) >= 0) && (difftime(end, to_open) >= 0)){
                    // si crea il percorso aggiungendo il nome di quel file e si legge
                    sprintf(date_var, "%d-%d-%d", tm.tm_mday, ((tm.tm_mon) +1), ((tm.tm_year)+1900));   

                    // si riordinano le strutture in base all'elemento diff_to_begin della struct
                    // in quanto non possiamo essere sicuri dell-ordine in cui verrà letta
                    // la struct  
                    insert_slot = 0;
                    // se l'array non è vuoto 
                    if(counter_date != 0){
                        // si scorre l'array fino al numero di elementi in quel momento
                        for(j = 0; j<=i; j++){
                            // se la differenza con la data di partenza
                            // è maggiore di quella attuale oppure non ho raggiunto l'ultimo elemento
                            // si incrementa la posizione di inserimento
                            if((difftime(to_open, start))>var_date[j].diff_to_begin && j!=counter_date){
                                insert_slot++;
                            }
                            else{
                                break;
                            }
                        }
                        // si aggiustano le posizione degli elementi successivi
                        for(w=i; w>insert_slot; w--){
                            var_date[w].diff_to_begin = var_date[w-1].diff_to_begin;
                            strcpy(var_date[w].date, var_date[w-1].date);
                            var_date[w].data_read = var_date[w-1].data_read;
                        }
                    }
                    // inserimento ordinato nell'array di struct
                    var_date[insert_slot].diff_to_begin = difftime(to_open, start);
                    strcpy(var_date[insert_slot].date, date_var);

                    // si riazzera perchè questa operazione deve essere ripetuta un tot di volte
                    // e sennò continueremmo a concatenare i nomi non trovando nulla
                    memset(path_completo_register, 0, sizeof(path_completo_register));
                    strcat(path_completo_register, path_register);
                    strcat(path_completo_register, toopen_date);   
read_var:
                    is_toread = read_file(path_completo_register, elements);
                    if(!is_toread){      
                        printf("Non è stato possibile leggere questo file\n");
                        create_file_register(peer, toopen_date, 0, 0, 0, 0);
                        goto read_var;
                    }

                    if(strcmp(type, "T") == 0){
                        type_toread = 2;
                    }
                    else{
                        type_toread = 3;
                    }

                    valore_entry_to_add = get_number_entry(type_toread, 3, elements);
                    if(valore_entry_to_add == 0){
                        result_conn_tcp = false;
                        // bisogna chiedere il dato aggr ai vicini
                        if(!already_asked_aggr){
                            for(n=0; n < N_NEIGHBOR; n++){
                                result_conn_tcp = create_tcp_connection(neighbors[n], 'V', info_tcp, 0, numero_file_period, name_file, &entry_from_peers, elements_var);
                                if(result_conn_tcp){
                                    goto save;
                                }
                            }
                            already_asked_aggr = true;
                        }
                        // bisogna creare una connessione tcp e prendere questo dato che è il valore complessivo di tutte le entry per quel
                        // particolare register di quel giorno
                        for(n=0; n < N_NEIGHBOR; n++){
                            result_conn_tcp = create_tcp_connection(neighbors[n], 'P', info_tcp, 0, 0, toopen_date, &entry_from_peers, elements_var);
                            if(result_conn_tcp){
                                valore_entry_to_add = entry_from_peers;
                                break;
                            }
                        }  
                        if(!result_conn_tcp){
                            
                            if(strcmp(type, "T") == 0){
                                type_toread = 0;
                            }
                            else{
                                type_toread = 1;
                            }

                            valore_entry_to_add = get_number_entry(type_toread, 2, elements);
                            result_conn_tcp = create_tcp_connection(neighbors[1], 'F', info_tcp, peer, valore_entry_to_add, toopen_date, &entry_from_peers, elements_var);
                            valore_entry_to_add = entry_from_peers;
                            
                        }
                        if(strcmp(type, "T") == 0){
                            sprintf(elements[2], "TT:%d;\n", valore_entry_to_add);
                        }
                        else{
                            sprintf(elements[3], "TN:%d;", valore_entry_to_add);
                        }
                    
                        // poi dobbiamo far scrivere questo valore nel registro di questo peer 
                        // perchè va salvato nel register del peer     
                        
                        is_towrite = write_file(path_completo_register, elements);
                        if(!is_towrite){      
                            printf("Non è stato possibile aggiornare/creare la entry\n");
                        }
                    }

                    var_date[insert_slot].data_read = valore_entry_to_add; 
                    // incremento del numero di date
                    counter_date++;
                    i++; 
                }  
                else{
                    break;
                }          
            }

            for(i = 0; i < numero_file_period-1; i++){
                variation[i] = var_date[i+1].data_read - var_date[i].data_read ;
                sprintf(elements_var[i], "%s_%s: %d\n", var_date[i].date, var_date[i+1].date, variation[i]);   
            }
save:  
            is_towrite = write_file_aggr(path_completo_aggr, numero_file_period-1, elements_var);
            if(!is_towrite){      
                printf("Non è stato possibile aggiornare/creare la entry\n");
            }
            printf("Dato aggregato non presente, creato con successo\n");   
            for(i = 0; i < numero_file_period-1; i++){
                printf("%s",elements_var[i]);   
            }
            printf("\n");
        }
    }
    
}

/*****************************************************************************************
funzione che crea un piccolo database con devi valori per fare i test di funzionamento 
viene eseguita solo una volta, è cioè dal primo peer che si connette alla rete
e crea un db per i peer 5001....5005
parametri:
@ port_peer è il peer attuale
@ neighbors è un array con il numero di porta dei vicini
@ db_is_to_create serve a controllare se il db è già stato creato
@ path_register è il percorso che contiene i register del peer
@ path_aggr_t è il percorso che contiene i dati aggregati del peer per i tamponi
@ path_aggr_c è il percorso che contiene i dati aggregati del peer per i casi
****************************************************************************************/
void create_db(unsigned short port_peer, unsigned short *neighbors, bool db_is_to_create, char *path_register, char *path_aggr_t, char *path_aggr_c){
    
    bool directory_created = false;
    // variabili per loop 
    int i, j=0;
    // variabili per dati
    char *dates[] = {"9-4-2021.txt", "10-4-2021.txt", "11-4-2021.txt", "12-4-2021.txt"};
    int t,n,tt = 1675,tn = 717;
    //int t,n,tt = 0,tn = 0;
    char path_register_temp[PATH_REGISTER];
    char path_aggr_t_temp[PATH_AGGR_T];
    char path_aggr_c_temp[PATH_AGGR_C];

    if((!db_is_to_create) && (neighbors[0] == 0)){
        for(j=0; j<4; j++){
            for(i = 5001; i < 5006; i++){
                switch (i){
                    case 5001:
                        t = 10;
                        n = 7;
                        break;
                    case 5002:
                        t = 750;
                        n = 500;
                        break;
                    case 5003:
                        t = 300;
                        n = 100;
                        break;
                    case 5004:
                        t = 550;
                        n = 70;
                        break;
                    case 5005:
                        t = 65;
                        n = 40;
                        break;                  
                }
                if(j==0){
                    directory_created = create_folders(i, path_register_temp, path_aggr_t_temp, path_aggr_c_temp);
                    if(!directory_created){
                        printf("Directory relative al peer %d già presenti\n", i);
                    }
                }
                if(i == 5002){
                    if(j == 3){
                        create_file_register(i,dates[j], t, n, tt, 0);
                    }      
                    else{
                        create_file_register(i,dates[j], t, n, 0, 0);
                    }          
                }
                else if(i == 5003){
                    if(j == 1){
                        create_file_register(i,dates[j], t, n, tt, tn);
                    }
                    else{
                        create_file_register(i,dates[j], t, n, 0, 0);
                    } 
                }
                else if(i == 5004){
                    if(j == 0){
                        create_file_register(i,dates[j], t, n, 0, tn);
                    }
                    else{
                        create_file_register(i,dates[j], t, n, 0, 0);
                    }
                } 
                else{
                    create_file_register(i,dates[j], t, n, 0, 0);
                    if(j == 2 && i == 5005){

                        create_file_register(i,dates[j], 165, n, 0, 0);
                    }
                }              
            }
        }
        db_is_to_create = true;
    }
}

/******************************************************************************
funzione che disconnette il peer dalla rete, e provoca l'invio ai neighbor
di tutte le entry registarte dall'avvio
parametri:
@ socket_peer è il socket utilizzato per dialogare con il DS usando UDP
@ indirizzo_socket_ds è la struct sockaddr_in relativa al ds
@ boot è la struttura messaggio per inviare i valori per la disconnessione
@ ready è il set di descrittori pronti, per il reinvio nel caso di dati 
        persi visto che si utilizza UDP
@ time_boot è il tempo che bisogna aspettare per il reinvio dei dati 
@ fdmax è il valore del descrittore più grande
@ buffer_ports è un buffer per l'ACK
@ addrlen_ds è sizeof(indirizzo_socket_ds)
@ neighbors è un array con il numero di porta dei vicini
******************************************************************************/
void stop(int socket_peer, struct sockaddr_in indirizzo_socket_ds, struct message* boot, fd_set ready, struct timeval time_boot, int fdmax, char * buffer_ports, socklen_t addrlen_ds, unsigned short *neighbors){

    int ret, i, j;
    bool is_toread = false;
    bool result_conn_tcp = false;
    //char path[PATH_COMPLETE];
    char elements[N_LINES][LINE_SIZE]; 
    int current_date[4]; 
    char **elements_var = NULL;  
    char **elements_tot = NULL;  
    // array di appoggio per la costruzione del path completo, per accedere al file
    // e variabili per accedere alla cartella
    char path_completo_register[PATH_COMPLETE];
    char path_register[PATH_REGISTER];
    char path[PATH_COMPLETE];
    //char name_file[NAME_AGGR_LEN];
    DIR *directory_register;
    struct dirent *directory_entry; 
    char info_tcp[LEN_INFO_TCP];
    int value_to_send = 0;
    unsigned short peer = ntohs(boot->port);
    int entry_from_peers = 0;
    int type_toread;
    unsigned short not_flood = 0;
    bool is_towrite = false;
    int number_entry = 0;
    // strutture e variabili per la gestione del tempo
    struct tm *timeinfo;
    struct tm *to_open_info;
    struct tm tm = {0};
    time_t to_open, saved, today;


    printf("-------------------------------------------------------------------------\n");
    printf("------------------- RICHIESTA DISCONNESSIONE INIZIATA -------------------\n");
    printf("-------------------------------------------------------------------------\n\n");
    
    time(&today);
    timeinfo = localtime(&today);
    current_date[0] = timeinfo->tm_mday;
    current_date[1] = timeinfo->tm_mon;
    current_date[2] = timeinfo->tm_year;
    current_date[3] = timeinfo->tm_hour;

    if(neighbors[0] == 0){
        goto ds_part;
    }
    
    memset(path, 0, sizeof(path));
    sprintf(path, "files/%d/last_register_saved.txt", peer);
    
    memset(path_register, 0, sizeof(path_register));
    sprintf(path_register, "files/%d/register/", peer);

    elements_tot = malloc(1 * sizeof(char*));
    elements_tot[0] = malloc((LINE_SIZE+1) * sizeof(char));

    
    is_toread = read_file(path, elements);
    if(is_toread){
        // si trasforma il nome del file in una data per essere confrontata
        if (strptime(elements[0], "%d-%m-%Y", &tm) == NULL){
            printf("errore nella creazione della data\n");
        }   
    }
    else{
        printf("Nessun dato dell'ultimo salvataggio effettutato\n");
        // si trasforma il nome del file in una data per essere confrontata
        if (strptime("04-03-2020", "%d-%m-%Y", &tm) == NULL){
            printf("errore nella creazione della data\n");
        }

        sprintf(elements_tot[0], "%d-%d-%d", timeinfo->tm_mday, ((timeinfo->tm_mon) +1), ((timeinfo->tm_year)+1900));

        is_towrite = write_file_aggr(path, 1, elements_tot);
        if(!is_towrite){      
            printf("Non è stato possibile aggiornare/creare la entry\n");
        }
    }

    saved = mktime(&tm);
    if (saved == -1){
        printf("errore nella mktime\n");
    }  

    // dobbiamo contare il numero di registri compresi nel periodo indicato nello stream di ingresso
    // creo il percorso per la directory register del peer
    memset(path_completo_register, 0, sizeof(path_completo_register));
    strcat(path_completo_register, path_register);
    // apro la cartella del register, dove ci sono tutti i register distrinti per data
    directory_register = opendir(path_completo_register); 
    if (directory_register == NULL){ 
        printf("Non è stato possibile trovare il registo del peer" ); 
    }     

    // scorriamo tutti i file, finchè non li abbiamo esaminati tutti
    while ((directory_entry = readdir(directory_register)) != NULL){
        if((strcmp(directory_entry->d_name,".") == 0) || (strcmp(directory_entry->d_name,"..") == 0)){
            continue;
        }
        
        // si trasforma il nome del file in una data per essere confrontata
        if (strptime(directory_entry->d_name, "%d-%m-%Y.txt", &tm) == NULL){
            printf("errore nella creazione della data\n");
        }
        to_open = mktime(&tm);
        if (to_open == -1){
            printf("errore nella mktime\n");
        }  
        to_open_info = localtime(&to_open);

        if(difftime(to_open, saved) >= 0){
            
            // si crea il percorso aggiungendo il nome di quel file e si legge
            memset(path_completo_register, 0, sizeof(path_completo_register));
            strcat(path_completo_register, path_register);
            strcat(path_completo_register, directory_entry->d_name);
            is_toread = read_file(path_completo_register, elements);
            if(!is_toread){      
                printf("Non è stato possibile leggere questo file\n");
            }
            else{             
                for(i=0; i < N_NEIGHBOR; i++){
                    for(j=0; j < 2; j++){
                        if(j == 0){
                            strcpy(info_tcp, "T");
                        }
                        else{
                            strcpy(info_tcp, "N");
                        }
                        if((current_date[0] == to_open_info->tm_mday) && (current_date[1] == to_open_info->tm_mon) && (current_date[2] == to_open_info->tm_year)){
                            if(current_date[3] < 18){
                                type_toread = j;

                                value_to_send = get_number_entry(type_toread, 2, elements);
                                not_flood = 1;
                                if(j == 0){
                                    sprintf(elements[0], "T:%d;\n", 0);
                                }
                                else{
                                    sprintf(elements[1], "N:%d;\n", 0);
                                }
                                is_towrite = write_file(path_completo_register, elements);
                                if(!is_towrite){      
                                    printf("Non è stato possibile aggiornare/creare la entry\n");
                                }
                                goto saving;
                            }
                        }
                        type_toread = j+2;

                        number_entry = get_number_entry(type_toread, 3, elements);
                        if(number_entry != 0){
                            value_to_send = number_entry;
                        }        
                        else{
                        
                            type_toread = j;

                            value_to_send = get_number_entry(type_toread, 2, elements);
                            result_conn_tcp = create_tcp_connection(neighbors[1], 'F', info_tcp, peer, value_to_send, directory_entry->d_name, &entry_from_peers, elements_var);
                            value_to_send = entry_from_peers;
                            
                            if(j == 0){
                                sprintf(elements[2], "TT:%d;\n", value_to_send);
                            }
                            else{
                                sprintf(elements[3], "TN:%d;", value_to_send);
                            }

                            is_towrite = write_file(path_completo_register, elements);
                            if(!is_towrite){      
                                printf("Non è stato possibile aggiornare/creare la entry\n");
                            }
                        }
                        
saving:                       
                        result_conn_tcp = create_tcp_connection(neighbors[i], 'S', info_tcp, not_flood, value_to_send, directory_entry->d_name, &entry_from_peers, elements_var);

                        if(result_conn_tcp){        
                            printf("Dato di tipo %s del register %s salvate nel register del vicino %hu\n\n", info_tcp, directory_entry->d_name, neighbors[i]);
                        }            
                    }

                    if(neighbors[0] == neighbors[1] || not_flood == 1){
                        break;
                    }
                }
            }
        }
    }

ds_part:
    // invio al ds dell'indirizzo IP del peer
    do{
        do{
            ret = sendto(socket_peer, &(boot->type), INET_ADDRSTRLEN, 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));	
        } while(ret < 0);
        printf("[INVIO TIPOLOGIA RICHIESTA]: Disconnessione peer\n");

    }while(select(fdmax + 1, &ready, NULL, NULL, &time_boot) < 0);  
    
    // invio al ds dell'indirizzo IP del peer
    do{
        do{
            ret = sendto(socket_peer, boot->address, INET_ADDRSTRLEN, 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));	
        } while(ret < 0);
        printf("[   INVIO INDIRIZZO PEER  ]: %s\n", boot->address);

    }while(select(fdmax + 1, &ready, NULL, NULL, &time_boot) < 0);   

    // invio del numero di porta del peer al ds per la disconnesione
    do{
        do{
            ret = sendto(socket_peer, &(boot->port), sizeof(uint16_t), 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));
            if(ret < 0){
                perror("[ ATTESA INVIO PORTA PEER ] tentativo di reinvio in corso...\n");	
            }	
        } while(ret < 0);
        printf("[     INVIO PORTA PEER    ]: %hu\n", ntohs(boot->port));

    }while(select(fdmax + 1, &ready, NULL, NULL, &time_boot) < 0); 

    // ricezione dell'ACK di corretta registrazione avvenuta
    do{
        ret = recvfrom(socket_peer, buffer_ports, RESPONSE_LEN, 0,  (struct sockaddr *)&indirizzo_socket_ds, &addrlen_ds);
        if(ret < 0){
            perror("[ ATTESA DISCONNECT PEER  ]: registrazione e invio dei vicini in corso...\n");	
        }	
    }while(ret < 0);
    printf("[       RICEZIONE ACK     ]: %s\n", buffer_ports); 

    printf("-------------------------------------------------------------------------\n");
    printf("------------------- RICHIESTA DISCONNESSIONE TERMINATA ------------------\n");
    printf("-------------------------------------------------------------------------\n");
}


int main(int argc, char* argv[]){
    
    // variabili per connessioni
    int ret, socket_peer, listener;   
    int comunication;
    socklen_t addrlen_ds, len;
	struct sockaddr_in indirizzo_socket_peer, indirizzo_socket_ds;
    struct sockaddr_in indirizzo_socket_neighbor;
    // variabili di appoggio per l'utilizzo delle select
    fd_set tocheck;
    fd_set ready;
    int fdmax;
    struct timeval time_boot = {1, 0}; // attesa di 1 sec
    // buffer vari
    struct message *mes;
    char buffer_ports[BUFFER_SIZE];	
    char buffer[BUFFER_STREAM_INPUT];	
    char received[BUFFER_SIZE];	
    char parsed_stream[4][23];
    char *response = "Neighbors aggiornati correttamente\0";
    // percorsi per le cartelle conteneti file
    char path_register[PATH_REGISTER];
    char path_aggr_t[PATH_AGGR_T];
    char path_aggr_c[PATH_AGGR_C];
    char path_for_files[PATH_COMPLETE];
    int number_entry = 0;
    // counter vari e var di controllo
    int i,j;
    int n_words = 0;
	int result;
    bool db_is_to_create = false;
    bool directory_created = false;
    bool stream_cmd = false;
    bool time_expired = false;
    bool stop_get = false;
    // variabili di porte e address dei soggetti
    unsigned short port_peer = 0;
    char address_peer[INET_ADDRSTRLEN] = "127.0.0.1";
    unsigned short port_ds = 0;
    char address_ds[INET_ADDRSTRLEN];
    unsigned short neighbors[N_NEIGHBOR];	
    unsigned short neighbor_toadd; 
    char type_udp;
    // variabili per dialogo connessioni tcp 
    char type_req;
    char info[LEN_INFO_TCP];
    char payload[NAME_AGGR_LEN];
    unsigned short port_req;
    int value_temp = 0;
    bool is_toread = false;
    bool is_towrite = false;
    char elements[N_LINES][LINE_SIZE];    // questo per prelevare i dati dai register
    int tcp_answer = -1;
    char quantity_read[6];  // array temporaneo in cui depositare il segmento di data prelevato
    int type_toread;
    int counter_char = 3; 
    int total_entry = -1;
    char **elements_var = NULL;
    int entry_from_peers = -1;
    int numero_rec_var = 0;
    char single_var[(LINE_SIZE_VAR+1) * sizeof(char)];
    struct tm *timeinfo;
    time_t today;
    char *position_ast;
    int pos_ast_n;
    bool display_message = true;

    if(check_port(argv[1])){
        port_peer = (unsigned short)atoi(argv[1]);   
    }
    else{
        perror("Numero di Porta inserito non valido");
    }

    socket_peer = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);	
    listener = socket(AF_INET, SOCK_STREAM, 0);	

    memset(&indirizzo_socket_peer, 0, sizeof(indirizzo_socket_peer));
    indirizzo_socket_peer.sin_family = AF_INET;
    indirizzo_socket_peer.sin_port = htons(port_peer);
    indirizzo_socket_peer.sin_addr.s_addr = INADDR_ANY;
    
    // eseguo la bind per il socket udp
    ret = bind(socket_peer, (struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
    if(ret < 0){
        perror("Bind non riuscita\n");
        exit(0);
    }

    // eseguo la bind per il socket tcp
    ret = bind(listener, (struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
    if(ret < 0){
        perror("Bind non riuscita\n");
        exit(0);
    }
	listen(listener, 10);		

    FD_ZERO(&tocheck);
    FD_ZERO(&ready);

    FD_SET(socket_peer, &tocheck);
    FD_SET(listener, &tocheck);
    FD_SET(STDIN_FILENO, &tocheck);
    
    if(socket_peer > listener){
        fdmax = socket_peer;
    }
    else{
        fdmax = listener;
    }

    while(1){
        
        if(display_message){
            starting_message_peer();
        }
        
        ready = tocheck;      
        if(select(fdmax + 1, &ready, NULL, NULL, NULL)){
            for(i = 0; i<=fdmax; i++){
                if(FD_ISSET(i, &ready)){
                    if(i == socket_peer){
                        printf("***************************** UPDATE NEIGHBORS ************************************\n");
                        printf("-------------------------------------------------------------------------\n");
                        printf("------------------------ INIZIO FASE DI AGGIORNAMENTO -------------------\n");
                        printf("-------------------------------------------------------------------------\n");
                        for(j=0; j<2; j++){   
                            do{
                                ret = recvfrom(socket_peer, &type_udp, sizeof(uint8_t), 0,  (struct sockaddr *)&indirizzo_socket_ds, &addrlen_ds);
                                
                            }while(ret < 0);         
                            if(type_udp == 'D'){

                                // ricezione dell-indirizzo IP del peer da registrare
                                do{
                                    ret = recvfrom(socket_peer, received, BUFFER_SIZE, 0, (struct sockaddr *)&indirizzo_socket_ds, &addrlen_ds);
                                }while(ret < 0);
                                printf("[ RICEZIONE INDIRIZZO PEER ]: %s\n", received);
                                goto fine_udp;
                            }       
                            do{
                                ret = recvfrom(socket_peer, &neighbor_toadd, sizeof(uint16_t), 0,  (struct sockaddr *)&indirizzo_socket_ds, &addrlen_ds);
                               
                            }while(ret < 0);
                            neighbor_toadd= ntohs(neighbor_toadd);
                            if(j == 0){
                                
                                printf("[ RICEZIONE PRIMO VICINO  ]: %hu\n", neighbor_toadd);    
                            }
                            else{
                                printf("[RICEZIONE SECONDO VICINO ]: %hu\n", neighbor_toadd);    
                            }   
                            neighbors[j] = neighbor_toadd;                                                   	
                        }


                        // invio di ack per indicare avvenuta registrazione
                        do{
                            ret = sendto(socket_peer, response, 36, 0, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));	
                            if(ret < 0){
                                perror("[    ATTESA INVIO ACK     ] tentativo di reinvio in corso...\n");	
                            }
                        }while(ret < 0);
fine_udp:               
                        printf("-------------------------------------------------------------------------\n");
                        printf("------------------------ FINE FASE DI AGGIORNAMENTO ---------------------\n");
                        printf("-------------------------------------------------------------------------\n");          
                        printf("***********************************************************************************\n\n");                     

                    }
                    else if(i == listener){             
                        printf("\n");
                        printf("*************************** SCAMBIO DATI CON GLI ALTRI PEER ****************************\n");                                   
                        printf("1) -------------- FASE DI CONNESSIONE -------------------------\n");
                        len = sizeof(indirizzo_socket_neighbor);
                        comunication = accept(listener, (struct sockaddr *)&indirizzo_socket_neighbor, &len);
                        printf("RICHIESTA CONNESSIONE TCP RICEVUTA DA UN NEIGHBOR\n");
                        
                        FD_SET(comunication, &tocheck);
                        
                        if(comunication > fdmax){
                            fdmax = comunication;
                        }

                        display_message = false; 
                    
                    }
                    else if(i == comunication && comunication!=0){
                        
                        printf("1) -------------- FASE DI SCAMBIO DATI ------------------------\n");
                        memset(elements, 0, sizeof(elements));
                        tcp_answer = -1;
                        number_entry = 0;
                        

                        ret = recv(i, &type_req, sizeof(uint8_t), 0);
                        if(ret == 0){
                            printf(" nessun messaggio ricevuto\n");
                        }

                        ret = recv(i, info, LEN_INFO_TCP, 0);
                        if(ret == 0){
                            printf(" nessun messaggio ricevuto\n");
                        }

                        ret = recv(i, payload, NAME_AGGR_LEN, 0);
                        if(ret == 0){
                            printf(" nessun messaggio ricevuto\n");
                        }

                        
                        if(type_req == 'T'){
                            printf("********** Richiesta Aggregato Totale per %s **********\n", payload);
                            memset(path_for_files, 0, sizeof(path_for_files));
                            if(strcmp(info, "T") == 0){
                                sprintf(path_for_files, "files/%hu/aggr/tamponi/", port_peer);
                            }
                            else{
                                sprintf(path_for_files, "files/%hu/aggr/casi/", port_peer);
                            }

                            strcat(path_for_files, payload);
                            is_toread = read_file(path_for_files, elements);                 
                            if(is_toread){
                                tcp_answer = atoi(elements[0]);
                                printf("DATO AGGREGATO PER IL TOTALE PRESENTE ED INVIATO\n");
                            }
                            else{
                                printf("DATO AGGREGATO PER IL TOTALE NON PRESENTE\n");
                            }

                            tcp_answer = htonl(tcp_answer);
                            ret = send(i, &tcp_answer, sizeof(tcp_answer), 0);
                            if(ret < 0){
                                perror("Errore in fase di invio: \n");	
                            }	
                        }
                        if(type_req == 'V'){
                            printf("******** Richiesta Aggregato Variazione per %s **********\n", payload);
                            ret = recv(i, &value_temp, sizeof(uint32_t), 0);
                            if(ret == 0){
                                printf(" nessun messaggio ricevuto\n");
                            }
                            numero_rec_var= ntohl(value_temp);
                            elements_var = malloc(numero_rec_var * sizeof(char*));
                            for(j = 0; j< numero_rec_var; j++){
                                elements_var[j] = malloc((LINE_SIZE_VAR+1) * sizeof(char));
                            }
                            memset(path_for_files, 0, sizeof(path_for_files));
                            if(strcmp(info, "T") == 0){
                                sprintf(path_for_files, "files/%hu/aggr/tamponi/", port_peer);
                            }
                            else{
                                sprintf(path_for_files, "files/%hu/aggr/casi/", port_peer);
                            }
                            strcat(path_for_files, payload);
                            is_toread = read_file_var(path_for_files, numero_rec_var, elements_var);   
                            if(is_toread){
                                for(j = 0; j < numero_rec_var; j++){
                                    sprintf(single_var, "%s", elements_var[j]);
                                    ret = send(i, single_var, LINE_SIZE_VAR, 0);
                                    if(ret < 0){
                                        perror("Errore in fase di invio: \n");	
                                    } 
                                }
                                printf("DATO AGGREGATO PER LA VARIAZIONE PRESENTE ED INVIATO\n");
                            }
                            else{
                                strcpy(single_var, "NO");
                                printf("DATO AGGREGATO PER LA VARIAZIONE NON PRESENTE\n");
                                ret = send(i, single_var, LINE_SIZE_VAR, 0);
                                if(ret < 0){
                                    perror("Errore in fase di invio: \n");	
                                } 
                            }
                            for (j = 0; j < numero_rec_var; j++) { 
                                free(elements_var[j]);
                            }
                            free(elements_var);
                        }
                        if(type_req == 'P'){
                            printf("********* Richista entry complessive per %s **********\n", payload);
                            total_entry = -1;
                            memset(path_for_files, 0, sizeof(path_for_files));
                            sprintf(path_for_files, "files/%hu/register/", port_peer);
                            strcat(path_for_files, payload);   


                            is_toread = read_file(path_for_files, elements);
                            if(is_toread){
                                if(strcmp(info, "T") == 0){
                                    type_toread = 2;
                                }
                                else{
                                    type_toread = 3;
                                }

                                number_entry = get_number_entry(type_toread, 3, elements);
                                if(number_entry != 0){
                                    total_entry = number_entry;
                                    printf("DATO PER LE ENTRY COMPLESSIVE PRESENTE ED INVIATO\n"); 
                                }  
                                else{
                                    printf("DATO PER LE ENTRY COMPLESSIVE NON PRESENTE\n");
                                }                                                          
                            }
                            
                            tcp_answer = htonl(total_entry);
                            ret = send(i, &tcp_answer, sizeof(tcp_answer), 0);
                            if(ret < 0){
                                perror("Errore in fase di invio: \n");	
                            }	
                        }
                        if(type_req == 'S'){             
                            
                            printf("****** Ricezione entry per salvataggio per %s **********\n", payload);

                            ret = recv(i, &port_req, sizeof(uint16_t), 0);
                            if(ret == 0){
                                printf(" nessun messaggio ricevuto\n");
                            }
                            port_req= ntohs(port_req);

                            ret = recv(i, &value_temp, sizeof(uint32_t), 0);
                            if(ret == 0){
                                printf(" nessun messaggio ricevuto\n");
                            }
                            value_temp= ntohl(value_temp);
                        
                            memset(path_for_files, 0, sizeof(path_for_files));
                            sprintf(path_for_files, "files/%hu/register/", port_peer);
                            strcat(path_for_files, payload);   
                            
                            is_toread = read_file(path_for_files, elements);
                            if(is_toread){
                                if(strcmp(info, "T") == 0){
                                    if(port_req == 1){
                                        type_toread = 0;
                                    }
                                    type_toread = 2;
                                }
                                else{
                                    if(port_req == 1){
                                        type_toread = 1;
                                    }
                                    type_toread = 3;
                                }                           
                                if(port_req == 1){
                                    counter_char = 2;
                                }
                                else{
                                    counter_char = 3;
                                }

                                number_entry = get_number_entry(type_toread, counter_char, elements);
                                
                                if(port_req == 1){
                                    if(strcmp(info, "T") == 0){
                                        sprintf(elements[0], "T:%d;\n", atoi(quantity_read)+ value_temp);
                                    }
                                    else{
                                        sprintf(elements[1], "N:%d;\n", atoi(quantity_read)+ value_temp);
                                    }
                                }
                                else{
                                    if(strcmp(info, "T") == 0){
                                        sprintf(elements[2], "TT:%d;\n", atoi(quantity_read)+ value_temp);
                                    }
                                    else{
                                        sprintf(elements[3], "TN:%d;", atoi(quantity_read)+ value_temp);
                                    }
                                }


                                is_towrite = write_file(path_for_files, elements);
                                if(!is_towrite){      
                                    printf("Non è stato possibile aggiornare/creare la entry\n");
                                }

                            }
                            else{
                                if(port_req == 1){
                                     if(strcmp(info, "T") == 0){
                                        create_file_register(port_peer, payload, value_temp, 0, 0, 0);
                                    }
                                    else{
                                        create_file_register(port_peer, payload, 0, value_temp, 0, 0);
                                    }    
                                }
                                else{
                                    if(strcmp(info, "T") == 0){
                                        create_file_register(port_peer, payload, 0, 0, value_temp, 0);
                                    }
                                    else{
                                        create_file_register(port_peer, payload, 0, 0, 0, value_temp);
                                    }      
                                }                   
                            }
                            printf("ENTRY DEL VICINO DISCONESSO SALVATE CON SUCCESSO\n");

                        }
                        if(type_req == 'F'){
                            printf("*********** Richiesta di flooding per %s **********\n", payload);
                            ret = recv(i, &port_req, sizeof(uint16_t), 0);
                            if(ret == 0){
                                printf(" nessun messaggio ricevuto\n");
                            }
                            port_req= ntohs(port_req);

                            ret = recv(i, &value_temp, sizeof(uint32_t), 0);
                            if(ret == 0){
                                printf(" nessun messaggio ricevuto\n");
                            }
                            value_temp= ntohl(value_temp);
                            

                            memset(path_for_files, 0, sizeof(path_for_files));
                            sprintf(path_for_files, "files/%hu/register/", port_peer);
                            strcat(path_for_files, payload);   
                            
                            total_entry = -1;
                            is_toread = read_file(path_for_files, elements);
                            if(is_toread){
                                if(strcmp(info, "T") == 0){
                                    type_toread = 2;
                                }
                                else{
                                    type_toread = 3;
                                }

                                number_entry = get_number_entry(type_toread, 3, elements);
                                if(number_entry != 0){
                                    total_entry = number_entry;
                                }
                                else{
                                    if(strcmp(info, "T") == 0){
                                        type_toread = 0;
                                    }
                                    else{
                                        type_toread = 1;
                                    }

                                    number_entry = get_number_entry(type_toread, 2, elements);                     
                                    value_temp = value_temp + number_entry;
                                }
                            }
                            tcp_answer = value_temp;

                            if(total_entry != -1){
                                printf("DATO PER LE ENTRY COMPLESSIVE PRESENTE\nSTOP AL FLOODING\n");
                                tcp_answer = total_entry;
                            }

                            if(total_entry == -1 && neighbors[1] != port_req){
                                printf("DATO PER LE ENTRY COMPLESSIVE NON ANCORA CALCOLATO\nFLOODING CONTINUA\n\n");
                                create_tcp_connection(neighbors[1], type_req, info, port_req, value_temp, payload, &entry_from_peers, elements_var);                  
                                tcp_answer = entry_from_peers;
                                
                            }        
                            else{
                                printf("DATO PER LE ENTRY COMPLESSIVE CALCOLATO\nSTOP AL FLOODING\n");
                            }
    
                            
                            tcp_answer = htonl(tcp_answer);
                            ret = send(i, &tcp_answer, sizeof(tcp_answer), 0);
                            if(ret < 0){
                                perror("Errore in fase di invio: \n");	
                            }	  
                            
                        }

                        

                        memset(info, 0, sizeof(info));
                        memset(payload, 0, sizeof(payload));

                        // chiudo il socket i-esimo per la comunicazione
                        close(i);
                        // rimuovo il descrittore dal set da controllare
                        FD_CLR(i, &tocheck);

                        display_message = true;

                        printf("****************************************************************************************\n\n");

                    }
                    else if (i == STDIN_FILENO){
                        memset(buffer, 0, sizeof(buffer));
                        fgets(buffer, BUFFER_STREAM_INPUT, stdin);	// legge e copia l'intera riga (compreso il new line)
                        buffer[strlen(buffer)-1] = '\0';
                        memset(parsed_stream, 0, sizeof(parsed_stream));
                        stream_cmd = parse_stream(buffer, strlen(buffer), &n_words, parsed_stream);      
                        if(!stream_cmd){
                            fprintf(stderr, "**********************************************************\n"
                                            "Errore: nessun comando digitato\n"
                                            "Si prega di riprovare l'inserimento: \n"
                                            "**********************************************************\n\n");
                        }
                        result = validate_parsed_stream(parsed_stream, n_words);
                        
                        if(result < 0){
                            
                            fprintf(stderr, "**********************************************************\n"
                                            "Errore: il comando digitato non e' stato riconosciuto\n"
                                            "Si prega di riprovare l'inserimento: \n"
                                            "**********************************************************\n\n");
                            fflush(stderr);
                            continue;
                        }
                        

                        switch(result){
                            case P_START:
    
                                port_ds = (unsigned short)atoi(parsed_stream[2]);
                                strcpy(address_ds, parsed_stream[1]);
                                printf("********************************** START ******************************************\n"); 
                                mes = create_message('B', NULL, (uint16_t)port_peer, (uint8_t *)address_peer, 0, NULL);

                                memset(&indirizzo_socket_ds, 0, sizeof(indirizzo_socket_ds));
                                indirizzo_socket_ds.sin_family = AF_INET;
                                indirizzo_socket_ds.sin_port = htons(port_ds);
                                inet_pton(AF_INET, address_ds, &indirizzo_socket_ds.sin_addr);

                                start(socket_peer, indirizzo_socket_ds, mes, ready, time_boot, fdmax, buffer_ports, addrlen_ds, neighbors);
                                
                                free(mes);
                                create_db(port_peer, neighbors, db_is_to_create, path_register, path_aggr_t, path_aggr_c);
                                directory_created = create_folders(port_peer, path_register, path_aggr_t, path_aggr_c);
                                if(!directory_created){
                                    printf("Directory relative al peer %hu già creata\n", port_peer);
                                }


                                printf("***********************************************************************************\n\n"); 
                                break;
                    
                            case P_ADD:

                                printf("************************************* ADD ******************************************\n"); 
                                if(port_ds == 0){
                                    printf("Bisogna prima registrarsi per poter aggiungere entry\n"); 
                                }
                                else{
                                    add(port_peer, path_register, parsed_stream[1], atoi(parsed_stream[2]));
                                }
                                printf("***********************************************************************************\n\n"); 
                                break;                               
              
                            case P_GET:

                                printf("************************************* GET ******************************************\n");
                                if(port_ds == 0){
                                    printf("Bisogna prima registrarsi per poter aggiungere entry\n"); 
                                }
                                else{

                                    time(&today);
                                    timeinfo = localtime(&today);
                                    if(timeinfo->tm_hour < 18){
                                        time_expired = true;
                                    }

                                    if(n_words == 3){
                                        sprintf(parsed_stream[3], "04:03:2020-%d:%d:%d", timeinfo->tm_mday, ((timeinfo->tm_mon) +1), ((timeinfo->tm_year)+1900));
                                        if(time_expired){
                                            stop_get = true;
                                        }
                                    }

                                    position_ast = strchr(parsed_stream[3], '*');
                                    if (position_ast != NULL){
                                        pos_ast_n = (int)(position_ast - parsed_stream[3]);
                                        if(pos_ast_n == 11 && time_expired){
                                            
                                            stop_get = true;
                                        }
                                    }                    
                                    
                                    if(!stop_get){
                                        if(strcmp(parsed_stream[2],"T") == 0){                                       
                                            get(port_peer, path_register, path_aggr_t, parsed_stream[1], parsed_stream[2], parsed_stream[3], neighbors);
                                        }
                                        else{
                                            get(port_peer, path_register, path_aggr_c, parsed_stream[1], parsed_stream[2], parsed_stream[3], neighbors);
                                        }
                                    }
                                    else{
                                        printf("Operazione non ancora consentita, riprovare dopo le 18\n"); 
                                    }

                                    stop_get = false;
                                    
                                }
                                printf("***********************************************************************************\n\n"); 
                                break;
                    
                            case P_STOP:

                                printf("********************************** STOP ******************************************\n"); 
                                if(port_ds == 0){
                                    printf("Bisogna prima registrarsi per potersi disconnettere\n"); 
                                }
                                else{                        
                                    mes = create_message('D', NULL, (uint16_t)port_peer, (uint8_t *)address_peer, 0, NULL);
                                    stop(socket_peer, indirizzo_socket_ds, mes, ready, time_boot, fdmax, buffer_ports, addrlen_ds, neighbors);
                                    free(mes);
                                    printf("***********************************************************************************\n\n"); 
                                }                           
                                
                                memset(&neighbors, 0, sizeof(neighbors));
                                break;
                        }
                    }
                }
            }
        }
    }
}