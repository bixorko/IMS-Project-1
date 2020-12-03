CC = g++
FLAGS = -lsimlib -lm

all: tlaciaren

tlaciaren: main.cc
	$(CC) -o $@ main.cc $(FLAGS)

run:
	./tlaciaren --validityCheck1
	./tlaciaren --validityCheck2
	./tlaciaren --validityCheck3

clean:
	rm -rf tlaciaren *.out
	