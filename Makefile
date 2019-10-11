ALL: main

main : main.c
	~/opt/usr/local/bin/mpicc -fopenmp main.c -o main

run:
	~/opt/usr/local/bin/mpirun -np 21 main

clean :
	/bin/rm -f main *.o
