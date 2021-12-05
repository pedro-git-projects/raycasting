build:
	gcc -std=c99 ./src/*.c -lm -lSDL2 -o raycast;

run:
	./raycast;

clean:
	rm raycast;
