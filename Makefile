ALL: init

init : init.c
	~/opt/usr/local/bin/mpicc -fopenmp init.c node.c base.c AES/aes.c -o program

run:
	~/opt/usr/local/bin/mpirun -np 21 program

clean :
	/bin/rm -f init *.o
