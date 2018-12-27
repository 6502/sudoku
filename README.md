# sudoku
Sudoku solver from picture

This is a self-contained C++ program that tries to find a sudoku puzzle in an image and to solve it.
It doesn't depend on any external library... the only external command needed is `convert` (from
[imagemagick](https://www.imagemagick.org)) if you want to input/output images beyond the natively
supported `PGM` and `PPM` formats.

The code implements

- Image blurring using a recursive filter
- Local binarization
- Blob detection
- Corner detection
- Camera matrix computation (in 20 lines using random walking (!))
- Bilinear filtering
- Line drawing
- Image mapping
- Distance transform
- Chamfer distance computation
- Backtracking sudoku solver

running the program with `--help` provides the list of options

![example output](test-images/out7.jpg)
