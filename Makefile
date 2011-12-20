main: main.o mandelbrot.o
	gcc -o main main.o mandelbrot.o -L/usr/local/cuda/lib64 -lcudart `pkg-config --libs --cflags opencv`
mandelbrot.o: mandelbrot.cu
	nvcc -c mandelbrot.cu -arch=sm_20
main.o: main.c mandelbrot.h
	gcc -c main.c -std=gnu99 `pkg-config --cflags opencv`

clean:
	rm -f *.o main
