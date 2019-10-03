ALL: main

mandelbrot_parallel_naive : main.c
	mpicc main.c -o main

run:
	mpirun -np 21 main

clean :
	/bin/rm -f main *.o
