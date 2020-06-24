ALL: init

init : init.c
	mpicc -fopenmp init.h init.c node.c base.c aes.c -o wes

run:
	mpirun -np 21 wes

clean :
	/bin/rm -f  *.o
