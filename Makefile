CC = g++
FLAGS = -lsimlib -lm

all: tlaciaren

tlaciaren: main.cc
	$(CC) -o $@ main.cc $(FLAGS)
