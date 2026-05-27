a.out: main.o prog1.o slp.o util.o
	gcc-15 -g main.o prog1.o slp.o util.o

main.o: main.c slp.h util.h
	gcc-15 -g -c main.c

prog1.o: prog1.c slp.h util.h
	gcc-15 -g -c prog1.c

slp.o: slp.c slp.h util.h
	gcc-15 -g -c slp.c

util.o: util.c util.h
	gcc-15 -g -c util.c

clean:
	rm -f a.out util.o prog1.o slp.o main.o
