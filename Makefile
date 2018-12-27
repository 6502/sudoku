CC = g++ -Wall -O3

ALL: sudoku

sudoku:	sudoku.cpp argv.h images.h random.h
	$(CC) sudoku.cpp -o sudoku

clean:
	rm sudoku
