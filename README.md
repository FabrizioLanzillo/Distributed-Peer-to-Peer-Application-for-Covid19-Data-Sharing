# Distributed-Peer-to-Peer-Application-for-Covid19-Data-Sharing
Progetto Universitario per il corso di Reti Informatiche (Ingegneria Informatica, Unipi A.A. 2020/2021)

Il progetto è sviluppato in linguaggio C per soli sistemi Linux. Per la compilazione del programma è consigliata l'esecuzione del comando make.

Il progetto riguarda un’applicazione distribuita peer-to-peer che implementi un sistema per la condivisione di dati costantemente aggiornati sulla pandemia di COVID-19.

Si considera uno scenario in cui vari peer cooperano per elaborare statistiche aggiornate sulla pandemia. Gli utenti dei peer inseriscono costantemente dati sui nuovi casi, sui tamponi effettuati, sugli esiti di questi tamponi e così via. 
Ogni peer è responsabile di raccogliere nuovi dati e di elaborarli, condividendo poi con gli altri peer sia i dati che le elaborazioni effettuate. L’architettura peer-to-peer oltre alla presenza dei vari peers, richiede anche un server centrale, detto discovery server (DS). Il DS si occupa di registrare i peer e mantenere un registro dei neighbor di ogni peer.
