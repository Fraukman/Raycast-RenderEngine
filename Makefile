build:
	gcc -std=c99 ./rayCasting-c/*.c -lSDL2 -o raycast;
run:
	./raycast;
clean:
	rm raycast;
