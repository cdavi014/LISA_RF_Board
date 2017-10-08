CC=g++
CFLAGS=-I./include
SRC=./src/
BINDIR=./bin/

lisa:
	$(CC) -O2 -Wall $(SRC)lisa_ll_gpio.cpp $(SRC)jetson_gpio.c $(SRC)lisa_lib.c -o $(BINDIR)LISA_GPIO $(CFLAGS)

clean:
	rm -f $(BINDIR)LISA_GPIO
