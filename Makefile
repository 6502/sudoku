#CC = g++ -Wall -O0 -g -fsanitize=address -D_GLIBCXX_DEBUG
CC = g++ -Wall -O3

ALL: sudoku

sudoku:	sudoku.cpp argv.h images.h random.h
	$(CC) sudoku.cpp -o sudoku

clean:
	rm -rf sudoku test-result
