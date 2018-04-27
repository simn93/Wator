Per eseguire il processo è necessario avere (oltre ai file contenuti nell'archivio):
	Un file pianeta con la configurazione iniziale
	Un file wator.conf con i parametri di nascita e morte

Per compilare è sufficiente scrivere make compila sulla bash. 
Per eseguire bisogna scrivere ./wator  pianeta, seguito da , opzionalmente
	-n numero_worker, per settare il numero di worker (1 di default)
	-v chronon_tick, per settare ogni quanti chronon avere la visualizzazione del pianeta (2 di default)
	-f dump_file, per impostare su quale file il visualizer deve stampare il pianeta(stdout di dafult)
	-a , per stampare il numero dei pesci e degli squali

Il progetto è stato sviluppato con Debian 7.8, versione del kernel 3.2
