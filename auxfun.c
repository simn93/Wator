/**
	Nome : Simone
	Cognome : Schirinzi
	Matricola :  490209
	Programma : Wator

	Si dichiara che il contenuto di questo file e' in ogni sua parte opera originale dell'autore.

	Simone Schirinzi
*/

#include "auxfun.h"
#include "wator.h"
#include <stdlib.h>
#include <unistd.h>

char *planet_to_string(int PSL, wator_t *ecosistema){
	char *string;
	int i, j, k = 0;

	ec( string = malloc((1024+PSL)*sizeof(char)), NULL);

	for	(i=0;i<ecosistema->plan->nrow;i++){
		for(j=0;j<ecosistema->plan->ncol;j++){
			string[k]=cell_to_char(ecosistema->plan->w[i][j]);
			k++;
		}
		string[k]='.';
		k++;
	}
	string[k]='\0';
	return string;
}

int max(int a,int b){
	if(a==b){return a;}
	if(a>b){return a;}
	else{return b;}
}

int min(int a, int b){
	if(a==b){return a;}
	else{
		if(a<b){return a;}
		else {return b;}
	}
}

void insert(list_guest *taillist, int fx, int fy, int tx, int ty){
	list temp;
	ec((temp = malloc(sizeof(elem))),NULL);
	temp->from.x = fx;
	temp->from.y = fy;
	temp->to.x = tx;
	temp->to.y = ty; 
	temp->next=NULL;

	if(taillist->head == NULL){ /*head e tail null*/
		taillist->head=temp;
		taillist->tail=temp;
	}
	else{
		taillist->tail->next=temp;
		taillist->tail=temp;
	}
}

int* extract(list_guest *taillist){
	int* retvalue;
	list temp;
	
	ec(retvalue=malloc(4*sizeof(int)),NULL);	
	
	/*la lista non deve essere vuota*/
	temp = taillist->head;
	taillist->head = taillist->head->next;

	retvalue[0] = temp->from.x;
	retvalue[1] = temp->from.y;
	retvalue[2] = temp->to.x;
	retvalue[3] = temp->to.y;
	free(temp);
	return retvalue;
}

void print_string_planet(char* planet_string, FILE* path, int nrow, int ncol, int LOCAL_PLANET_STRING_LEN,int *fish_n, int *shark_n){
	int i;
	ecm(fprintf(path,"%d\n",nrow),0);
	ecm(fprintf(path,"%d\n",ncol),0);
	for(i=0;i<LOCAL_PLANET_STRING_LEN-1;i++){
		if(planet_string[i]=='.'){ecm(fprintf(path,"\n"),0);}
		else{
			ecm(fprintf(path,"%c",planet_string[i]),0);
			if(planet_string[i+1]!='.')ecm(fprintf(path," "),0);
		}
		if(planet_string[i]=='F') *fish_n += 1;
		if(planet_string[i]=='S') *shark_n += 1;
	}
}

void crea_wid_file(int w_id){
	char mypath[1001];
	FILE *myfile;
	ecm(sprintf(mypath,"./wator_worker_%d",w_id),0);
	ec(myfile = fopen(mypath,"w+"),NULL);
	ec(fclose(myfile),EOF);
}
