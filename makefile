c: main.cpp
	g++ main.cpp -o main -O3 -lpthread

r:
	./main

cr:
	g++ main.cpp -o main -O3 -lpthread && ./main && eog maze_img.ppm