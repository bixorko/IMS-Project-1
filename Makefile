CC = g++
FLAGS = -lsimlib -lm

all: tlaciaren

tlaciaren: main.cc
	$(CC) -o $@ main.cc $(FLAGS)

run:
	@echo -e "Output of each run is printed into *.out file!"
	./tlaciaren --validityCheck1
	./tlaciaren --validityCheck2
	./tlaciaren --validityCheck3
	./tlaciaren --simulation1
	./tlaciaren --simulation2
	./tlaciaren --simulation3
	./tlaciaren --simulation4
	./tlaciaren --simulation5
	./tlaciaren --economyCheck1
	./tlaciaren --economyCheck2

clean:
	rm -rf tlaciaren *.out
	