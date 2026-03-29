main: main.c parse_file.c fftw_helper.c modify_data.c
	gcc -Wall -o main main.c parse_file.c fftw_helper.c modify_data.c -lm -I/usr/include/mkl/fftw -lmkl_rt