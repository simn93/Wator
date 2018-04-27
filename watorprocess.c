/**
	Nome : Simone
	Cognome : Schirinzi
	Matricola :  490209
	Programma : Wator

	Si dichiara che il contenuto di questo file e' in ogni sua parte opera originale dell'autore.

	Simone Schirinzi
*/

/**************INCLUDE**************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <ctype.h>
#include "wator.h"
#include "auxfun.h"

/**************DEFINE**************/
/* Numero di worker e intervallo di aggiornamento standard */
#define NWORK_DEF 1
#define CHRON_DEF 2

/* Lunghezza della stringa di trasmissione */
#define PLANET_STRING_LEN (1+ecosistema->plan->nrow+(ecosistema->plan->nrow*ecosistema->plan->ncol))

/* Numero di righe da elaborare in ogni tile */
#define N 5

/**************VARIABILI GLOBALI**************/
/* Mutua eclusione e di condizione */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_chronon_cnd = PTHREAD_COND_INITIALIZER;
pthread_cond_t start_cnd = PTHREAD_COND_INITIALIZER;
pthread_cond_t stop_cnd = PTHREAD_COND_INITIALIZER;
pthread_cond_t acquire_cnd = PTHREAD_COND_INITIALIZER;

/* Ecosistema da gestire */
wator_t *ecosistema;

/* Numero di worker da usare e ogni quanti chronon aggiornare la simulazione */
int worker_n;		
int chronon_n;

/* Variabile di tick tra dispatcher e collector */
int new_loop = 1;

/* Variabile di tick tra worker e collector */
int work_done = 0;

/* Variabile di sincronizzazione per la corretta acquisizione del pianeta */
int acquire_done = 0;

/* Il file in cui stampa visualizer */
char *dumpfile_path;

/* Visualizer pid*/
pid_t visualizer_pid;

/* Il pianeta da caricare */
char* PLANET;

/* Matrice per il controllo delle morti dei pesci */
int **fish_death;

/* Tid dei thread in esecuzione */
pthread_t dispatcher_tr, collector_tr, *worker_tr, saver_tr;

/* Socket file descriptor*/
int csfd;

/* Salvataggio del pianeta e chiusura del processo*/
volatile sig_atomic_t save=0, wator_exit=0;

/* Chiusura dei worker */
int main_exit = 0;

/* Coda di sezioni da elaborare */
list_guest taillist;

/* Gestione dei segnali */
sigset_t set;
struct sigaction sa;

/* Flag per le statistiche del wator */
int auto_flag = 0;
/**************FUNZIONI**************/
static void *dispatcher();
static void *collector(void *df);
static void *worker(void *wid);
static int handle_signal(void);
static void SIGINT_SIGTERM_handle (int signum);
static void SIGUSR1_handle (int signum);
static void *saver();
void update_tile(int* tile);

int main(int argc, char **argv){
	/* Variabile di iterazione */
	int index;
	
	/* Allocazione di memora per le stringhe globali */
	ec((dumpfile_path=malloc(STRING_SIZE*sizeof(char))),NULL);
	ec((PLANET=malloc(STRING_SIZE*sizeof(char))),NULL);
	
	/* Gestione dei segnali */
	handle_signal();
	
	/* Gestione dei parametri di input.
	 * Worker e chronon sono inizializzati a -1, per poter in seguito controllare se sono stati modificati.
	 * Altrimenti vengono settati a parametri di default
	 */
	
	worker_n = chronon_n = -1;
	index=1;
	while( index < argc){
		switch(argv[index][1]){
			case 'n': worker_n = strtod(argv[index+1],NULL); index+=2;break;
			case 'v': chronon_n = strtod(argv[index+1],NULL); index+=2;break;
			case 'f': sprintf(dumpfile_path,"./%s",argv[index+1]); index+=2;break;
			case 'a': auto_flag = 1; index++; break;
			default : sprintf(PLANET,"./%s",argv[index]); index++;break;
		}
	}
	if(worker_n == -1)worker_n = NWORK_DEF;
	if(chronon_n == -1)chronon_n = CHRON_DEF;
	if(strlen(dumpfile_path) == 0)dumpfile_path="stdout";
	
	/* Caricamento della configurazione inziale del pianeta*/
	if((ecosistema = new_wator (PLANET)) == NULL){ exit(EXIT_FAILURE); }
	
	/* Inizializzo la simulazione */
	ecosistema->nwork = worker_n;
	ec((ecosistema->ns = shark_count(ecosistema->plan)),-1);
	ec((ecosistema->nf = fish_count(ecosistema->plan)),-1);
	ec(fish_death=malloc(ecosistema->plan->nrow * sizeof(int *)),NULL);
	for(index=0;index<ecosistema->plan->nrow;index++)ec((fish_death[index]=malloc(ecosistema->plan->ncol * sizeof(int))),NULL);
	
	/* Eliminazione di una precedente, eventuale, socket */
	if(access(SOCKNAME,F_OK)!=-1)ec((remove(SOCKNAME)),-1);
	
	/* Avvio del processo visualizer */
	if((visualizer_pid = fork()) == 0)ec(execl("./visualizer","visualizer",NULL),-1);
	ec(visualizer_pid,-1);
	
	/* Avvio dei thread dispatcher, collector, visualizer e saver */
	ec( (worker_tr = malloc(worker_n * sizeof(pthread_t))),NULL);
	ecd( pthread_create(&dispatcher_tr,NULL,&dispatcher,NULL),0);
	ecd( pthread_create(&collector_tr,NULL,&collector,NULL),0);
	for(index=0;index<worker_n;index++)
		ecd((pthread_create(&worker_tr[index],NULL,&worker,(void*)index)),0);
	
	/* Avvio del processo di salvataggio */
	ecd((pthread_create(&saver_tr,NULL,&saver,NULL)),0);
	
	/* Attesa di collector per terminazione gentile */
	ecd((pthread_join(collector_tr,NULL)),0)
	return 0;
}

/** Dispatcher ha il compito di iniziare un chronon di simulazione.
 * E' in attesa su new_loop, inizialmente a 1, e new_chronon_cnd, 
 *  che gli vengono spediti dal collector ad elaborazione finita.
 * Dispacher inserisce nella coda condivisa taillist 
 *  i rettangoli che verranno poi elaborati dai worker.
 * I rettangoli hanno dimensione N:5 * K:ncol.
 * Ad ogni inserimento vengono avvisati tutti i worker, 
 *  in modo che uno di loro possa iniziare la rispettiva elaborazione.
 * Finita la distribuzione, si rimette in attesa.
 * Viene terminato solo dal collector che setta a 1 main_exit.
 */ 
static void *dispatcher(){
	/* variabile di iterazione */
	int index, i, j;
		
	while(1){
		/* Attesa di fine chronon */
		ecd(pthread_mutex_lock(&mtx),0);
		while(!new_loop){ 
			if(main_exit){pthread_mutex_unlock(&mtx);pthread_exit(NULL);}
			ecd(pthread_cond_wait(&new_chronon_cnd,&mtx),0); }
		new_loop = 0;
		ecd(pthread_mutex_unlock(&mtx),0);
		
		/* Azzeramento fish_death */
		for(i=0;i<ecosistema->plan->nrow;i++)for(j=0;j<ecosistema->plan->ncol;j++)fish_death[i][j]=0;
					
		/* Creazione delle tile :
		 * N = 5
		 * K = ncol
 		 */
		for(index=0;index<(1+((int)((ecosistema->plan->nrow)/N)));index++){
			ecd(pthread_mutex_lock(&mtx),0);
			if(index+1 == worker_n){ /* Mi trovo nell'ultimo worker */
				insert(&taillist,index*N,0,ecosistema->plan->nrow,ecosistema->plan->ncol);
				index=1+((int)((ecosistema->plan->nrow)/N));
			}
			else{ 
				if( ( index * N  ) + N > ecosistema->plan->nrow ){ insert(&taillist,index*N,0,ecosistema->plan->nrow,ecosistema->plan->ncol); }
				else{ insert(&taillist,index*N,0,(index+1)*N,ecosistema->plan->ncol); }
			}
			ecd(pthread_cond_broadcast(&start_cnd),0);
			ecd(pthread_mutex_unlock(&mtx),0);
		}
	}
}

/** Worker è la funzione comune ai thread che gestiscono l'elaborazione del pianeta.
 * Alla creazione, crea un file vuoto wator_worker_wid, 
 *  dove wid è il suo numero identificativo, compreso tra 0 e worker_n.
 * Quindi si mette in attesa sulla coda, mettendosi in pausa di start_cnd 
 *  che gli arriva dal dispatcher.
 * Estrae il rettangolo da elaborare dalla coda, lo elabora,
 * 	avvisa il collector di aver finito incrementando work_done 
 *  e inviando il segnale di stop_cnd.
 * Quindi si rimette in attesa di un nuovo thread da elaborare.
 * Viene terminato solo dal collector che setta a 1 main_exit.
 */ 
static void *worker(void *wid){
	/* tile array */
	int *tile;
	ec(tile=malloc(4*sizeof(int)),NULL);
	
	/* Creazione del file wator_worker_wid */
	crea_wid_file((int)wid);
		
	while(1){
		/* Attesa di fine dispatcher */
		ecd(pthread_mutex_lock(&mtx),0);	
		while(taillist.head == NULL){
			if(main_exit){pthread_mutex_unlock(&mtx);pthread_exit(NULL);}
			ecd(pthread_cond_wait(&start_cnd,&mtx),0);
		}
		
		/* estrazione elemento dalla coda */
		tile = extract(&taillist);
		ecd(pthread_mutex_unlock(&mtx),0);
	
		/* elaborazione tile */
		update_tile(tile);
		
		/* Comunicazione di file elaborazione */
		ecd(pthread_mutex_lock(&mtx),0);
		work_done++;
		ecd(pthread_cond_signal(&stop_cnd),0);
		ecd(pthread_mutex_unlock(&mtx),0);
	}
}

/** Collector è il thread riservato alla sincronizzazione con gli altri thread,
 *  e alla comunicazione con il processo visualizer.
 * All'avvio si connette alla socket SOCKNAME creata dal visualizer, 
 *  quindi gli invia i parametri necessari per stampare il pianeta.
 * Nel loop principali, il thread aspetta che almeno x worker abbiano finito,
 *  dove x è il numero di rettangoli creati dal dispatcher o il numero di worker.
 * Quindi incrementa il chronon del pianeta, 
 *  e prova a inviare al visualizer una stringa rappresentante il pianeta,
 * 	dove gli spazi sono stati eliminati, e '\n' è rappresentato da '.' .
 * Dunque resetta il contatore dei work terminati, e invia il segnale al dispatcher
 *  per iniziare un nuovo chronon.
 * Il thread gestisce infine la terminazione gentile.
 * Se wator_exit è settato a 1 invia il segnale di terminazione al visualizer,
 *  aspettandolo.
 * Dunque imposta a 1 il flag main_exit, obbligando dispatcher e worker a terminare.
 * Per ultimo termina, facendo il terminare tutto il processo.
 */ 
static void *collector(void *df){
	/* Variabile di iterazione */
	int index;
	
	/* Stringhe per le ricezioni e gli invii delle comunicazioni */
	char temp[STRING_SIZE], *planet_string, temp1024[STRING_SIZE];
	
	/* Inizializzione socket */
	struct sockaddr_un sa;
	sa.sun_family = AF_UNIX;
	ec((strncpy(sa.sun_path,SOCKNAME,UNIX_MAX_PATH)),NULL);
	ec((csfd=socket(AF_UNIX,SOCK_STREAM,0)),-1);
	unlink(SOCKNAME);
	while(connect(csfd,(struct sockaddr*)&sa,sizeof(sa)) == -1) ;
	
	/* invio PLANET_STRING_LEN e il path del filedump */
	ecm(sprintf(temp,"%d",PLANET_STRING_LEN),0);
	ec(write(csfd,temp,STRING_SIZE),-1);
	ec(write(csfd,dumpfile_path,STRING_SIZE),-1);
	
	/* invio nrow , ncol */
	ecm(sprintf(temp,"%d",ecosistema->plan->nrow),0);
	ec(write(csfd,temp,STRING_SIZE),-1);
	ecm(sprintf(temp,"%d",ecosistema->plan->ncol),0);
	ec(write(csfd,temp,STRING_SIZE),-1);
	
	/* invio autoflag */
	ecm(sprintf(temp,"%d",auto_flag),0);
	ec(write(csfd,temp,STRING_SIZE),-1);
	
	while(1){
		/* Attesa di fine work */
		ecd((pthread_mutex_lock(&mtx)),0);
		while(work_done<min(((int)(ecosistema->plan->nrow/N)),worker_n))
			ecd((pthread_cond_wait(&stop_cnd,&mtx)),0);
		
		/* se è passato abbastanza tempo, o devo chiudere, stampo il pianeta*/
		ecosistema->chronon += 1;
		if( ( (ecosistema->chronon) % chronon_n) == 0 || wator_exit){
			planet_string = planet_to_string(PLANET_STRING_LEN,ecosistema);
			if(PLANET_STRING_LEN<=STRING_SIZE){
				errno=0;
				write(csfd,(char*)planet_string,STRING_SIZE);
				if(errno != EPIPE && errno != 0 && errno != ECONNRESET)ec(0,0);
			}
			else{
				for(index=0;index<1+(int)(PLANET_STRING_LEN/STRING_SIZE);index+=STRING_SIZE){
					strncpy(temp1024,*(&planet_string+index),STRING_SIZE);
					errno=0;
					write(csfd,(char*)temp1024,STRING_SIZE);
					if(errno != EPIPE && errno != 0 && errno != ECONNRESET)ec(0,0);
				}
			}
		}
		
		/* Resetto le variabili e inizio un nuovo chronon o termino il processo*/
		if(wator_exit){
			/* Terminazione visualizer */
			ec((kill(visualizer_pid,SIGINT)),-1);
			ec((kill(visualizer_pid,SIGTERM)),-1);
			ec(waitpid(visualizer_pid,(int*)NULL,(int)NULL),-1);
			
			/* Chiusura socket */
			close(csfd);
			
			/* Terminazione thread */
			main_exit=1;
			ecd(pthread_cond_signal(&new_chronon_cnd),0);
			ecd(pthread_cond_broadcast(&start_cnd),0);
			ecd(pthread_mutex_unlock(&mtx),0);
			assert(taillist.head==NULL);
			ecd(pthread_join(dispatcher_tr,NULL),0);
			for(index=0;index<worker_n;index++)ecd(pthread_join(worker_tr[index],NULL),0);
			
			/* Terminazione wator */
			pthread_exit(NULL);
		}
		else{
			/* Contatore delle tile elaborate azzerato */
			work_done = 0;
			
			/* Contatore di tile lette azzerato */
			acquire_done = 0;
			
			/* Flag per il dispatcher */
			new_loop = 1;
			ecd(pthread_cond_signal(&new_chronon_cnd),0);
			ecd(pthread_mutex_unlock(&mtx),0);
		}
	}
}

/** Saver è il thread riservato al salvataggio del pianeta.
 *E' in attesa su "save", che viene modificato solo alla ricezione di SIGURS1, dall'apposito signal handler.
 *Stampa il pianeta sul wator.check e si rimette in attesa.  
 */
static void *saver(){
	FILE *printfile;
	while(1){
		if(save){
			ec(printfile = fopen("./wator.check","w+"),NULL);
			ec(print_planet(printfile,ecosistema->plan),-1);
			ec((fclose(printfile)),EOF);
			save=0;
		}
	}
}

/** Gestione dei segnali.
 * Si setta temporaneamente una maschera per ignorare tutti i segnali.
 * Quindi si associa SIGINT_SIGTERM_handle e SIGUSR1_handle,
 *  per la gestione dei rispettivi segnali.
 * Infine si ripristinano i segnali modificati.
 */
 static int handle_signal(void){
	ec(sigfillset(&set),-1);
	ec(pthread_sigmask(SIG_SETMASK,&set,NULL),-1);
	
	memset(&sa,0,sizeof(sa));

	sa.sa_handler = SIG_IGN;
	ec((sigaction(SIGPIPE,&sa,NULL)),-1);

	sa.sa_handler = SIGINT_SIGTERM_handle;
	sa.sa_flags=SA_RESTART;
	ec((sigaction(SIGINT,&sa,NULL)),-1);

	sa.sa_handler = SIGINT_SIGTERM_handle;
	sa.sa_flags=SA_RESTART;
	ec((sigaction(SIGTERM,&sa,NULL)),-1);
	
	sa.sa_handler = SIGUSR1_handle;
	ec((sigaction(SIGUSR1,&sa,NULL)),-1);

	ec((sigemptyset(&set)),-1);
	ec((pthread_sigmask(SIG_SETMASK,&set,NULL)),-1);
	return 0;
}

static void SIGINT_SIGTERM_handle (int signum){ wator_exit=1; }

static void SIGUSR1_handle(int signum){
	save=1;
}

/** La funzione prende in input un vector rappresentante gli estremi
 *   degli estremi del rettangolo da elaborare.
 *  Quindi memorizza al suo interno i pesci e gli squali cui applicare le regole.
 *  Per evitare di danneggiare le tile degli altri worker, e creare situazioni 
 * 	 di animali che si muovono più volte, la funzione si sincronizza 
 *   con gli altri worker, permettendogli di acquisire un pianeta stabile.
 *  In seguito vengono applicate in ordine le funzioni per il movimento.
 *  La comunicazione dei pesci mangiati avviene tramite la matrice fish_death,
 *   in cui, se una posizione vale 1, significa che il pesce che si trovava li
 *   è stato mangiato.
 *  Ogni movimento è effettuato in maniera atomica e in mutua esclusione.
 */ 
void update_tile(int* tile){
	/* Vettore con le posizioni degli squali */
	int *local_shark;
	
	/* Vettore con le posizioni dei pesci */
	int *local_fish;
	
	/* Dimensione del rettangolo da elaborare*/
	int tile_size = (tile[2]-tile[0])*(tile[3]-tile[1]);
	
	/* Contatore del numero degli squali */
	int local_shark_n = 0;
	
	/* Contatore del numero dei pesci */
	int local_fish_n = 0;
	
	/* Variabili di iterazione */
	int i, j, index;
	
	/* Allocazione dei vettori posizione */
	ec(local_shark=malloc(tile_size*2*sizeof(int)),NULL);
	ec(local_fish=malloc(tile_size*2*sizeof(int)),NULL);
	
	/* Lettura dei pesci e degli squali presenti */
	for(i=tile[0];i<tile[2];i++){
		for(j=tile[1];j<tile[3];j++){
			if(ecosistema->plan->w[i][j] == SHARK){
				local_shark[local_shark_n*2] = i;
				local_shark[local_shark_n*2 + 1] = j;
				local_shark_n++;
			}
			if(ecosistema->plan->w[i][j] == FISH){
				local_fish[local_fish_n*2] = i;
				local_fish[local_fish_n*2 + 1] = j;
				local_fish_n++;
			}
		}
	}
	
	/* Sincronizzazione tra worker */
	ecd(pthread_mutex_lock(&mtx),0);
	acquire_done++;
	ecd(pthread_cond_broadcast(&acquire_cnd),0);
	
	/* Attesa di fine acquisizione pianeta degli altri worker */
	while(acquire_done< min(((int)(ecosistema->plan->nrow/N)),worker_n)){
		ecd((pthread_cond_wait(&acquire_cnd,&mtx)),0);
	}
	ecd(pthread_cond_broadcast(&acquire_cnd),0);
	ecd(pthread_mutex_unlock(&mtx),0);
		
	/* Shark_rule1 */
	for(index=0;index<2*local_shark_n;index+=2){
		ecd(pthread_mutex_lock(&mtx),0);
		switch(shark_rule1(ecosistema,local_shark[index],local_shark[index+1],&local_shark[index],&local_shark[index+1])){
			case EAT :
				/* un pesce e' stato mangiato */
				ecosistema->nf -= 1;
				fish_death[local_shark[index]][local_shark[index+1]] =  1;
				break;
			case -1 : 
				ec(0,0);
				break;
			default :
				break;
		}
		ecd(pthread_mutex_unlock(&mtx),0);
	}
	
	/* Shark_rule2 */
	for(index=0;index<2*local_shark_n;index+=2){
		ecd(pthread_mutex_lock(&mtx),0);
		switch(shark_rule2(ecosistema,local_shark[index],local_shark[index+1],&local_shark[index],&local_shark[index+1])){
			case DEAD :
				/* E' morto lo squalo */
				ecosistema->ns -= 1;
				break;
			case -1 : 
				ec(0,0);
				break;
			case 0 :
				if (get_is_born()){ ecosistema->ns += 1; }
				break;
			default :
				break;
		}
		ecd(pthread_mutex_unlock(&mtx),0);
	}
	
	/* Fish_rule3 */
	for(index=0;index<2*local_fish_n;index+=2){
		ecd(pthread_mutex_lock(&mtx),0);
		if(!fish_death[local_fish[index]][local_fish[index+1]]){
			switch(fish_rule3(ecosistema,local_fish[index],local_fish[index+1],&local_fish[index],&local_fish[index+1])){
				case -1 : 
					ec(0,0);
					break;
				default :
					break;
			}
		}
		ecd(pthread_mutex_unlock(&mtx),0);
	}
	
	/* Fish_rule4 */
	for(index=0;index<2*local_fish_n;index+=2){
		ecd(pthread_mutex_lock(&mtx),0);
		if(!fish_death[local_fish[index]][local_fish[index+1]]){
			switch(fish_rule4(ecosistema,local_fish[index],local_fish[index+1],&local_fish[index],&local_fish[index+1])){
				case -1 : 
					ec(0,0);
					break;
				case 0 :
					if(get_is_born()) { ecosistema->nf += 1; }
					break;
				default :
					ec(0,0);
					break;
			}
		}
		ecd(pthread_mutex_unlock(&mtx),0);
	}
}
