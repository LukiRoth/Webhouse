Template: main.o Webhouse.o handshake.o
	gcc -o Template main.o Webhouse.o handshake.o base64.o sha1.o -lbcm2835 -lpthread -ljansson -lm

main.o: main.c Webhouse.h handshake.h
	gcc -c main.c
	
Webhouse.o: Webhouse.c Webhouse.h
	gcc -c Webhouse.c

handshake.o: handshake.c handshake.h base64.h sha1.h
	gcc -c handshake.c
	
base64.o: base64.c base64.h
	gcc -c base64.c
	
sha1.o: sha1.c sha1.h
	gcc -c sha1.c
	
clean:
	rm Template main.o Webhouse.o handshake.o base64.o sha1.o
	
