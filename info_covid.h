#ifndef INFO_COVID_H
#define INFO_COVID_H

#include "costanti.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#define __USE_XOPEN
#include <time.h>
#include <netinet/in.h>

/*************************************************************************************************************/
/******************************************* STRUTTURE DATI **************************************************/
/*************************************************************************************************************/

struct info_peer{

    char *ip_address;               // indirizzo ip del peer
    uint16_t port;                // numero di porta del peer in esamina
    struct info_peer *previous;         // numero di porta del peer successivo
    struct info_peer *next;             // nuemro di porta del peer precedente
};


// B=boot, U=update, D=disconnesione, A=aggr, F=flooding, P=partial, E =end of tcp trasmission, V=variazione, S=saving
struct message{

    uint8_t type;                      
    uint8_t *info;                     // campo opzionale per altre informazioni
    uint16_t port; 
    uint8_t *address; 
    uint32_t data;
    uint8_t* payload;
};

struct variation_date{
    
    double diff_to_begin;
    char date[NAME_LEN];
    int data_read;
};


/*************************************************************************************************************/
/************************************** VARIABILI E PUNTATORI ************************************************/
/*************************************************************************************************************/

// VARIABILI E STRUTTURE GLOBALI
// puntatore al peer con porta di valore minore
struct info_peer *head;
// variabile che contiene il peer con porta minore 
// per implementare politica ad anello
extern uint16_t first;
// variabile che contiene il peer con porta maggiore 
// per implementare politica ad anello
extern uint16_t last;
// variabile che conta il numero totale di peers sulla rete
extern unsigned short number_peer;

/*************************************************************************************************************/
/**************************************** FUNZIONI DI UTILITY ************************************************/
/*************************************************************************************************************/

//*******************************************************************
// funzione che prende le parole inserite nello stream di ingresso
// e ritorna errore se non viene scritto niente oppure se non
// è possibile memorizzare queste parole in memoria
// parametri:
// @ stream è un puntatore al buffer su cui eseguire il parse
// @ len è la lunghezza del buffer
// @ n_words è un puntatore dove viene memorizzata la lunghezza
//   dell'array di stringhe restituito
//
// @ return: restituisce un array di stringhe contenente le 
//           parole del comando immesso nello stream
//*******************************************************************
bool parse_stream(char *stream, int len, int *n_words, char parsed_stream[4][23]);

//*******************************************************************
// funzione che copia in un buffer di dimensione len, 
// che verrà restituito al chiamante, il buffer passato alla 
// funzione a partire dall'elemento indice 
// parametri:
// @ buff, il buffer dal quale copiare i caratteri
// @ len è la lunghezza del buffer in cui copio i carateri
// @ index è l'indice di buff dal quale inizio il trasferimento
//
// @ return: restituisce buf, array di char, contenente len
//           caratteri copiati da buff a partire da index
//*******************************************************************
char* buff_copy(char *buff, int len, int index);

//*******************************************************************
// funzione che controlla che il tipo sia tra quelli corretti
// parametri:
// @ type, il tipo su cui si fa il controllo

//*******************************************************************
bool check_type(char *type);

//*******************************************************************
// funzione che controlla che la porta sia in un intervallo corretto
// parametri:
// @ port, contiene il numero della porta

//*******************************************************************
bool check_port(char *port);

//*******************************************************************
// funzione che controlla che il numero non sia negativo
// parametri:
// @ number, il numero su cui si esegue il controllo

//*******************************************************************
bool check_int(char *number);

//*******************************************************************
// funzione che controlla che aggr sia tra quelli corretti
// parametri:
// @ aggr, l'aggr su cui si fa il controllo

//*******************************************************************
bool check_aggr(char *aggr);

//*******************************************************************
// funzione che controlla che i giorni, i mesi e l'anno siano tra
// quelli consentiti
// @ date, data (formato dd:mm:yyyy) su cui si esegue il controllo

//*******************************************************************
bool check_date(char *date);

//*******************************************************************
// funzione che controlla che il periodo fornito sia in un formato
// adeguato
// parametri:
// @ date, che indica il periodo scritto nello stream

//*******************************************************************
bool check_period(char *date);

//*******************************************************************
// funzione che valuta le parole scritte nello strem e valuta
// se corrispondono alla giusto comando scelto, e in caso 
// affermativo ritornano il codice del comando 
// parametri:
// @ parsed_stream, indica un array di stringhe, dove ogni stringa
//                  corrisponde ad una parola per quel comando 
// @ n_words,       indica il numero di parole in quel comando
//
// @ return: restituisce un numero codificato con una direttiva 
//           per leggibilita' oppure -1 in caso di comando sbagliato
//*******************************************************************
int validate_parsed_stream(char parsed_stream[4][23], int n_words);

//*******************************************************************
// funzione che inserisce un peer nella struttura dati ad anello
// implementato attraverso una lista
// parametri:
// @ port, numero di porta del peer da aggiungere
// @ address, indirizzo del peer da aggiungere

//*******************************************************************
bool insert_peer(uint16_t port, char *address);

//*******************************************************************
// funzione che rimuove un peer dalla rete, passando il suo numero
// di porta
// parametri:
// @ port, numero di porta del peer da rimuovere

//*******************************************************************
bool remove_peer(uint16_t port);

/* 
*******************************************************************
funzione che crea un messaggio inserendo tutti i valori che si
desiderano inviare in una struct, e serializza quei valori che sono 
più grandi di un byte
parametri:
@ type è la tipologia di messaggio da inviare
@ info è il tipo di dati che vogliamo inviare
@ port è la porta che si desira inviare
@ address è l'indirizzo IP
@ data è un valore aggiuntivo che vogliamo 
@ payload è una stringa di dati che vogliamo inviare

@ return: restituisce il puntatore alla struttura
*******************************************************************
*/
struct message *create_message(uint8_t type, uint8_t *info, uint16_t port, uint8_t *address, uint32_t data, uint8_t *payload);

/* 
*******************************************************************
funzione che estrapola il valore delle entry dai register
parametri:
@ line_toread è la linea da leggere nel register
@ char_to_start_read è il carattere da cui inziare a leggere
@ elements è l'array in cui immagazzinare i valori 

@ return: restituisce il valore trovato convertito ad intero
*******************************************************************
*/
int get_number_entry(int line_toread, int char_to_start_read, char elements[][LINE_SIZE]);

#endif	// INFO_COVID_H