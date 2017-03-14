CFLAGS=-std=c99 -g -fopenmp -Wall
LFLAGS=-lm
#-Wall

objects = main.o core.o blake2b.o utils.o hash_functions.o compression_functions.o usage.o

main: $(objects)
	gcc -fopenmp $(objects) $(LFLAGS) -o argon2

main.o: main.c
	gcc -c $(CFLAGS) main.c 

core.o: core.c
	gcc -c $(CFLAGS) core.c

compression_functions.o: compression_functions.c
	gcc -c $(CFLAGS) compression_functions.c

hash_functions.o: hash_functions.c
	gcc -c $(CFLAGS) hash_functions.c

blake2b.o: blake2b.c
	gcc -c $(CFLAGS) blake2b.c


utils.o: utils.c
	gcc -c $(CFLAGS) utils.c

usage.o: usage.c
	gcc -c $(CFLAGS) usage.c

clean: 
	rm -f $(objects)