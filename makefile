all: productor broker consumidor

productor: productor.o
	gcc -Wall -Wshadow -Wextra $^ -o $@

broker: broker.c hashmap.c hashmap.h
	gcc -g -Wall -Wshadow -Wextra -I./ $^ -o $@

consumidor: consumidor.c
	gcc -Wall -Wshadow -Wextra $^ -o $@

.c.o:
	gcc -Wall -Wshadow -Wextra -c $*.c 

clean:
	rm -rf *.o broker consumidor productor 
