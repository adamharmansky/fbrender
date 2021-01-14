all: render.c a.c a.h
	gcc a.c render.c -lm -lpthread -o game
clean:
	rm game
