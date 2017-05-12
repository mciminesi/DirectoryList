CC=cc
CFLAGS=-Wall -I. -std=c11
SOURCE=myls.c
OBJ=myls.o

all: myls

${OBJ}:	${SOURCE}
	${CC} -c ${SOURCE}

myls: myls.o
	${CC} ${CFLAGS} myls.o -o myls

clean:
	rm ${OBJ} myls
