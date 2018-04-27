/**
	Nome : Simone
	Cognome : Schirinzi
	
	Programma : Wator

	Si dichiara che il contenuto di questo file e' in ogni sua parte opera originale dell'autore.

	Simone Schirinzi
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "wator.h"
#include <string.h>

/** I seguenti valori sono il risultato di string_to_int a "sd","sb" e "fb" */
#define SSD 215
#define SSB 213
#define SFB 200

/** Indica il percorso del file di configurazione del pianeta */
#define CONF_FILE "./wator.conf"

/**
VARIABILI GLOBALI
	Fish, water e shark indicano , dato un punto (x,y), il numero di pesci, squali e spazi vuoti nelle 
	posizioni (x-1,y), (x+1,y), (x,y-1) e (x,y+1).

	Fish_p[4][2] e water_p[4][2] indicano invece dove eventualmente si trovano , dato un punto (x,y), acqua 
	e pesci. Si tratta sostanzialmente di un array di punti, di dimensione fish e water.
	
	Is_born controlla se l'animale e' nato. Viene aggiornato da shark_rule2 e fish_rule4.
*/
int fish, water, shark, fish_p[4][2], water_p[4][2];

/** Controllo delle nascite */
int is_born;

/*****************************INIZIO FUNZIONI*********************************/

int get_is_born(){return is_born;}

/** Converte in char gli elementi di tipo cell_t */
char cell_to_char (cell_t a){
	switch(a){
		case WATER:
			return 'W';
			break;
		case SHARK:
			return 'S';
			break;
		case FISH:
			return 'F';
			break;
		default:
			errno = EINVAL;
			return '?';
			break;
	}
}

/** Converte in cell_t gli elementi di tipo char */
int char_to_cell (char c){
	switch(c){
		case 'W':
			return WATER;
			break;
		case 'S':
			return SHARK;
			break;
		case 'F':
			return FISH;
			break;
		default:
			errno = EINVAL;
			return -1;
			break;
	}
}

/** Data una coppia (righe,colonne) crea un nuovo pianeta inizializzandone le variabili . Se si verifica un errore durante l'allocazione, libero tutta la memoria occupata */
planet_t * new_planet (unsigned int nrow, unsigned int ncol){
	/* Indici per visitare la matrice */
	unsigned int i , j;
	
	/* Il pianeta da creare */
	planet_t *new;

	/* Controllo sull'input */
	if(nrow == 0 || ncol == 0){
		errno = EINVAL;
		return NULL;		
	}

	/* Allocazione del pianeta */
	if((new = (planet_t *)malloc(sizeof(planet_t)))==NULL){ 
		errno = ENXIO;
		return NULL;
	}

	/* Inizializzo le dimensioni del pianeta */
	new->nrow = nrow;
	new->ncol = ncol;

	/* Alloco il mondo */
	if((new->w = malloc(new->nrow * sizeof(cell_t *))) == NULL){
		free(new);   
		errno = ENXIO;
		return NULL;
	}
	
	/* Alloco la matrice per il controllo delle nascite */
	if((new->btime = malloc(new->nrow * sizeof(int *))) == NULL){
		free(new->w);
		free(new);
		errno = ENXIO;
		return NULL;
	}
	
	/*Alloco la matrice per il controllo delle morti */
	if((new->dtime = malloc(new->nrow * sizeof(int *)))== NULL){
		free(new->w);
		free(new->btime);
		free(new);
		errno = ENXIO;
		return NULL;
	}

	/* Si allocano le rimanenti strutture necessarie, e le si deallocano in caso di errore*/
	for(i=0;i<(new->nrow);i++){
		new->w[i] = malloc(new->ncol * sizeof(cell_t));
		new->btime[i] = malloc(new->ncol * sizeof(int));
		new->dtime[i] = malloc(new->ncol * sizeof(int));
		if(new->w[i] == NULL || new->btime[i] == NULL || new->dtime[i] == NULL ){
			errno = ENXIO;
			for(j=0;j<=i;j++){
				if(new->w[j] != NULL)free(new->w[j]);
				if(new->btime[j] != NULL)free(new->btime[j]);
				if(new->dtime[j] != NULL)free(new->dtime[j]);
			}
			if(new->w != NULL)free(new->w);
			if(new->btime != NULL)free(new->btime);
			if(new->dtime != NULL)free(new->dtime);
			if(new != NULL)free(new);
			return NULL;
		}
	}

	/* Si inizializzano le strutture, inondando il pianeta, e azzerando i contatori di nascite e morti */
	for(i=0;i<(new->nrow);i++){
		for(j=0;j<(new->ncol);j++){
			new->w[i][j] = WATER;
			new->btime[i][j] = (unsigned)0;
			new->dtime[i][j] = (unsigned)0;
		}
	}
	
	return new;
}

/**
Le funzioni
	(*) is_valid_planet
	(*) is_valid_wator
controllano che gli argomenti passati come parametro (planet_t o wator_t) siano "validi".
Ovvero visitano la struttura , e segnalano eventuali NULL pointer.
Cio' mi consente di essere sicuro di poter accedere ai dati al loro interno senza SIGSEGV.
*/
int is_valid_planet(planet_t *p){
	unsigned int i;
	if(p == NULL){return 0;}
	if(p->w == NULL || p->btime == NULL || p->dtime == NULL){return 0;}
	for(i=0;i<(p->nrow);i++){
		if(p->w[i] == NULL || p->btime[i] == NULL || p->dtime[i] == NULL){return 0;}
	}
	return 1;
}

int is_valid_wator(wator_t *pw){
	if(pw == NULL){return 0;}
	if(!(is_valid_planet(pw->plan))){return 0;}
	return 1;
}

/* Semplice funzione per lo scambio di due interi tramite indirizzo */
void scambia(int *a,int *b){
	int temp = *a;
	*a = *b;
	*b = temp;
}

/* Dealloca un pianeta */
void free_planet (planet_t* p){
	/* Indici per scorrere la matrice */
	unsigned int i;
	
	/* Controllo sulla validita' dell'input */
	if(!is_valid_planet(p)){
		errno = EINVAL;
	}

	/* Dealloco la matrice */
	for(i=0;i<p->nrow;i++){
		free(p->w[i]);
		p->w[i] = NULL;
		free(p->btime[i]);
		p->btime[i] = NULL;
		free(p->dtime[i]);
		p->dtime[i] = NULL;
	}

	free(p->w);
	p->w = NULL;
	free(p->btime);
	p->btime = NULL;
	free(p->dtime);
	p->dtime = NULL;
	
	/* Dealloco il pianeta */
	free(p);
	p = NULL;
}

/* 
Dato un file f di output , che si suppone aperto, vi si scrive dento il pianeta p,
secondo il format dato nelle istruzioni.
*/
int print_planet (FILE* f, planet_t* p){
	/* indici per scorrere il pianeta*/
	unsigned int i, j;
	
	/* Controllo sull'input */
	if(f == NULL || (!is_valid_planet(p))){
		errno = EINVAL;
		return -1;
	}
	
	/* Stampo righe e colonne */
	if(fprintf(f,"%d\n%d\n",p->nrow,p->ncol) == EOF ){
		errno = EIO;
		return -1;
	}

	/* Stampo il pianeta */
	for(i=0;i<(p->nrow);i++){
		for(j=0;j<(p->ncol);j++){
			/*Prima stampo un carattere*/
			if( putc( cell_to_char(p->w[i][j]) ,f ) == EOF ){
				errno = EIO;
				return -1;
			}
			
			/* Poi stampo uno spazio , a meno che non devo andare a capo*/
			if(j != (p->ncol - 1)){
				if( putc(' ',f) == EOF ){
					errno = EIO;
					return -1;
				}
			}
		}
		
		/* A fine riga si va a capo */
		if(putc('\n',f) == EOF){
				errno = EIO;
				return -1;
			return -1;
		}
	}
	return 0;
}

/**
Dato un file f, che si suppone aperto, si legge al suo interno e si inizializza un pianeta ( che viene creato con new_planet).
*/
planet_t* load_planet (FILE* f){
	/* Indici per scorrere la matrice*/
	unsigned int i, j;
	
	/* variabili in cui vengono salvati i valori di righe e colonne del pianeta */
	unsigned int uno, due; 
	
	/*  Conserva il carattere acquisito da getc. */
	char temp; 
	
	/* Il pianeta */
	planet_t *planet;

	/* Controllo sull'input */
	if(f == NULL){ 
		errno = EINVAL;
		return NULL;
	}

	/* Input righe */
	if(fscanf(f,"%u",&uno) == EOF){
		errno = ERANGE;
		return NULL;
	}

	/* input colonne */
	if(fscanf(f,"%u",&due) == EOF){
		errno = ERANGE;
		return NULL;
	}

	/* Creazione nuovo pianeta */
	if((planet = new_planet(uno,due)) == NULL){
		errno = EPERM;
		/*free_planet(planet);*/
		return NULL;
	}
	
	/* Inizializzo gli indici */
	i = j = 0; 
	
	/*
	Fino a fine file prendo in analisi ogni carattere presente nel file.
	Se ottengo un carattere valido, lo converto e lo metto nella posizione idonea nel pianeta.
	I e j servono per arrivare in tutte le posizioni del pianeta.
	*/
	temp = getc(f);
	do{
		temp = getc(f);
		switch(temp){
			case EOF :
				break;
			case ' ' :
				break;
			case '\n' :
				i++;
				j = 0;
				break;
			default : 
				if((temp == 'W' || temp == 'S'|| temp == 'F') && (i<(planet->nrow)) && (j<(planet->ncol)) ){
					planet->w[i][j] = char_to_cell(temp);
					j++;
				}
				else{	
					errno = ERANGE;
					return NULL;
				}
				break;
		}
	}while(temp != EOF);
	return planet;
}	

/** Data una stringa in input, restituisce un intero che e' la somma dei codici ASCII dei caratteri presenti in essa.*/
int string_to_int(char *string){
	/* Indice per scorrere la stringa */
	int i;
	
	/* variabile per contenere la somma dei vari valori */
	int sum;

	/* Controllo sull'input*/
	if(string == NULL){
		errno = EINVAL;
		return -1;
	}
	
	/*Inzializazzione delle variabile */
	sum = i = 0;
	
	/* Scorro la stringa e incremento sum */
	while(string[i] != '\0'){
		sum += string[i];
		i++;
	}
	
	/* restituisco il valore calcolato */
	return sum;
}

/**
La funzione prende in input una stringa indicante la posizione del file
contenente la configurazione del pianeta.
Apre il file, invoca la load_planet, e lo chiude.
Apre il file contente la configurazione del wator ( predeterminato da CONF_FILE ).
La funzione entra quindi in un ciclo dove legge il file linea per linea,
costruendo la coppia <stringa,valore>, e inizializza il wator in modo opportuno.
*/
wator_t* new_wator (char* fileplan){
	/* Qui verranno aperti e gestiti i file PLANET e WATOR*/
	FILE *f = NULL;
	
	/* Il mio wator */
	wator_t *new;

	/* Conterra' la stringa presa in input da getline */
	char *string_temp;
	
	/* Contiene la stringa contenente il valore per settare la simulazione */
	char *numb_temp;
	
	/* lunghezza stringa restituito della getline */
	size_t len = 0;

	/* esito della getline */
	ssize_t read;
	
	/* Controllo dell'input */
	if(fileplan	== NULL){
		errno = EINVAL;
		return NULL;
	}

	/* Creo il wator */
	if((new = (wator_t *)malloc(sizeof(wator_t))) == NULL){
		errno = EPERM;
		return NULL;
	}
	
	/* Inizializzo preventivamente tutti i campi a zero. */
	new->sd = new->sb = new->fb = new->nf = new->ns = new->nwork = new->chronon = 0;

	/* Apro il file con la configurazione del pianeta. */
	if((f = fopen(fileplan,"r")) == NULL ){
		errno = EIO;
		return NULL;
	}
	
	/* Configuro il pianeta */
	new->plan = load_planet(f);
	if(!is_valid_wator(new)){
		errno = EIO;
		return NULL;
	}	
	
	/* chiudo il file */
	fclose(f);
	f = NULL;	

	/* apro il file contenente la configurazine del wator */ 
	if((f = fopen(CONF_FILE,"r")) == NULL ){
		errno = EIO;
		return NULL;
	}
	
	/**
		Leggo il file linea per linea, associando i valori di conseguenza.
		GetLine mi da una stringa contentente tutta la linea letta.
		Strtok_r la divide in occorrenza dello spazio , restituendo la prima parte, e salvando in numb_temp la seconda.
		strtod converte la stringa in un numero.
	*/
	while ((read = getline(&string_temp, &len, f)) != -1) {
			switch(string_to_int(strtok_r(string_temp," ",&numb_temp))){
				case SSD :
					new->sd = (int)(double)strtod(numb_temp,NULL);
					break;
				case SSB :
					new->sb = (int)(double)strtod(numb_temp,NULL);
					break;
				case SFB :
					new->fb = (int)(double)strtod(numb_temp,NULL);
					break;
				case -1 :
					errno = EPERM;
				default :
					errno = EIO;
					return NULL;
			}
		if(string_temp != NULL)free(string_temp);
		string_temp = NULL;
	}
	if(string_temp != NULL) free(string_temp);
	
	/* Dopo aver deallocato le strutture, chiudo il file e restituisco il pianeta creato */
	fclose(f);
	return new;
}

/* Dealloca un wator */
void free_wator(wator_t* pw){
	/* Controllo sull'input */
	if(!(is_valid_wator(pw))){
		errno = EINVAL;
	}
	
	/* Deallocazione del pianeta */
	free_planet(pw->plan);
	free(pw);
	pw=NULL;
}

/* La funzione inizializza fish, water, shark, fish_p[], water_p[]. */
void make_cross(wator_t* pw, int x, int y){
	/* Limite di righe del pianeta */
	int upx = pw->plan->nrow;

	/* Limite di colonne del pianeta */
	int upy = pw->plan->ncol;	
	
	/* xpu : x piu' uno . xmu = x meno uno . ypu : y piu' uno . ymu : y meno uno */
	int xpu = x+1 , xmu = x-1 , ypu = y+1 , ymu = y-1;

	/*
	N,S,E,O indicano cosa c'e' in  
	N : (x-1,y)
	S : (x+1,y)
	E : (x,y+1)
	O : (x,y-1)
	*/
	cell_t N,S,E,O;
	
	/* Controllo sull'input */
	if( (!(is_valid_wator(pw))) || x<0 || y<0 ||  (x >= (pw->plan->nrow))  || (y >= (pw->plan->ncol)) ){
		errno = EINVAL;
	}
	
	/* Azzero i contatori */
	water = 0;
	fish = 0;
	shark = 0;
	
	/* Rendo valide le coordinate */
	if(xpu >= upx) xpu = 0;
	if(xmu < 0 ) xmu = upx-1;
	if(ypu >= upy) ypu = 0;
	if(ymu < 0 ) ymu = upy-1;
	
	/* Inizializzo N,S,E,O*/
	N = pw->plan->w[xmu][y];
	S = pw->plan->w[xpu][y]; 
	E = pw->plan->w[x][ypu]; 
	O = pw->plan->w[x][ymu];
	
	/* 
		Creazione degli array di punti fish_p e water_p , e dei contatori.
		Per ogni posizione possibile, vedo cosa c'e' dentro, quindi incremento il contatore correlato, e salvo le posizioni nel relativo array.
	*/
	if(N==FISH){fish++; fish_p[fish-1][0] = (xmu); fish_p[fish-1][1] = (y);}
	if(N==WATER){water++; water_p[water-1][0] = (xmu); water_p[water-1][1] = (y);} 
	if(N==SHARK){shark++;}
							
	if(S==FISH){fish++; fish_p[fish-1][0] = (xpu); fish_p[fish-1][1] = (y);}
	if(S==WATER){water++; water_p[water-1][0] = (xpu); water_p[water-1][1] = (y);} 
	if(S==SHARK){shark++;}
							
	if(E==FISH){fish++; fish_p[fish-1][0] = (x); fish_p[fish-1][1] = (ypu);}
	if(E==WATER){water++; water_p[water-1][0] = (x); water_p[water-1][1] = (ypu);} 
	if(E==SHARK){shark++;}
							
	if(O==FISH){fish++; fish_p[fish-1][0] = (x); fish_p[fish-1][1] = (ymu);}
	if(O==WATER){water++; water_p[water-1][0] = (x); water_p[water-1][1] = (ymu);} 
	if(O==SHARK){shark++;}
}

/*
Dato un array array_p( tra fish_p e water_p ), 
un pianeta pw da modificare,
un modulo module su cui eseguire la rand() ( tra water fish e shark ),
un animale animal ( tra fish e shark ) da inserire nel pianeta,
e un indirizzo <k,l> che restituisce dove e' stato modificato il pianeta,
la funzione aggiorna il pianeta.
*/ 
void smista(wator_t *pw, int array_p[4][2], int module, cell_t animal, int *k, int *l){
	/* Controllo la validita' dell'input */
	if( module < 0 || module > 4 || (!(is_valid_wator(pw))) ){
		errno = EINVAL;
	}

	/* Casualmente inserisco l'animale in una delle posizioni libere nel relativo array , e aggiorno k e l .*/
	switch(rand()%(module)){
		case 0:	
			pw->plan->w[array_p[0][0]][array_p[0][1]] = animal;
			*k = (array_p[0][0]); 
			*l = (array_p[0][1]);
			break;
		case 1:	
			pw->plan->w[array_p[1][0]][array_p[1][1]] = animal;
			*k = (array_p[1][0]); 
			*l = (array_p[1][1]);
			break;
		case 2:	
			pw->plan->w[array_p[2][0]][array_p[2][1]] = animal;
			*k = (array_p[2][0]); 
			*l = (array_p[2][1]);
			break;
		case 3:	
			pw->plan->w[array_p[3][0]][array_p[3][1]] = animal;
			*k = (array_p[3][0]); 
			*l = (array_p[3][1]);
			break;
	}
}

/* Implementa la shark_rule1 */
int shark_rule1 (wator_t* pw, int x, int y, int *k, int *l){
	/* Controlla che l'input sia valido */
	if( (!(is_valid_wator(pw))) || (x<0) || (x>=(pw->plan->nrow)) || (y<0) || (y>=(pw->plan->ncol)) || (pw->plan->w[x][y] != SHARK) ){
		errno = EINVAL;
		return -1;
	}
	
	/* Genero valori validi per la "Croce" intorno allo squalo */
	make_cross(pw,x,y);
	
	/* Ci sono solo squali: Sto fermo*/
	if(shark==4){ 
		*k = x; *l = y;
		return STOP;
	}
	
	/* c'e' un azione da fare, lo squalo di spostera' di sicuro e lasciera' acqua al suo posto */
	pw->plan->w[x][y] = WATER;
	
	/*Non essendoci solo squali, controllo se ci sono o meno pesci*/
	if(fish==0){ 
		/* Non ci sono pesci : lo squalo si muove soltanto */
		smista(pw,water_p,water,SHARK,k,l);
		
		/*Aggiorno btime e dtime*/
		scambia(&(pw->plan->btime[x][y]),&(pw->plan->btime[*k][*l]));
		scambia(&(pw->plan->dtime[x][y]),&(pw->plan->dtime[*k][*l]));
		return MOVE;	
	}
	else{
		if(fish>0){
			/* Ci sono pesci: Lo squalo ne mangia uno a caso */
			smista(pw,fish_p,fish,SHARK,k,l);
			
			/* Aggiorno btime e dtime */
			scambia(&pw->plan->btime[x][y],&pw->plan->btime[*k][*l]);
			scambia(&pw->plan->dtime[x][y],&pw->plan->dtime[*k][*l]);
			
			/* Azzero il contatore del pesce, essendo morto */
			pw->plan->btime[x][y]=(unsigned)0;
			pw->plan->dtime[x][y]=(unsigned)0;
			return EAT;
		}
	}
	
	/* Shark != 4 o fish < 0 */
	errno = ESPIPE;
	return -1;
}

/* Implementa la seconda regola ( Riproduzione e morte dello squalo ) */
int shark_rule2 (wator_t* pw, int x, int y, int *k, int* l){
	/* Controllo la validita' dell'input */
	if( (!(is_valid_wator(pw))) || (x<0) || (x>=(pw->plan->nrow)) || (y<0) || (y>=(pw->plan->ncol)) || (pw->plan->w[x][y] != SHARK) ){
		errno = EINVAL;
		return -1;
	}
	
	is_born = 0;
	
	/* Genero valori validi per la "Croce" intorno allo squalo */
	make_cross(pw,x,y);
	
	/* Se e' passato abbastanza tempo, provo a generare lo squalo */	
	if(pw->plan->btime[x][y] < pw->sb){
		/* Lo squalo e' ancora "giovane", incremento il suo contatore */
		pw->plan->btime[x][y] += 1;
	}
	else{
		if ((pw->plan->btime[x][y] == pw->sb)){
			/* lo squalo e' pronto a riprodursi */
			pw->plan->btime[x][y] = 0;

			/* Controllo se c'e' posto per un nuovo squalo */
			if (water > 0){
				/* eseguo la creazione del nuovo squalo*/
				smista(pw, water_p, water, SHARK, k, l);

				/* setto is_born */
				is_born = 1;
			}
			else{
				if (water < 0){
					errno = ESPIPE;
					return -1;
				}
			}
		}
		else{
			errno = EPERM;
			return -1;
		}
	}
	
	/* Se e' passato troppo tempo, lo squalo muore */
	if(pw->plan->dtime[x][y] < pw->sd){
		/* lo squalo non e' abbastanza vecchio */
		pw->plan->dtime[x][y] += 1;
		return ALIVE;
	}
	else{
		if(pw->plan->dtime[x][y] == pw->sd){
			/* lo squalo muore*/
			/* Lascia acqua al suo posto */
			pw->plan->w[x][y] = WATER;
			
			/* azzero btime e dtime */
			pw->plan->btime[x][y]=(unsigned)0;
			pw->plan->dtime[x][y]=(unsigned)0;
			return DEAD;
		}
		else{
			errno = ESPIPE;
			return -1;
		}
	}
	
	/* Btime e Dtime inconsistenti */
	errno = ESPIPE;
	return -1;

}

/* Implementa la prima regola dei pesci : Si muove o sta fermo */
int fish_rule3 (wator_t* pw, int x, int y, int *k, int* l){
	/* Controllo che l'input sia valido*/
	if( (!(is_valid_wator(pw))) || (x<0) || (x>=(pw->plan->nrow)) || (y<0) || (y>=(pw->plan->ncol)) || (pw->plan->w[x][y] != FISH) ){
		errno = EINVAL;
		return -1;
	}

	/* Genero valori validi per la "Croce" intorno al pesce */
	make_cross(pw,x,y);

	/* Se c'e' acqua , il pesce si sposta. Altrimenti resta fermo */
	if(water > 0){
		/*spostandosi, lascia acqua dietro di se*/
		pw->plan->w[x][y] = WATER;	
		smista(pw,water_p,water,FISH,k,l);
		
		/* si aggiorna la btime */
		scambia(&pw->plan->btime[x][y],&pw->plan->btime[*k][*l]);
		return MOVE;
	}
	else{
		if(water == 0){
			/* non c'e' posto per muoversi : sta fermo */
			return STOP;
		}
		else{
			errno = ESPIPE;
			return -1;
		}
	}

	errno = ESPIPE;
	return -1;
}

/* Implementa la seconda regola dei pesci : Si riproducono */
int fish_rule4(wator_t* pw, int x, int y, int *k, int* l){
	/* Controllo che l'input sia valido */
	if( (!(is_valid_wator(pw))) || (x<0) || (x>=(pw->plan->nrow)) || (y<0) || (y>=(pw->plan->ncol)) || (pw->plan->w[x][y] != FISH) ){
		errno = EINVAL;
		return -1;
	}

	is_born = 0;

	/* Controllo l'eta' della cella e la incremento. */ 
	if(pw->plan->btime[x][y] < pw->fb){
		pw->plan->btime[x][y] += 1;
		return 0;
	}

	/* La cella e' matura. Provo a generare il pesce. */
	if(pw->plan->btime[x][y] == pw->fb){
		pw->plan->btime[x][y] = 0;
		make_cross(pw,x,y);
		
		/* controllo la presenza di uno spazio libero */
		switch(water){
			case 0 :
				break;
			default :
				/* Inserisco il nuovo pesce */
				smista(pw,water_p,water,FISH,k,l);
				
				/*setto is_born*/
				is_born = 1; 
				break; 
		}
		return 0;
	}
	else{
		errno = EPERM;
		return -1;
	}
	
	errno = EINVAL;
	return -1;
}

/* Conta i pesci nel mondo */
int fish_count (planet_t* p){
	/* Contatore per i pesci */
	int fish_number = 0;

	/* Valori indice */
	unsigned int i, j;	

	/* Controllo sull'input */
	if(!(is_valid_planet(p))){
		errno = EINVAL;
		return -1;
	}
	
	/* visito il pianeta, contando i pesci al suo interno */
	for(i=0;i<(p->nrow);i++){
		for(j=0;j<(p->ncol);j++){
			if(p->w[i][j] == FISH){
				fish_number++;
			}
		}
	}
	return fish_number;
}

/* Conta gli squali nel mondo */
int shark_count (planet_t* p){
	/* Valori indice */
	unsigned int i, j;

	/* Contatore per gli squali */
	int shark_number = 0;

	/* Controllo sull'input */
	if(!(is_valid_planet(p))){
		errno = EINVAL;
		return -1;
	}

	/* visito il pianeta contando gli squali. */
	for(i=0;i<(p->nrow);i++){
		for(j=0;j<(p->ncol);j++){
			if(p->w[i][j] == SHARK){
				shark_number++;
			}
		}
	}
	return shark_number;
}

/* Aggiorna il mondo */
int update_wator (wator_t * pw){
	/* Pesci e squali sono gli array in cui conservero' la mia lista di pesci e squali. Gli altri 2 valori indicano la loro dimensione */
	int *pesci = NULL, *squali = NULL, pesci_dim = 0, squali_dim = 0;

	/* Indici per scorrere la matrice */
	unsigned int i, j;

	/* Valori temporanei per il controllo delle nascite */
	int x_succ , y_succ;

	/* Controlla la validita' dell'input */
	if(!(is_valid_wator(pw))){
		errno = EINVAL;
		return -1;
	}

	/* Conto quanti pesci ci sono */
	if((pw->nf = fish_count(pw->plan)) == -1 ){
		errno = EPERM;
		return -1;
	}
	
	/* Conto quanti squali ci sono */
	if((pw->ns = shark_count(pw->plan))== -1 ){
		errno = EPERM;
		return -1;
	}
	
	/* Inizializzo gli array */
	if((pesci = (int *)malloc((2*(pw->plan->ncol)*(pw->plan->nrow))*sizeof(int))) == NULL ){
		errno = ENXIO;
		return -1;
	}
	
	if((squali = (int *)malloc((2*(pw->plan->ncol)*(pw->plan->nrow))*sizeof(int))) == NULL ){
		errno = ENXIO;
		free(pesci);
	}

	/* 
		Pesci e squali sono degli array di punti lineari. Gli elementi in posizione pari sono il campo x delle coordinate della cella. Quelli in posizione dispari , il campo y. L'array si scorre a salti di due.
		Gli inserimenti avvengono in coda, le cancellazioni avvengono spostando in coda l'elemento da cancellare ( invertendono con quello realmente in coda ) e riducendo la dimensione dell'array.
	*/
	
	/* Eseguo un passo della simulazione ( Prima si muovono, poi nascono ). */
	
	/* I primi a muoversi sono i pesci. Infatti essi non possono morire. Muovendo  prima gli squali avrei rischiato di uccidere un pesce pronto a ripordursi, rischiando il collasso dell'ecosistema */
	
	/* Cerco i pesci */
	for(i=0;i<(pw->plan->nrow);i++){
		for(j=0;j<(pw->plan->ncol);j++){
			if(pw->plan->w[i][j] == FISH){
				pesci[pesci_dim] = i;
				pesci[pesci_dim + 1] = j;
				pesci_dim += 2;
			}
		}
	}

	/* Per ogni pesce */
	for(i=0;i<pesci_dim;i += 2){
		/* I pesci si muovono */
		switch(fish_rule3(pw,pesci[i],pesci[i+1],&pesci[i],&pesci[i+1])){
			case -1 : 
				errno = EINVAL;
				free(pesci);
				free(squali);
				return -1;
				break;
			default :
				break;
		}

		/* inizializzo is_born */
		is_born = 0;
		
		/* I pesci si riproducono */
		switch(fish_rule4(pw,pesci[i],pesci[i+1],&x_succ,&y_succ)){			
			case -1 : 
				errno = EINVAL;
				free(pesci);
				free(squali);
				return -1;
				break;
			case 0 :
				if(is_born) { pw->nf += 1; }
				break;
			default :
				errno = EINVAL;
				return -1;
				break;
		}
	}
	
	/* Cerco gli squali */
	for(i=0;i<(pw->plan->nrow);i++){
		for(j=0;j<(pw->plan->ncol);j++){
			if(pw->plan->w[i][j] == SHARK){
				squali[squali_dim] = i;
				squali[squali_dim + 1] = j;
				squali_dim += 2; 
			}
		}
	}
	
	/* Generato il loro cibo, e' il turno degli squali di muovere. */
	for(i=0;i<squali_dim;i+=2){
		switch(shark_rule1(pw,squali[i],squali[i+1],&squali[i],&squali[i+1])){
			case EAT :
				/* un pesce e' stato mangiato */
				pw->nf -= 1;
				break;
			case -1 : 
				errno = EINVAL;
				free(pesci);
				free(squali);
				return -1;
				break;
			default :
				break;
		}
		
		switch(shark_rule2(pw,squali[i],squali[i+1],&x_succ,&y_succ)){			
			case DEAD :
				/* E' morto lo squalo */
				pw->ns -= 1;
				break;
			case -1 : 
				errno = EINVAL;
				free(pesci);
				free(squali);
				return -1;
				break;
			case 0 :
				if (is_born) pw->ns += 1;
				break;
			default :
				break;
		}	
	}
	
	/* Aggiorna il timer della simulazione*/
	pw->chronon += 1;
	
	/* dealloco le mie strutture */
	free(pesci);
	free(squali);
	
	return 0;
}
