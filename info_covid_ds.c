#include "info_covid.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define REQUEST_LEN 5
#define RESPONSE_LEN 31 //OK\0

/******************************************************************************
funzione che stampa il messaggio iniziale
******************************************************************************/
void start_message_ds(){
    
    printf("---------------------------------------------------------------------------------------\n");
    printf("--------------------------------- DISCOVERY SERVER STARTED ----------------------------\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Digita un comando tra quelli elencati per iniziare: \n\n");
    printf("1) help                ===> mostra maggiori dettagli su questi comandi \n");
    printf("2) showpeers           ===> mostra un elenco dei peer connessi \n");
    printf("3) showneighbor <peer> ===> mostra i neighboor di un peer \n");
    printf("4) esc                 ===> chiude il DS e causa la terminazione di tutti i peer \n");
    printf("---------------------------------------------------------------------------------------\n");

}

/******************************************************************************
funzione di appoggio che per ogni peer passato restituisce i suoi vicini
parametri:
@ neighbors è l'array in cui depostare i vicini
@ port è il peer del quale cercare i vicini
******************************************************************************/
void get_neighbor(unsigned short *neighbors, unsigned short port){

    struct info_peer *scan;
    int j = 1;
    scan = head;
    while(j<=number_peer){
        if(scan->port == port){  
            if(number_peer == 1){    
                neighbors[0] = 0;
                neighbors[1] = 0;  
            }
            else{
                if(j == 1){
                    neighbors[0] = last;
                    neighbors[1] = scan->next->port;
                }
                else if(j == number_peer){
                    neighbors[0] = scan->previous->port;
                    neighbors[1] = first;
                }
                else{
                    neighbors[0] = scan->previous->port;
                    neighbors[1] = scan->next->port;
                }
            }
        }
        j++;
        scan = scan->next;
    }

}

/******************************************************************************
funzione che stampa una lista di peer attualmente connessi alla rete
******************************************************************************/
void showpeers(){
    
    int j;
    struct info_peer* scan;
    
    j=1;
    scan = head;   
    printf("***********************************************************************************\n\n");
    printf("Lista peer di %hu peer: \n", number_peer);
    while(j<=number_peer){
        
        printf("%hu, ", scan->port);
        scan = scan->next;
        j++;
    }
    printf("\n");
    printf("***********************************************************************************\n\n");
}  

/******************************************************************************
funzione di appoggio alla showneighbor che stampa i vicini a schermo
parametri:
@ scan è il puntatore alla struct che scorre la lista circolare
@ iter è una variabile contatore 
******************************************************************************/
void print_neighbor(struct info_peer* scan, int iter){

    if(number_peer == 1){
        printf("%hu: nessuno \n", scan->port);
    }
    else{
        if(iter == 1){
            printf("%hu: [%hu] [%hu] \n", scan->port, last, scan->next->port);
        }
        else if(iter == number_peer){
            printf("%hu: [%hu] [%hu] \n", scan->port, scan->previous->port, first);
        }
        else{
            printf("%hu: [%hu] [%hu] \n", scan->port, scan->previous->port, scan->next->port);
        }
    }
}

/******************************************************************************
funzione che mostra i vicini dei peer
parametri:
@ parsed_stream array con comandi inseiriti
@ parameters che inidica se è stato inserito o meno una porta
******************************************************************************/
void showneighbor(char parsed_stream[4][23], bool parameters){

    uint16_t port;
    int j;
    struct info_peer* scan;
    printf("***********************************************************************************\n\n");
    j=1;
    scan = head; 
    if(parameters){

        port = (uint16_t)atoi(parsed_stream[1]);
        printf("Neighbor del peer con porta numero %hu: \n", port);      
        scan = head; 
        while(j<=number_peer){           
            if(scan->port == port){              
                print_neighbor(scan, j);
                break;
            }
            if(j == number_peer){
                printf("Per con numero di porta %hu non presente nella rete \n", port);	
            }
            scan = scan->next;
            j++;
        }
    }
    else{
        printf("Lista dei neighbor per ogni peer: \n");   
        while(j<=number_peer){
            
            print_neighbor(scan, j);
            
            scan = scan->next;
            j++;
        }
    }
    printf("\n");
    printf("***********************************************************************************\n\n");
}

/******************************************************************************
funzione che aggiorna i vicini dei peer
parametri:
@ update_type tipologia di messaggio di update
@ num_peers è il numero dei peer presenti
@ neighbors array contenente porte dei vicini
@ peers è un array contennente i indirizzo_socket_peer dei peer connessi
        attualmente alla rete
@ set è un puntatore alla struct del messaggio da inviare
@ indirizzo_socket_peer è la struct sockaddr_in relativa al peer
@ address_port_to_add indirizzo del peer da aggiungere
@ addrlen_peer  è sizeof(indirizzo_socket_peer)
@ socket_ds è il socket utilizzato per dialogare con i peer usando UDP
******************************************************************************/
void update_neighbors(char update_type, int *num_peers, unsigned short *neighbors, struct sockaddr_in *peers, struct message* set, struct sockaddr_in indirizzo_socket_peer, char *address_port_to_add, socklen_t addrlen_peer, int socket_ds){

    unsigned short neighbors_tosend, first_neighbors_toedit, second_neighbors_toedit, neighbors_toedit;
    int ret,k,j;  
    char ack_peer[BUFFER_SIZE];

    if(((*num_peers)>1) || (((*num_peers) == 1) && (update_type == 'D'))){
        
        printf("\n");
        printf("********** Aggiornamento vicini degli altri Peer *********\n");
        first_neighbors_toedit = neighbors[0];   
        second_neighbors_toedit = neighbors[1];
        for(k = 0; k< N_NEIGHBOR; k++){

            if(k == 0){   
                get_neighbor(neighbors, first_neighbors_toedit);
                printf("Aggiornamento dei vicini del primo Neighbor: %hu\n", first_neighbors_toedit);
                printf("--------------------------------------------\n");
                neighbors_toedit = first_neighbors_toedit;
            }
            else{
                get_neighbor(neighbors, second_neighbors_toedit);
                printf("\n");
                printf("Aggiornamento dei vicini del secondo Neighbor: %hu\n",second_neighbors_toedit);
                printf("--------------------------------------------\n");
                neighbors_toedit = second_neighbors_toedit;
            }
            for(j=0; j<N_NEIGHBOR; j++){    
                // si assegna il neighbor da inviare         
                if(j == 0){
                    neighbors_tosend = neighbors[0];
                }
                else{
                    neighbors_tosend = neighbors[1];
                }
                // creazione del messaggio da inviare
                set = create_message('U', NULL, (uint16_t)neighbors_tosend, (uint8_t*)address_port_to_add, 0, NULL);
                do{
                    ret = sendto(socket_ds, &(set->type), sizeof(uint8_t), 0,(struct sockaddr *)&peers[neighbors_toedit], sizeof(peers[neighbors_toedit]));
                    if(ret < 0){
                        perror("[   ATTESA INVIO VICINO   ] tentativo di reinvio in corso...\n");	
                    }
                }while(ret < 0);
                do{
                    ret = sendto(socket_ds, &(set->port), sizeof(uint16_t), 0,(struct sockaddr *)&peers[neighbors_toedit], sizeof(peers[neighbors_toedit]));
                    if(ret < 0){
                        perror("[   ATTESA INVIO VICINO   ] tentativo di reinvio in corso...\n");	
                    }
                }while(ret < 0);
                if(j == 0){
                    printf("[  INVIO DEL PRIMO VICINO  ]: %hu\n", ntohs(set->port));
                }
                else{
                    printf("[ INVIO DEL SECONDO VICINO ]: %hu\n", ntohs(set->port));
                }         
            }
            // ricezione dell-indirizzo IP del peer da registrare
            do{
                ret = recvfrom(socket_ds, ack_peer, 36, 0, (struct sockaddr *)&indirizzo_socket_peer, &addrlen_peer);
            }while(ret < 0);
            printf("[ACK DI AVVENUTA RICEZIONE ]: %s\n", ack_peer);
            if(((*num_peers) <= 2)){
                if(((update_type != 'D') && ((*num_peers) == 2)) || ((update_type == 'D') && ((*num_peers) == 1))){
                     break;
                }            
            }
        }

    free(set);
    }  
}

/******************************************************************************
funzione che gestisce le richieste provenienti dai peer
parametri:
@ socket_ds è il socket utilizzato per dialogare con i peer usando UDP
@ buffer_ports è un buffer per le risposte
@ indirizzo_socket_peer è la struct sockaddr_in relativa al peer
@ addrlen_peer  è sizeof(indirizzo_socket_peer)
@ peers è un array contennente i indirizzo_socket_peer dei peer connessi
        attualmente alla rete
@ max_port è il peer con massima porta presente
@ num_peers è il numero dei peer presenti
******************************************************************************/
void handle_request(int socket_ds, char* buffer_ports, struct sockaddr_in indirizzo_socket_peer, socklen_t addrlen_peer, struct sockaddr_in *peers, int *max_port, int *num_peers){

    // variabili di utility
    int ret,j;
    // variabile per il controllo di inserimento
    bool inserted, removed;
    char *response = "PEER REGISTRATO CORRETTAMENTE\0";
    char type;
    // array per i neighbors
    unsigned short neighbors[N_NEIGHBOR];
    // variabile di appoggio per inviare i neighbors
    unsigned short neighbors_tosend;
    // variabile che contiene l'indirizzo IP del peer da registrare
    char received[BUFFER_SIZE];
    // variabile che contiene la porta del peer da registrare
    unsigned short port_to_add, port_to_remove;
    char address_port_to_add[INET_ADDRSTRLEN] = "127.0.0.1";
    // message per inviare messaggi ai peer
    int len = sizeof(struct message);
    struct message *set = (struct message*)malloc(len+1);

    printf("***********************************************************************************\n\n");
    // ricezione dell-indirizzo IP del peer da registrare
    do{
        ret = recvfrom(socket_ds, &type, INET_ADDRSTRLEN, 0, (struct sockaddr *)&indirizzo_socket_peer, &addrlen_peer);
    }while(ret < 0);
    if(type == 'B'){
        printf("[ RICEZIONE TIPO RICHIESTA ]: Registrazione nuovo peer\n");
    }
    if(type == 'D'){
        printf("[ RICEZIONE TIPO RICHIESTA ]: Disconnessione peer\n");
    }
    
    // ricezione dell-indirizzo IP del peer da registrare
    do{
        ret = recvfrom(socket_ds, received, INET_ADDRSTRLEN, 0, (struct sockaddr *)&indirizzo_socket_peer, &addrlen_peer);
    }while(ret < 0);
    printf("[ RICEZIONE INDIRIZZO PEER ]: %s\n", received);
    
    // controllo se la richiesta è di inserimento o disconnessione
    if(type == 'B'){    

        // ricezione della porta del peer da registrare
        do{
            ret = recvfrom(socket_ds, &port_to_add, sizeof(uint16_t), 0, (struct sockaddr *)&indirizzo_socket_peer, &addrlen_peer);
        }while(ret < 0);
        port_to_add= ntohs(port_to_add);
        printf("[ RICEZIONE PORTA DEL PEER ]: %hu\n", port_to_add);
        printf("\n");
        printf("-------------------------------------------------------------------------\n");
        printf("------------------------- REGISTRAZIONE INIZIATA ------------------------\n");
        printf("-------------------------------------------------------------------------\n");      
        printf("***************** Registrazione Peer *********************\n");

        // inserimento del peer nella rete e prelievo dei neighbor
        inserted = insert_peer(port_to_add, received);
        if(!inserted){
            response = "PEER GIA' PRESENTE NELLA RETE\0";
        }
        get_neighbor(neighbors, port_to_add);
        
        // invio di ack per indicare avvenuta registrazione
        do{
            ret = sendto(socket_ds, response, RESPONSE_LEN, 0,(struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
            if(ret < 0){
                perror("[    ATTESA INVIO ACK     ] tentativo di reinvio in corso...\n");	
            }
        }while(ret < 0);
        
        // se il peer è stato registrato, si inviano i suoi neighbor
        if(inserted){
            
            if(*max_port < port_to_add){
                *max_port = port_to_add;
            }
            peers[port_to_add] = indirizzo_socket_peer;
            (*num_peers)++;

            for(j=0; j<N_NEIGHBOR; j++){    
                // si assegna il neighbor da inviare         
                if(j == 0){
                    neighbors_tosend = neighbors[0];
                }
                else{
                    neighbors_tosend = neighbors[1];
                }
                // creazione del messaggio da inviare
                set = create_message('U', NULL, (uint16_t)neighbors_tosend, (uint8_t*)address_port_to_add, 0, NULL);
                do{
                    ret = sendto(socket_ds, &(set->port), sizeof(uint16_t), 0,(struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
                    if(ret < 0){
                        perror("[   ATTESA INVIO VICINO   ] tentativo di reinvio in corso...\n");	
                    }
                }while(ret < 0);
                if(j == 0){
                    printf("[  INVIO DEL PRIMO VICINO  ]: %hu\n", ntohs(set->port));
                }
                else{
                    printf("[ INVIO DEL SECONDO VICINO ]: %hu\n", ntohs(set->port));
                }          
            }
            
            update_neighbors('B', num_peers, neighbors, peers, set, indirizzo_socket_peer, address_port_to_add, addrlen_peer, socket_ds);
        }                        
        printf("-------------------------------------------------------------------------\n");
        printf("------------------------ REGISTRAZIONE TERMINATA ------------------------\n");
        printf("-------------------------------------------------------------------------\n");   
        printf("\n");
        
    }
    if(type == 'D'){  

        response = "PEER DISCONNESSO CORRETTAMENTE\0";

        // ricezione della porta del peer da registrare
        do{
            ret = recvfrom(socket_ds, &port_to_remove, sizeof(uint16_t), 0, (struct sockaddr *)&indirizzo_socket_peer, &addrlen_peer);
        }while(ret < 0);
        port_to_remove= ntohs(port_to_remove);
        printf("[ RICEZIONE PORTA DEL PEER ]: %hu\n", port_to_remove);
        printf("\n");
        printf("-------------------------------------------------------------------------\n");
        printf("------------------------ DISCONNESSIONE INIZIATA ------------------------\n");
        printf("-------------------------------------------------------------------------\n");      
        printf("***************** Disconnessione Peer ********************\n");

        get_neighbor(neighbors, port_to_remove);
        // rimozione del peer nella rete 
        removed = remove_peer(port_to_remove);
        if(!removed){
            response = "PEER NON PRESENTE NELLA RETE \0";
        }    
        // invio di ack per indicare avvenuta disconnessione
        do{
            ret = sendto(socket_ds, response, RESPONSE_LEN, 0,(struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
            if(ret < 0){
                perror("[    ATTESA INVIO ACK     ] tentativo di reinvio in corso...\n");	
            }
        }while(ret < 0);
        if(!removed){
            printf("[    PEER NON PRESENTE     ]: %hu\n", port_to_remove);
        }
        if(removed){     
            printf("[PEER RIMOSSO CON SUCCESSO ]: %hu\n", port_to_remove);

            if(port_to_remove == *max_port){
                (*max_port)--;
            }
            (*num_peers)--;
            
            update_neighbors('D', num_peers, neighbors, peers, set, indirizzo_socket_peer, address_port_to_add, addrlen_peer, socket_ds);      
        }
        printf("-------------------------------------------------------------------------\n");
        printf("----------------------- DISCONNESSIONE TERMINATA ------------------------\n");
        printf("-------------------------------------------------------------------------\n");  
        printf("\n");
    }
    printf("***********************************************************************************\n\n"); 
}

/******************************************************************************
funzione che provoca la disconnessione di tutti i peer
parametri:
@ socket_ds è il socket utilizzato per dialogare con i peer usando UDP
@ addrlen_peer  è sizeof(indirizzo_socket_peer)
@ peers è un array contennente i indirizzo_socket_peer dei peer connessi
        attualmente alla rete
@ max_port è il peer con massima porta presente
@ num_peers è il numero dei peer presenti
******************************************************************************/
void esc(int socket_ds, socklen_t addrlen_peer, struct sockaddr_in *peers, int *max_port, int *num_peers){

    int ret;
    struct sockaddr_in indirizzo_socket_peer;
    // variabile per il controllo di inserimento
    bool removed;
    char *response = "PEER RIMOSSO CORRETTAMENTE\0";
    // array per i neighbors
    unsigned short neighbors[N_NEIGHBOR];
    // variabile che contiene la porta del peer da registrare
    unsigned short port_to_remove;
    char address_port_to_add[INET_ADDRSTRLEN] = "127.0.0.1";
    // message per inviare messaggi ai peer
    int len = sizeof(struct message);
    struct message *set = (struct message*)malloc(len+1);
    struct info_peer *scan;
    char type = 'D';

    printf("***********************************************************************************\n\n");
    printf("-------------------------------------------------------------------------\n");
    printf("------------------------ DISCONNESSIONE INIZIATA ------------------------\n");
    printf("-------------------------------------------------------------------------\n");      
    scan = head;
    while(scan != NULL){

        port_to_remove = scan->port;
        indirizzo_socket_peer = peers[port_to_remove];
        printf("\n");
        printf("***************** Disconnessione Peer %hu********************\n", scan->port);

        get_neighbor(neighbors, port_to_remove);
        // rimozione del peer nella rete 
        removed = remove_peer(port_to_remove);
        if(!removed){
            printf("[    PEER NON PRESENTE     ]: %hu\n", port_to_remove);
        }
        if(removed){     
            printf("[PEER RIMOSSO CON SUCCESSO ]: %hu\n", port_to_remove);


            do{
                ret = sendto(socket_ds, &type, sizeof(uint8_t), 0,(struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
                if(ret < 0){
                    perror("[   ATTESA INVIO VICINO   ] tentativo di reinvio in corso...\n");	
                }
            }while(ret < 0);
            do{
                ret = sendto(socket_ds, response, RESPONSE_LEN, 0,(struct sockaddr *)&indirizzo_socket_peer, sizeof(indirizzo_socket_peer));
                if(ret < 0){
                    perror("[    ATTESA INVIO ACK     ] tentativo di reinvio in corso...\n");	
                }
            }while(ret < 0);

            if(port_to_remove == *max_port){
                (*max_port)--;
            }
            (*num_peers)--;
            
            update_neighbors('D', num_peers, neighbors, peers, set, indirizzo_socket_peer, address_port_to_add, addrlen_peer, socket_ds);      
        }

        scan = scan->next;
    }
    printf("-------------------------------------------------------------------------\n");
    printf("----------------------- DISCONNESSIONE TERMINATA ------------------------\n");
    printf("-------------------------------------------------------------------------\n");  
    printf("***********************************************************************************\n\n");

}

int main(int argc, char* argv[]){

    // variabili per connessioni
    int ret, socket_ds;
    socklen_t addrlen_peer;
    struct sockaddr_in indirizzo_socket_peer, indirizzo_socket_ds;
    struct sockaddr_in peers[MAX_PEERS];
    int max_port = 0;
    int num_peers = 0;
    // variabili di appoggio per l'utilizzo delle select
    fd_set tocheck;
    fd_set ready;
    int fdmax;
    char buffer_ports[BUFFER_SIZE];	
    char buffer[BUFFER_STREAM_INPUT];	
    char parsed_stream[4][23];
    // counter vari e var di controllo
    int i;
    int n_words = 0;
	int result;
    bool parameters = false;
    bool stream_cmd = false;
    // variabili di porte e address dei soggetti
    uint16_t port_ds;

    if(check_port(argv[1])){
        port_ds = (unsigned short)atoi(argv[1]);   
    }
    else{
        perror("Numero di Porta inserito non valido");
    }
    

	socket_ds = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);	
	memset(&indirizzo_socket_ds, 0, sizeof(indirizzo_socket_ds));
	indirizzo_socket_ds.sin_family = AF_INET;
	indirizzo_socket_ds.sin_port = htons(port_ds);
	indirizzo_socket_ds.sin_addr.s_addr = INADDR_ANY;
	

	ret = bind(socket_ds, (struct sockaddr *)&indirizzo_socket_ds, sizeof(indirizzo_socket_ds));
    if(ret < 0){
        perror("Bind non riuscita\n");
        exit(0);
    }

    addrlen_peer = sizeof(indirizzo_socket_peer);

    // azzero i set, per standard, dovrrebbero essere già a zero però
    FD_ZERO(&tocheck);
    FD_ZERO(&ready);
    // aggiungo il socket di attesa al set di descrittori da controllare
    FD_SET(socket_ds, &tocheck);
    FD_SET(STDIN_FILENO, &tocheck);

    // inizializzo il primo fdmax
    fdmax = socket_ds;
	
	while(1){

        start_message_ds();
        ready = tocheck;      
        if(select(fdmax + 1, &ready, NULL, NULL, NULL)){
            // scorro i descrittori che sono pronti 
            for(i = 0; i<=fdmax; i++){
                // se ne incontro uno tra questi
                if(FD_ISSET(i, &ready)){
                    if(i == socket_ds){
                        handle_request(socket_ds, buffer_ports, indirizzo_socket_peer, addrlen_peer, peers, &max_port, &num_peers);
                    }
                    else{
                        
                        fgets(buffer, BUFFER_SIZE, stdin);

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
                            case D_HELP:

                                printf("***********************************************************************************\n");
                                printf("showpeers           ===> mostra un elenco dei peer connessi attualmente alla rete\n");
                                printf("showneighbor <peer> ===> mostra i neighboor dei peer connessi alla rete \n"
                                       "                         se viene inserita solo la parola showneighbor, vengono\n"
                                       "                         mostrati i neighbor di ogni peer, se invece si inserisce \n"
                                       "                         anche il peer desiderato accanto, vengono mostrati solo \n"
                                       "                         i neighbor relativi al peer inserito \n");
                                printf("esc                 ===> chiude il DS e causa la terminazione di tutti i peer \n"
                                       "                         attualmente collegati alla rete\n");
                                printf("***********************************************************************************\n\n");

                                break;
                            case D_SHOWPEERS:

                                showpeers();  
                                break;
                            case D_SHOWNEIGHBOR:
                                 
                                if(atoi(parsed_stream[1]) != 0){
                                    parameters = true;
                                }
                                else{
                                    parameters = false;
                                }
                                showneighbor(parsed_stream, parameters); 
                                break;
                            case D_ESC:
                                
                                esc(socket_ds, addrlen_peer, peers, &max_port, &num_peers);
                                break;
                        }

                    }
                }
            }
        }

        
    }
}