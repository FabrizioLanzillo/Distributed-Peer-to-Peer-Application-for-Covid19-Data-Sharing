all: ds peer files

ds: info_covid_ds.o info_covid_utility.o
	gcc -Wall info_covid_ds.o info_covid_utility.o -o ds

info_covid_ds.o: costanti.h info_covid.h info_covid_ds.c
	gcc -c -Wall info_covid_ds.c

peer: info_covid_peer.o info_covid_utility.o
	gcc -Wall info_covid_peer.o info_covid_utility.o -o peer

info_covid_peer.o: costanti.h info_covid.h info_covid_peer.c
	gcc -c -Wall info_covid_peer.c

info_covid_utility.o: costanti.h info_covid.h info_covid_utility.c
	gcc -c -Wall info_covid_utility.c

files: files/ 

files/:
	mkdir files/

clean:
	rm *.o ds peer 
	rm -r files
