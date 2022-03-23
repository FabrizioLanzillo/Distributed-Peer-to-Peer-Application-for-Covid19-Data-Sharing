#ifndef COSTANTI_H
#define COSTANTI_H

#define BUFFER_SIZE 1024
#define BUFFER_STREAM_INPUT 5120
#define BOOT_LEN 5
#define POLLING_TIME 5  

// direttive per il riconoscimento dei comandi di un peer
#define P_START 0 
#define P_ADD 1 
#define P_GET 2 
#define P_STOP 3  

// direttive per il riconoscimento dei comandi di un peer
#define P_START 0 
#define D_HELP 4 
#define D_SHOWPEERS 5
#define D_SHOWNEIGHBOR 6
#define D_ESC 7

// direttive globali
#define N_NEIGHBOR 2
#define MAX_PEERS 65535

// direttive per scritture su file
#define N_LINES 4           // numero di linee su ogni file
#define LINE_SIZE 13        // caratteri costanti e valore numerico del dato
#define LINE_SIZE_VAR 30        // caratteri costanti e valore numerico del dato
#define VALUE_LEN 7         // lunghezza effettiva del valore numerico per ogni riga

// direttive per i percorsi dei file
#define NAME_LEN 16         // lunghezza del nome del file
#define NAME_AGGR_LEN 27    // lunghezza del nome del file aggregato
#define PATH_AGGR_T 31      // path che porta nella cartella tamponi contenuta in aggr 
#define PATH_AGGR_C 28      // path che porta nella cartella casi contenuta in aggr
#define PATH_REGISTER 25    // path che porta nella cartella register contenuta in quella del nome del peer  
#define PATH_PEER 13        // path per accedere alla cartella del nome del peer contenuta in files
#define PATH_NO_FILE 31     // path massimo senza la dimensione dei file
#define PATH_COMPLETE 58    // path che contiene l-inidizzo completo del file a cui si vuole accedere


#define LEN_INFO_TCP 10








#endif	// COSTANTI_H