CC:=gcc
EXE:=bin/simple-shell
FLAGS:=-Wall -static-libgcc -std=c11
SRC:=src/*.c

all:
	$(CC) $(SRC) -o $(EXE) $(FLAGS)

clean:
	rm -f $(EXE)
