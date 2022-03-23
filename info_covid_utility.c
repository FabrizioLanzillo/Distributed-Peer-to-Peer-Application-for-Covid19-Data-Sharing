#include "info_covid.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define __USE_XOPEN
#include <time.h>

unsigned short number_peer = 0;
uint16_t last = 0;
uint16_t first = 0;

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
bool parse_stream(char *stream, int len, int *n_words, char parsed_stream[4][23]){

    int number_words = 1;
	char space = ' ';
	char* word = NULL;
	int i;
	
	// errore, stream vuoto
	if (!stream){	    
	    return false;
	}
	
	// Conta le parole
	for (i = 0; i < len; ++i) {
		if (stream[i] == space){
		    number_words++;
		}
	}


	// Parse del stream, attraverso la strsep, e poi ricopiatura
	// nella locazione i-esima dell-array di stringhe
	for (i = 0; i < number_words; ++i) {
        word = NULL;
		word = strsep(&stream, &space);
		strcpy(parsed_stream[i], word);
	}

    // si aggiorna il parametro contenente il numero di parole e si 
    // restituisce l-array
    *n_words = number_words;
    return true;
}

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
char* buff_copy(char *buff, int len, int index){
   
    int i;
    char* buf = NULL;
    
    buf = (char*)malloc(len * sizeof(char));
	if (!buf) {
		fprintf(stderr, "Impossibile allocare parsed_comando in memoria\n");
		fflush(stderr);
	}

    for(i=0; i<len; i++){
        
        buf[i] = buff[i + index];
    }

    return buf;
}

//*******************************************************************
// funzione che controlla che il tipo sia tra quelli corretti
// parametri:
// @ type, il tipo su cui si fa il controllo

//*******************************************************************
bool check_type(char *type){
    
    if(!strcmp(type, "N") || !strcmp(type, "T")){
        return true;
    }
    else{
        return false;
    }
}

//*******************************************************************
// funzione che controlla che la porta sia in un intervallo corretto
// parametri:
// @ port, contiene il numero della porta

//*******************************************************************
bool check_port(char *port){
    
    int value = atoi(port);
            
    if(value <= 0 || value > 65535){
        return false;
    }
    
    return true;
}

//*******************************************************************
// funzione che controlla che il numero non sia negativo
// parametri:
// @ number, il numero su cui si esegue il controllo

//*******************************************************************
bool check_int(char *number){

    int value = atoi(number);
    
    if(value <= 0){
        
        return false;
    }
    
    return true;
}

//*******************************************************************
// funzione che controlla che aggr sia tra quelli corretti
// parametri:
// @ aggr, l'aggr su cui si fa il controllo

//*******************************************************************
bool check_aggr(char *aggr){
    
    if(!strcmp(aggr, "totale") || !strcmp(aggr, "variazione")){
        return true;
    }
    else{
        return false;
    }
}

//*******************************************************************
// funzione che controlla che i giorni, i mesi e l'anno siano tra
// quelli consentiti
// @ date, data (formato dd:mm:yyyy) su cui si esegue il controllo

//*******************************************************************
bool check_date(char *date){

    struct tm tm;
    int month, day, year;
    
    strptime(date, "%d:%m:%Y", &tm);
    
    day = tm.tm_mday;
    month = tm.tm_mon+1;
    year = tm.tm_year+1900;
    
    if((day>=0 && day<32)&&(month>=0 && month<13)&&(year>=0)){
        
        return true;
    }
    else{
        return false;
    }
}

//*******************************************************************
// funzione che controlla che il periodo fornito sia in un formato
// adeguato
// parametri:
// @ date, che indica il periodo scritto nello stream

//*******************************************************************
bool check_period(char *date){
    
    int len = strlen(date);
    char *buf = NULL;
    char *buf2 = NULL;
    bool res, res2;
    
    // formato: dd:mm:yyyy-* ||  *-dd:mm:yyyy
    if(len == 12){
        if(date[0] == '*'){
            if(date[1] != '-'){
                return false;
            }
            buf = buff_copy(date, 10, 2);
        }
        else if(date[11] == '*'){
            if(date[10] != '-'){
                return false;
            }
            buf = buff_copy(date, 10, 0);
        }
        else{
            return false;
        }
        res = check_date(buf);
        if(!res){
            return false;
        }
        return true;
    }
    // formato dd:mm:yyyy-dd:mm:yyy
    else if(len == 21){
        if(date[10] != '-'){
            return false;
        }
        buf = buff_copy(date, 10, 0);
        buf2 = buff_copy(date, 10, 11);
        res = check_date(buf);
        res2 = check_date(buf2);
        if(!res || !res2){
            return false;
        }
        return true;
    }
    else{
        return false;
    }
}

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
int validate_parsed_stream(char parsed_stream[4][23], int n_words){

    if(n_words == 3){
        if(!strcmp(parsed_stream[0], "start")){
            
            if(!check_port(parsed_stream[2])){
                return -1;
            }
            
            return P_START;
        }
        else if(!strcmp(parsed_stream[0], "add")){
            if(!check_type(parsed_stream[1]) || !check_int(parsed_stream[2])){
                return -1;
            }
            return P_ADD;
        }
        else if(!strcmp(parsed_stream[0], "get")){

            if(!check_aggr(parsed_stream[1]) || !check_type(parsed_stream[2])){
                return -1;
            }
            return P_GET;
        }
        else{
            return -1;
        }
    }   
    else if (n_words == 4){
        if(!strcmp(parsed_stream[0], "get")){
            
            if(!check_aggr(parsed_stream[1]) || !check_type(parsed_stream[2]) || !check_period(parsed_stream[3])){
                return -1;
            }
            return P_GET;
        }
        return -1;
    }
    else if(n_words == 1){  
        if(!strcmp(parsed_stream[0], "stop")){
         
            return P_STOP;
        }
        
        else if(!strcmp(parsed_stream[0], "help")){
         
            return D_HELP;
        }
        
        else if(!strcmp(parsed_stream[0], "showpeers")){
         
            return D_SHOWPEERS;
        }
        
        else if(!strcmp(parsed_stream[0], "showneighbor")){
         
            return D_SHOWNEIGHBOR;
        }
        
        else if(!strcmp(parsed_stream[0], "esc")){
         
            return D_ESC;
        }

        else{
            return -1;
        }
    }
    else if(n_words == 2){
        if(!strcmp(parsed_stream[0], "showneighbor")){
            
            if(!check_port(parsed_stream[1])){
                return -1;
            }
            return D_SHOWNEIGHBOR;
        }
        return -1;
    }
    else{
        return -1;
    }
}

//*******************************************************************
// funzione che rimuove un peer dalla rete, passando il suo numero
// di porta
// parametri:
// @ port, numero di porta del peer da rimuovere

//*******************************************************************
bool remove_peer(uint16_t port){
    
    struct info_peer *scan, *scan_previous, *to_del;
	bool found;
	int i;
    
    // si allinea, scan alla testa della lista, e scan_previous che sarà
    // il puntatore al peer precedente rispetto a scan lo poniamo inizialmente 
    // a 0, mentre found e is_last sono variabili di controllo
	scan = head;
	scan_previous = 0;
	found = false;

    // si scorre la lista e si esce quando trovo la posizione del peer giusta
    // oppure se è finita, mentre non entro se la lista è vuota, o è finita
    i = 1;
	while (scan != NULL && !found){
        // se il numero di porta è stato superato è inutile continuare il loop
		if(scan->port <= port){
            if (port != scan->port) {
                scan_previous = scan;
                scan = scan->next;
                // se si trova in ultima posizione 
                // si setta a true questa var di controllo
            } 
            else{
                found = true;
                to_del = scan;
            }
            i++;
        }
        else{
            return false;
        }
        
	}   
	// controllo nel caso si volesse elimnare un peer, quando la lista è vuota
	if(number_peer != 0){
        // caso in cui il peer non è presente, ma la lista non è vuota
        // avviene quando il peer è più grande del max port della lista
        if(scan != NULL){
            // caso in cui l'elemento da eliminare non è l'ultimo della lista
            if(scan->next != NULL){
                scan = scan->next;
                // se è il primo della lista
                if(scan_previous == 0){
                    head = scan;
                    first = scan->port;
                }
                else{
                    scan_previous->next = scan;
                }
            }
            // se è l'ultimo della lista
            else{
                // caso in cui non è l'unico elemento della lista
                if(scan_previous != 0){
                    scan_previous->next = NULL;
                    last = scan_previous->port;
                }
                else{
                    head = 0;
                }
            }
            // se è stato allocato to_del si libera
            if(to_del != NULL){   
                free(to_del);
            }
            // si aggiorna il numero totale di peer sulla rete
            number_peer--;
            // si riallineano i puntatori previous, per l'inserimento fuori ordine
            scan_previous = 0;
            for(scan = head; scan != NULL; scan = scan->next){
                if(scan_previous != scan->previous){
                    scan->previous = scan_previous;
                }
                scan_previous = scan;
            }
        }
        else{
            return false;
        }
	}
	else{
	    return false;
	}
    return true;
}


//*******************************************************************
// funzione che inserisce un peer nella struttura dati ad anello
// implementato attraverso una lista
// parametri:
// @ port, numero di porta del peer da aggiungere
// @ address, indirizzo del peer da aggiungere

//*******************************************************************
bool insert_peer(uint16_t port, char *address) {
    
    struct info_peer *to_add, *scan, *scan_previous;
	bool found, is_last;
	int i;
    
    // si alloca la struttura di tipo info_peer e si settano i campi dati
    to_add = (struct info_peer*) malloc(sizeof(struct info_peer));
	if (!to_add) {
		perror("Memoria esaurita");
	}
    to_add->port = port;
    to_add->ip_address = address;
    
    // si allinea, scan alla testa della lista, e scan_previous che sarà
    // il puntatore al peer precedente rispetto a scan lo poniamo inizialmente 
    // a 0, mentre found e is_last sono variabili di controllo
	scan = head;
	scan_previous = 0;
	found = false;
	is_last = false;

    // si scorre la lista e si esce quando trovo la posizione del peer giusta
    // oppure se [è finita, mentre non entro se la lista è vuota
	i = 1;
	while (scan != 0 && !found){
		if (to_add->port > scan->port) {
			scan_previous = scan;
			scan = scan->next;
			if(i == number_peer){
			    is_last = true;
			}
			i++;
		} 
        else if(to_add->port == scan->port){

            return false;
        }
		else{
			found = true;
		}
	}
	
	// si collega il puntatore next al peer successivo
	to_add->next = scan;
	
	// se la lista non è vuota
	if (scan_previous != 0){
		// si aggiorna il puntatore del peer precedente
		scan_previous->next = to_add;
		// si collega il puntatore previous al peer precedente
		to_add->previous = scan_previous;
		// se il peer che sto aggiungendo va inserito in fondo alla lista
		// si setta la variabile che contiene la porta dell'ultima porta nella lista
		if(is_last){
		    last = to_add->port;
		}
	}
	// caso di lista vuota o di inserimento in testa
	else{
	    // si aggiorna il puntatore alla testa della lista
		head = to_add;
	    // si collega il puntatore previous al peer precedente, in questo caso
		// non essendoci niente a zero
		to_add->previous = 0;
		// si setta first e nel caso del primo inserimento anche last
		if(number_peer == 0){
		    last = to_add->port;
		}
		first = to_add->port;
	}
	
	// si aggiorna il numero totale di peer sulla rete
	number_peer++;
	// si riallineano i puntatori previous, per l'inserimento fuori ordine
	scan_previous = 0;
	for(scan = head; scan != NULL; scan = scan->next){
	    if(scan_previous != scan->previous){
	        scan->previous = scan_previous;
	    }
	    scan_previous = scan;
	}

    return true;
}

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
struct message *create_message(uint8_t type, uint8_t *info, uint16_t port, uint8_t *address, uint32_t data, uint8_t *payload){

    struct message *mex = (struct message*)malloc(sizeof(struct message));

    mex->type = type;
    mex->info =  info;
    mex->port = port; 
    mex->address =  address;
    mex->data = data;
    mex->payload =  payload;

    mex->port= htons(mex->port);
    mex->data = htonl(mex->data);
    return mex;
}

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
int get_number_entry(int line_toread, int char_to_start_read, char elements[][LINE_SIZE]){

    char quantity_read[VALUE_LEN]; 
    int char_toread;
    int value;

    char_toread = char_to_start_read;
    memset(quantity_read, 0, sizeof(quantity_read));
    // la entry è terza in ogni file e si parte da 0
    while(elements[line_toread][char_toread] != ';'){
        // salvo il valore della entry sotto forma di stringa
        quantity_read[char_toread - char_to_start_read] = elements[line_toread][char_toread];
        char_toread++;
    }
    value = atoi(quantity_read);

    return value;
}


