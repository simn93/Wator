/**
	Nome : Simone
	Cognome : Schirinzi

	Programma : Wator

	Si dichiara che il contenuto di questo file e' in ogni sua parte opera originale dell'autore.

	Simone Schirinzi
*/
#ifndef __AUXFUN__H
#define __AUXFUN__H

/** Piccola libreria per l'implementazioni di funzioni secondarie,
 *  e macro comuni ai due processi.
 */ 

#include "wator.h"

/* Macro per la gestione degli errori */
#define ec(a,b) if((a)==(b)){fprintf(stdout,"err:%d\n",__LINE__);perror("");fflush(stdout);_exit(EXIT_FAILURE);}
#define ecd(a,b) if((a)!=(b)){fprintf(stdout,"err:%d\n",__LINE__);perror("");fflush(stdout);_exit(EXIT_FAILURE);}
#define ecm(a,b) if((a)<(b)){fprintf(stdout,"err:%d\n",__LINE__);perror("");fflush(stdout);_exit(EXIT_FAILURE);}
#define ecs(a,b) if((a)==(b))_exit(EXIT_FAILURE);

/* socket max path */
#define UNIX_PATH_MAX 108
#define UNIX_MAX_PATH 108

/* nome della socket */
#define SOCKNAME "./visual.sck"

/* Dimensione delle stringhe standard usate per la comunicazione*/
#define STRING_SIZE 1024

/* Debug print */
#define debug fprintf(stdout,"Line: %d ok\n",__LINE__);fflush(stdout);
/**************STRUTTURE**************/
/* Gestione della coda */
typedef struct pnt{int x; int y;}point;
typedef struct x{	point from;	point to;	struct x *next;}elem;
typedef elem* list;
typedef struct y{	list head;	list tail;}list_guest;

/** La coda è composta da un unica struttura list_guest.
 * List_guest è una coppia di puntatori, uno all'inizio e uno alla fine della coda.
 * L'inserimento avviene il coda e l'estrazione in testa.
 * Ogni elemento della lista è una coppia di punti , 
 *  dove un punti è una coppia di interi.
 * I punti rappresentano un rettangolo, nel senso che:
 * (*) il punto from è quello in alto a sinistra.
 * (*) il punto to è quello in basso a destra.
 */ 

/**************FUNZIONI**************/

/** Planet to string converte un pianeta in una stringa.
 *  Converte ogni wator_t in un carattere ( tramite wator_to_char ),
 *  e li mette uno accanto all'altro.
 * 	I '.' rappresentano il punto di fine riga.
 */
char *planet_to_string(int PSL, wator_t *ecosistema);

/** Restituisce il maggiore tra i due numeri */
int max(int a,int b);

/** Restituisce il minimo tra i due numeri */
int min(int a,int b);

/** Dati i valori in input, insert inserisce in coda a taillist, 
 *  l'elemento formato dai punti:
 *  (*) From=(fx,fy)
 *  (*) To=(tx,tx) 
 */ 
void insert(list_guest *taillist,int fx, int fy, int tx, int ty);

/** Extract estrae l'elemento nella testa di taillist.
 *  Lo converte in un array di 4 elementi nel seguente ordine:
 *  { from.x, from.y, to.x, to.y }
 *  E lo restituisce al chiamante della funzione (il worker).
 */ 
int* extract(list_guest *taillist);

/** Dati in input:
 *  (*) La stringa planet_string da stampare.
 *  (*) Il path path del file il cui stampare la stringa.
 *  (*) Il numero di righe del pianeta da stampare.
 *  (*) Il numero di colonne del pianeta da stampare.
 *  (*) La dimensione (LOCAL_PLANET_STRING_LEN) della stringa planet_string da stampare.
 *  La funzione stampa il pianeta secondo i parametri dati.
 */ 
void print_string_planet(char* planet_string, FILE* path, int nrow, int ncol, int LOCAL_PLANET_STRING_LEN,int *fish_n, int *shark_n);

/** Crea un file wator_worker_w_id vuoto*/
void crea_wid_file(int w_id);


#endif
