/**
	Nome : Simone
	Cognome : Schirinzi
	
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

/**************VARIABILI GLOBALI**************/
/*variabile di uscita*/
volatile sig_atomic_t visual_exit=0;

/**************FUNZIONI**************/
static void SIGINT_SIGTERM_handle(int signum);

int main(int argc, char **argv){
	/* Socket file id*/
	int sfd;

	/* Dimensioni del pianeta*/
	int local_nrow, local_ncol;

	/* Dimensione della stringa rappresentante il pianeta*/
	int LOCAL_PLANET_STRING_LEN;

	/* File su cui stampare il pianeta*/
	FILE* dumpfile_file;

	/* Path del dumpfile*/
	char dumpfile_path[STRING_SIZE];
	
	/* File descriptor del canale di comunicazione */
	int fdc;
	
	/* Variabile di iterazione*/
	int index;
	
	/* Stringa del pianeta*/
	char *planet_string;
		
	/* Buffer per la ricezione dei valori numerici*/
	char temp[STRING_SIZE];
	
	/* Buffer per la comunicazione */
	char temp1024[STRING_SIZE];
	
	/* Socket descriptor*/
	struct sockaddr_un sau;
	
	/* Signal descriptor */
	sigset_t set;
	struct sigaction sa; 
	
	/* Flag per le statistiche del wator */
	int auto_flag = 0;
	int local_fish_n = 0;
	int local_shark_n = 0;
	
	/* Eliminazione di una precedente, eventuale, socket */
	if(access(SOCKNAME,F_OK)!=-1)ec((remove(SOCKNAME)),-1);

	/* Maschero tutti i segnali */
	ec(sigfillset(&set),-1);
	ec(pthread_sigmask(SIG_SETMASK,&set,NULL),-1);
	memset(&sa,0,sizeof(sa));

	/* Inizializzazione socket*/
	sau.sun_family = AF_UNIX;
	ec((strncpy(sau.sun_path,SOCKNAME,UNIX_MAX_PATH)),NULL);
	ec((sfd=socket(AF_UNIX,SOCK_STREAM,0)),-1);
	ec((bind(sfd,(struct sockaddr *)&sau,sizeof(sau))),-1);
	ec((listen(sfd,SOMAXCONN)),-1);
	ec((fdc=accept(sfd,NULL,0)), -1);
	ec(unlink(SOCKNAME),-1);
	
	/* Imposto i gestori.
	 * Ignoro SIGPIPE e imposto SIGINT_SIGTERM_handle
	 *  come gestore dei rispettivi segnali.
	 */
	sa.sa_handler = SIG_IGN;
	ec((sigaction(SIGPIPE,&sa,NULL)),-1);
	sa.sa_handler = SIGINT_SIGTERM_handle;
	sa.sa_flags=SA_RESTART;
	ec((sigaction(SIGINT,&sa,NULL)),-1);
	sa.sa_handler = SIGINT_SIGTERM_handle;
	sa.sa_flags=SA_RESTART;
	ec((sigaction(SIGTERM,&sa,NULL)),-1);
	
	/*  Ripristino i segnali. */
	ec((sigemptyset(&set)),-1);
	ec((pthread_sigmask(SIG_SETMASK,&set,NULL)),-1);
	
	/* Ricezione parametri*/
	ec((read(fdc,temp,1024)),-1);LOCAL_PLANET_STRING_LEN = (int)strtod(temp,NULL);
	ec((read(fdc,dumpfile_path,1024)),-1);
	ec((read(fdc,temp,1024)),-1);local_nrow = (int)strtod(temp,NULL);
	ec((read(fdc,temp,1024)),-1);local_ncol = (int)strtod(temp,NULL);
	ec((read(fdc,temp,1024)),-1);auto_flag = (int)strtod(temp,NULL);
	
	/* Preparazione della stringa del pianeta */
	ec(planet_string = malloc((1024+LOCAL_PLANET_STRING_LEN)*sizeof(char)),NULL);
	
	/** Il main loop è sostanzialmente in attesa su una read,
	 *   aprendo e chiudendo un eventuale file dumpfile.
	 *  Visual exit è settato alla ricezione di SIGINT, 
	 *   che arriva subito dopo la read, dando modo al processo
	 *   di terminare in modo controllato.
	 */  
	
	while(1){
		/* Eventuale apertura del dumpfile */
		if(strcmp(dumpfile_path,"stdout")!=0){ec((dumpfile_file = fopen(dumpfile_path,"w+")),NULL);}
		else {dumpfile_file=stdout;}
		
		/* Lettura della stringa del pianeta */
		if(LOCAL_PLANET_STRING_LEN<=1024){
				ec(read(fdc,(char*)planet_string,1024),-1);
		}
		else{
			for(index=0;index<(1+(int)(LOCAL_PLANET_STRING_LEN/1024));index+=1024){
				ec(read(fdc,(char*)temp1024,1024),-1);
				strncpy(*(&planet_string+index),temp1024,1024);
			}
		}
		
		local_fish_n = local_shark_n = 0;
		
		/* Stampa del pianeta */
		print_string_planet(planet_string,dumpfile_file,local_nrow,local_ncol,LOCAL_PLANET_STRING_LEN,&local_fish_n,&local_shark_n);
		
		/* Stampa statistiche */
		if(auto_flag){
			ecm(fprintf(dumpfile_file,"Pesci:%d Squali:%d\n",local_fish_n,local_shark_n),0);
			fflush(stdout);
		}
		
		/* System clear*/
		if(strcmp(dumpfile_path,"stdout")==0){system("clear");}
		else{ec((fclose(dumpfile_file)),EOF);}
		
		/* Chiusura */
		if(visual_exit){
			close(fdc);
			close(sfd);
			exit(EXIT_SUCCESS);
		}
	}
}

static void SIGINT_SIGTERM_handle (int signum){	visual_exit = 1;}
