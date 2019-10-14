ALL: init

init : init.c
	~/opt/usr/local/bin/mpicc -fopenmp init.c node.c base.c aes.c -o wes

run:
	~/opt/usr/local/bin/mpirun -np 21 wes

clean :
	/bin/rm -f init *.o
