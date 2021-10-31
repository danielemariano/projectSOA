# projectSOA
La specifica del progetto richiede l'implementazione di un sottosistema del kernel Linux che permette lo scambio di messaggi tra threads. Il servizio espone 32 livelli (che abbiamo definito tag) di default guidati dalle seguenti chiamate di sistema:

int tag_get(int key, int command, int permission)

    questa chiamata di sistema istanzia o apre il servizio TAG associato alla chiave a seconda del valore del comando. Il valore IPC_PRIVATE deve essere utilizzato per la chiave per creare un'istanza del servizio in modo tale che non possa essere riaperto da questa stessa chiamata di sistema. Il valore di ritorno dovrebbe indicare il descrittore del servizio TAG (-1 è l'errore di ritorno) per gestire le operazioni effettive sul servizio TAG. Inoltre, il valore dell'autorizzazione dovrebbe indicare se il servizio TAG viene creato per le operazioni dai thread che eseguono un programma per conto dello stesso utente che installa il servizio o da qualsiasi thread.

int tag_send(int tag, int level, char* buffer, size_t size)

    questo servizio consegna il messaggio attualmente presente nel buffer all'indirizzo con tag come descrittore. Tutti i thread che sono attualmente in attesa di tale messaggio sul corrispondente livello dovrebbero essere ripresi per l'esecuzione e dovrebbero ricevere il messaggio (sono comunque consentiti messaggi di lunghezza zero). Il servizio non mantiene il registro dei messaggi che sono stati inviati e se nessun destinatario è in attesa del messaggio questo viene semplicemente scartato.

int tag_receive(int tag, int level, char* buffer, size_t size)

    questo servizio permette ad un thread di chiamare l'operazione di bloccante di ricezione del messaggio da prelevare dal corrispondente tag descriptor ad un dato livello. L'operazione di ricezione può fallire a causa della consegna di un segnale Posix al thread mentre il thread è in attesa del messaggio.

int tag_ctl(int tag, int command)

    questa chiamata di sistema permette al chiamante di controllare il servizio TAG, definito dal rispettivo descrittore tag, secondo un comando che può essere AWAKE_ALL (per svegliare tutti i thread in attesa di messaggi, indipendentemente dal livello), oppure REMOVE (per rimuovere il servizio TAG dal sistema). Non è possibile rimuovere un servizio TAG se sono presenti thread in attesa di messaggi.

Di default il sottosistema implementato può consentire la gestione di almeno 256 servizi TAG e la dimensione massima consentita per ogni messaggio è di 4KB.

Inoltre è stato sviluppato un device driver che permette di verificare lo stato corrente, ovvero le chiavi correnti del servizio TAG e il numero di thread attualmente in attesa di messaggi. Ogni riga del file del device corrispondente deve quindi essere strutturata come "TAG-chiave TAG creatore TAG-livello di attesa-thread".
