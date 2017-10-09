CC=g++
CFLAGS=-I./include
SRC=./src/
BINDIR=./bin/

lisa:
	$(CC) -O2 -Wall $(SRC)lisa_ll_gpio.c $(SRC)jetson_gpio.c $(SRC)lisa_lib.c -o $(BINDIR)LISA_GPIO $(CFLAGS)

alg_test:
	$(CC) -O2 -Wall $(SRC)alg_test.c $(SRC)jetson_gpio.c $(SRC)lisa_lib.c -o $(BINDIR)LISA_TEST $(CFLAGS)

clean:
	rm -f $(BINDIR)LISA_GPIO $(BINDIR)LISA_TEST
