#if !defined(IMAGES_H_INCLUDED)
#define IMAGES_H_INCLUDED

/*
MIT License

Copyright (c) 2018 Andrea Griffini

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <string>
#include <stdexcept>

struct ImageError : std::runtime_error {
    ImageError(const char *what) : runtime_error(what) {}
};

template<typename T>
struct Image {
    int w, h;
    std::vector<T> data;

    Image(int w, int h) : w(w), h(h), data(w*h) {}

    T operator()(int x, int y) const {
        return x>=0 && y>=0 && x<w && y<h ? data[y*w+x] : 0;
    }

    void operator()(int x, int y, T v) {
        if (x>=0 && y>=0 && x<w && y<h) data[y*w+x] = v;
    }

    T operator[](int index) const { return data[index]; }
    T& operator[](int index) { return data[index]; }

    template<typename U>
    Image(const Image<U>& other)
        : w(other.w), h(other.h),
          data(other.data.begin(), other.data.end())
    { }
};

template<typename T>
Image<T> loadImage(const std::string& name);

template<>
Image<unsigned char> loadImage<unsigned char>(const std::string& fname) {
    struct F {
        FILE *f;
        bool convert;
        F(const std::string& fname) {
            if (fname.size() > 3 && fname.substr(fname.size()-4) == ".pgm") {
                convert = false;
                f = fopen(fname.c_str(), "rb");
            } else {
                convert = true;
                f = popen(("convert " + fname + " pgm:-").c_str(), "r");
            }
            if (!f) {
                perror("loadImage<unsigned char>");
                throw ImageError("Error opening image file");
            }
        }
        ~F() {
            if (f) (convert ? pclose : fclose)(f);
        }
        operator FILE *() { return f; };
    } f(fname);

    int w, h;
    if (fgetc(f) != 'P' || fgetc(f) != '5' || fgetc(f) != '\n') throw ImageError("Not a PGM file");
    int c; while((c = fgetc(f)) == '#') {
        while ((c = fgetc(f)) != EOF && c != '\n') ;
    }
    if (c != EOF) ungetc(c, f);
    if (fscanf(f, "%i %i 255%*c", &w, &h)!=2 || w<0 || h<0) ImageError("Not a PGM file");
    Image<unsigned char> img(w, h);
    fread(&img[0], 1, w*h, f);
    return img;
}

template<>
Image<unsigned> loadImage<unsigned>(const std::string& fname) {
    struct F {
        FILE *f;
        bool convert;
        F(const std::string& fname) {
            if (fname.size() > 4 && fname.substr(fname.size()-4) == "ppm") {
                convert = false;
                f = fopen(fname.c_str(), "rb");
            } else {
                convert = true;
                f = popen(("convert " + fname + " ppm:-").c_str(), "r");
            }
            if (!f) {
                perror("loadImage<unsigned>");
                throw ImageError("Error opening image file");
            }
        }
        ~F() {
            if (f) (convert ? pclose : fclose)(f);
        }
        operator FILE *() { return f; };
    } f(fname);
    int w, h;
    if (fgetc(f) != 'P' || fgetc(f) != '6' || fgetc(f) != '\n') throw ImageError("Not a PPM file");
    int c; while((c = fgetc(f)) == '#') {
        while ((c = fgetc(f)) != EOF && c != '\n') ;
    }
    if (c != EOF) ungetc(c, f);
    if (fscanf(f, "%i %i 255%*c", &w, &h)!=2 || w<0 || h<0) throw ImageError("Not a PPM file");
    Image<unsigned int> img(w, h);
    std::vector<unsigned char> row(w*3);
    for (int y=0; y<h; y++) {
        fread(&row[0], 1, w*3, f);
        for (int x=0; x<w; x++) {
            img[y*w+x] = row[x*3+2] + (row[x*3+1]<<8) + (row[x*3]<<16);
        }
    }
    return img;
}

template<typename T>
void saveImage(const Image<T>& img, const std::string& filename);

template<>
void saveImage<unsigned char>(const Image<unsigned char>& img, const std::string& fname) {
    struct F {
        FILE *f;
        bool convert;
        F(const std::string& fname) {
            if (fname.size() && fname.substr(fname.size()-4) == ".pgm") {
                convert = false;
                f = fopen(fname.c_str(), "wb");
            } else {
                convert = true;
                f = popen(("convert pgm:- " + fname).c_str(), "w");
            }
            if (!f) {
                perror("saveImage<unsigned char>");
                throw ImageError("Error saving image");
            }
        }
        ~F() {
            if (f) (convert ? pclose : fclose)(f);
        }
        operator FILE *() { return f; };
    } f(fname);
    fprintf(f, "P5\n%i %i 255\n", img.w, img.h);
    fwrite(&img.data[0], 1, img.w*img.h, f);
}

template<>
void saveImage<unsigned>(const Image<unsigned>& img, const std::string& fname) {
    struct F {
        FILE *f;
        bool convert;
        F(const std::string& fname) {
            if (fname.size()>4 && fname.substr(fname.size()-4) == ".ppm") {
                convert = false;
                f = fopen(fname.c_str(), "wb");
            } else {
                convert = true;
                f = popen(("convert ppm:- " + fname).c_str(), "w");
            }
            if (!f) {
                perror("saveImage<unsigned>");
                throw ImageError("Error saving image");
            }
        }
        ~F() {
            if (f) (convert ? pclose : fclose)(f);
        }
        operator FILE *() { return f; };
    } f(fname);

    fprintf(f, "P6\n%i %i 255\n", img.w, img.h);
    std::vector<unsigned char> row(img.w*3);
    for (int y=0; y<img.h; y++) {
        for (int x=0; x<img.w; x++) {
            row[x*3] = (img[y*img.w+x]>>16) & 255;
            row[x*3+1] = (img[y*img.w+x]>>8) & 255;
            row[x*3+2] = img[y*img.w+x] & 255;
        }
        fwrite(&row[0], 1, img.w*3, f);
    }
}

#endif
