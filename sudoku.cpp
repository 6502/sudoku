#include <stdio.h>
#include <math.h>
#include "images.h"
#include "random.h"
#include "argv.h"

template<typename T>
double bili(Image<T>& img, double x, double y) {
    int ix = std::max(0, std::min(img.w-2, int(x-0.5))),
        iy = std::max(0, std::min(img.h-2, int(y-0.5)));
    double fx = x-0.5 - ix, fy = y-0.5 - iy;
    return ((img(ix, iy)*(1-fx) + img(ix+1, iy)*fx)*(1-fy) +
            (img(ix, iy+1)*(1-fx) + img(ix+1, iy+1)*fx)*fy);
}

void binarize(Image<unsigned char>& img,
              double kblur, double threshold) {
    int h = img.h, w = img.w;
    std::vector<double> base(img.data.begin(), img.data.end());
    auto blur = [&](double *p, int step, int n, double kblur) {
                    for (int i=1; i<n; i++) {
                        p[i*step] = p[(i-1)*step]*kblur + p[i*step]*(1-kblur);
                    }
                    for (int i=n-2; i>=0; i--) {
                        p[i*step] = p[(i+1)*step]*kblur + p[i*step]*(1-kblur);
                    }
                };
    for (int y=0; y<h; y++) {
        blur(&base[y*w], 1, w, kblur);
    }
    for (int x=0; x<w; x++) {
        blur(&base[x], w, h, kblur);
    }
    for (int i=0; i<w*h; i++) img[i] = (img[i] > base[i]*threshold ? 255 : 0);
}

struct P { double x, y; };

struct Blob {
    std::vector<P> pts;
    int x0, y0, x1, y1;
};

void dt(Image<unsigned char>& img) {
    for (int y=1; y<img.h; y++) {
        for (int x=1; x<img.w-1; x++) {
            img(x, y, std::min(int(img(x, y)),
                               std::min(std::min(img(x-1, y), img(x, y-1))+30,
                                        std::min(img(x-1, y-1), img(x+1, y-1))+42)));
        }
    }
    for (int y=img.h-2; y>=0; y--) {
        for (int x=img.w-2; x>0; x--) {
            img(x, y, std::min(int(img(x, y)),
                               std::min(std::min(img(x+1, y), img(x, y+1))+30,
                                        std::min(img(x+1, y+1), img(x-1, y+1))+42)));
        }
    }
}

Blob blob(Image<unsigned char>& img, int x, int y) {
    int w = img.w, h = img.h;
    Blob res{{P{x+0.5, y+0.5}},x,y,x+1,y+1};
    int c1 = img(x, y);
    int c2 = (c1 + 1) & 255;
    img(x, y, c2);
    std::vector<P> active(res.pts);
    while (active.size()) {
        std::vector<P> na;
        for (auto& p : active) {
            int x = p.x, y = p.y,
                x0 = std::max(0, x-1), x1 = std::min(w-1, x+1),
                y0 = std::max(0, y-1), y1 = std::min(h-1, y+1);
            for (int yy=y0; yy<=y1; yy++) {
                for (int xx=x0; xx<=x1; xx++) {
                    if (img(xx, yy) == c1) {
                        res.x0 = std::min(res.x0, xx); res.x1 = std::max(res.x1, xx+1);
                        res.y0 = std::min(res.y0, yy); res.y1 = std::max(res.y1, yy+1);
                        img(xx, yy, c2);
                        res.pts.push_back(P{xx+0.5, yy+0.5});
                        na.push_back(P{xx+0.5, yy+0.5});
                    }
                }
            }
        }
        active = na;
    }
    return res;
}

int main(int argc, const char *argv[]) {
    PARM(std::string, src_name, "Source filename", "input.pgm");
    PARM(std::string, digits_name, "Digits reference filename", "digits.pgm");
    PARM(std::string, output_name, "Output filename", "out.ppm");
    PARM(std::string, debug_name, "Debug output filename", "");
    PARM(std::string, binarized_name, "Binarized rectified filename", "");
    PARM(std::string, binarized_dt_name, "DT-transformed rectified filename", "");
    PARM(std::string, digits_dt_name, "DT-transformed digits filename", "");
    PARM(double, kblur, "Blur constant", "0.95");
    PARM(double, threshold, "Binarization threshold", "0.8");
    PARM(int, sz, "Rectified cell size", "100");
    PARM(int, maxerr, "Maximum error threshold", "50");
    PARM(int, changes, "Maximum number of changes to compute", "81");

    parse_argv("sudoku", argc, argv);

    auto src = loadImage<unsigned char>(src_name);

    auto org = src;
    int w = src.w, h = src.h;

    binarize(src, kblur, threshold);

    std::vector<P> area;
    int best = -1;
    for (int y=0; y<h; y++) {
        for (int x=0; x<w; x++) {
            if (src(x, y) == 0) {
                auto res = blob(src, x, y);
                int a = (res.x1 - res.x0)*(res.y1 - res.y0);
                if (a > best) {
                    best = a;
                    area = res.pts;
                }
            }
        }
    }
    for (auto& p : area) {
        src[int(p.y)*w + int(p.x)] = 0xFE;
    }
    for (int y=0; y<h; y++) {
        for (int x=0; x<w && src(x, y) != 0xFE; x++) src(x, y, 0);
        for (int x=w-1; x>=0 && src(x, y) != 0xFE; x--) src(x, y, 0);
    }
    for (int x=0; x<w; x++) {
        for (int y=0; y<h && src(x, y) != 0xFE; y++) src(x, y, 0);
        for (int y=h-1; y>=0 && src(x, y) != 0xFE; y--) src(x, y, 0);
    }

    P center;
    {
        double cx = 0, cy = 0, sz = 0;
        for (int y=0; y<h; y++) {
            for (int x=0; x<w; x++) {
                if (src[y*w+x]) {
                    cx += x+0.5; cy += y+0.5; sz += 1;
                }
            }
        }
        center = P{cx/sz, cy/sz};
    }

    auto farthest = [&](P a) -> P {
                        double bd = 0;
                        P res(a);
                        for (auto& p : area) {
                            double d2 = (p.x - a.x)*(p.x - a.x) + (p.y - a.y)*(p.y - a.y);
                            if (d2 > bd) {
                                bd = d2; res = p;
                            }
                        }
                        return res;
                 };

    auto farthest2 = [&](P a, P b) -> P {
                         double dx = b.x - a.x, dy = b.y - a.y, d2 = dx*dx + dy*dy;
                         double bd = 0;
                         P res(a);
                         for (auto& p : area) {
                             double t = std::max(0., std::min(1., ((p.x - a.x)*dx + (p.y - a.y)*dy)/d2));
                             double xx = a.x + t*dx, yy = a.y + t*dy;
                             double d = (xx - p.x)*(xx - p.x) + (yy - p.y)*(yy - p.y);
                             if (d > bd) {
                                 bd = d;
                                 res = p;
                             }
                         }
                         return res;
                     };

    P A = farthest(center), D = farthest(A), B = farthest2(A, D), C = farthest(B);
    if (A.y > D.y) std::swap(A, D);
    if (B.y > C.y) std::swap(B, C);
    if (A.x > B.x) { std::swap(A, B); std::swap(C, D); }

    std::vector<double> mat{ (B.x-A.x),   (B.y-A.y),   0.,
                             (C.x-A.x),   (C.y-A.y),   0.,
                             A.x,         A.y,         1., };
    auto project = [&](double x, double y) -> P {
                       double iz = mat[2]*x + mat[5]*y + mat[8];
                       double ix = mat[0]*x + mat[3]*y + mat[6];
                       double iy = mat[1]*x + mat[4]*y + mat[7];
                       return P{ix/(iz + !iz), iy/(iz + !iz)};
                   };

    auto project_err = [&]() -> double {
                           auto a = project(0, 0);
                           auto b = project(1, 0);
                           auto c = project(0, 1);
                           auto d = project(1, 1);
                           return ((A.x-a.x)*(A.x-a.x) + (A.y-a.y)*(A.y-a.y) +
                                   (B.x-b.x)*(B.x-b.x) + (B.y-b.y)*(B.y-b.y) +
                                   (C.x-c.x)*(C.x-c.x) + (C.y-c.y)*(C.y-c.y) +
                                   (D.x-d.x)*(D.x-d.x) + (D.y-d.y)*(D.y-d.y));
                       };
    double be = project_err();
    for (int count=0; count<1000000; count++) {
        auto old = mat;
        for (int i=3; i>=0; i--) {
            mat[rnd(9)] += (rnd()-0.5)*rnd()*rnd();
        }
        mat[8] = 1.;
        double e = project_err();
        if (e < be) {
            be = e;
        } else {
            mat = old;
        }
    }

    Image<unsigned char> rectified(sz*11, sz*11);
    for (int y=-sz; y<sz*10; y++) {
        double s = (y+0.5)/(sz*9);
        for (int x=-sz; x<sz*10; x++) {
            double t = (x+0.5)/(sz*9);
            P p = project(t, s);
            rectified(x+sz, y+sz, std::max(0, std::min(255, int(bili(org, p.x, p.y)))));
        }
    }
    auto binr(rectified);
    binarize(binr, kblur, threshold);
    if (binarized_name != "") saveImage(binr, binarized_name);

    auto digits_image = loadImage<unsigned char>(digits_name);
    auto org_digits_image(digits_image);
    binarize(digits_image, kblur, threshold);

    std::vector<Blob> digits;
    for (int y=0; y<digits_image.h; y++) {
        for (int x=0; x<digits_image.w; x++) {
            if (digits_image(x, y) == 0) {
                digits.push_back(blob(digits_image, x, y));
            }
        }
    }
    if (digits.size() != 9) {
        fprintf(stderr, "Digits sample doesn't have 9 digits. Aborting.\n");
        exit(1);
    }

    dt(digits_image);
    dt(binr);
    if (digits_dt_name != "") saveImage(digits_image, digits_dt_name);
    if (binarized_dt_name != "") saveImage(binr, binarized_dt_name);

    Image<unsigned> debug(rectified.w, rectified.h);

    std::vector<int> data(9*9);
    Image<unsigned> out(org.w, org.h);
    for (int i=0; i<org.w*org.h; i++) out[i] = org[i]*3/4*0x010101;

    auto show = [&](int ii, int jj, int d, unsigned color) {
                    std::vector<int> aa(org.w*org.h*2);
                    Blob& dd = digits[d];
                    for (int y=0; y<sz; y++) {
                        for (int x=0; x<sz; x++) {
                            double j = double(x) / sz / 9;
                            double i = double(y) / sz / 9;
                            P p = project(jj/9.+0.5/9+(j-0.5/9)*0.5, ii/9.+0.5/9+(i-0.5/9)*0.6);
                            int ix = p.x, iy = p.y;
                            if (ix >=0 && ix < org.w && iy >= 0 && iy < org.h) {
                                double s = (y+0.5)/sz, t = (x+0.5)/sz;
                                double xx = dd.x0*(1-t)+dd.x1*t, yy = dd.y0*(1-s)+dd.y1*s;
                                int a = (iy*org.w + ix)*2;
                                aa[a] += bili(org_digits_image, xx, yy);
                                aa[a+1] += 1;
                            }
                        }
                    }
                    for (int i=0; i<org.w*org.h; i++) {
                        if (aa[i*2+1]) {
                            int r = (out[i] & (color*255))/color;
                            int ov = 255 - aa[i*2] / aa[i*2+1];
                            out[i] = (out[i] & (color*255 ^ 0xFFFFFF)) + std::min(255, r + ov)*color;
                        }
                    }
                };

    for (int y=0; y<rectified.h; y++) {
        for (int x=0; x<rectified.w; x++) {
            if (binr(x, y) == 0) {
                auto res = blob(binr, x, y);
                int bw = res.x1 - res.x0, bh = res.y1 - res.y0;
                if (bh > sz/3 && bh < sz && bw > sz/8 && bw < sz) {
                    int be = 0, bd = -1;
                    int x0 = res.x0-sz/8, x1 = res.x1 + sz/8,
                        y0 = res.y0-sz/8, y1 = res.y1 + sz/8;
                    for (int d=0; d<9; d++) {
                        for (int tx=-1; tx<=1; tx++) {
                            for (int ty=-1; ty<=1; ty++) {
                                Blob& dd = digits[d];
                                double sf = double(dd.y1 - dd.y0)/(res.y1 - res.y0);
                                double rx = (res.x0 + res.x1)*0.5 + 0.5;
                                double ry = (res.y0 + res.y1)*0.5 + 0.5;
                                double dx = (dd.x0 + dd.x1)*0.5 + 0.5;
                                double dy = (dd.y0 + dd.y1)*0.5 + 0.5;
                                double e = 0, n = 0;
                                for (auto& p : res.pts) {
                                    int x = p.x+tx, y = p.y+ty;
                                    e += digits_image((x-rx)*sf+dx, (y-ry)*sf+dy);
                                    n += 1;
                                }
                                for (auto& p : dd.pts) {
                                    int x = p.x, y = p.y;
                                    e += binr((x-dx)/sf+rx+tx, (y-dy)/sf+ry+ty);
                                    n += 1;
                                }
                                e /= n;
                                if (bd == -1 || e < be) {
                                    be = e; bd = d;
                                }
                            }
                        }
                    }
                    if (be < maxerr) {
                        Blob& dd = digits[bd];
                        double sf = double(dd.y1 - dd.y0)/(res.y1 - res.y0);
                        double rx = (res.x0 + res.x1)*0.5 + 0.5;
                        double ry = (res.y0 + res.y1)*0.5 + 0.5;
                        double dx = (dd.x0 + dd.x1)*0.5 + 0.5;
                        double dy = (dd.y0 + dd.y1)*0.5 + 0.5;
                        for (int y=y0; y<y1; y++) {
                            for (int x=x0; x<x1; x++) {
                                int ref = digits_image((x-rx)*sf+dx, (y-ry)*sf+dy);
                                int v = (x<res.x0 || x>=res.x1 || y<res.y0 || y>=res.y1) ? 255 : binr(x, y);
                                debug(x, y, v*0x010000 + (255-ref)*0x000100);
                            }
                        }

                        int i = int(ry / sz) - 1, j = int(rx / sz) - 1;
                        if (i >= 0 && i < 9 && j >= 0 && j < 9) {
                            data[i*9 + j] = bd+1;
                            show(i, j, bd, 0x010000);
                        }
                    }
                }
            }
        }
    }
    auto line = [&](P a, P b, unsigned color) {
                    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
                    int ix = x0 < x1 ? 1 : -1, dx = abs(x1 - x0);
                    int iy = y0 < y1 ? 1 : -1, dy = abs(y1 - y0);
                    int m = std::max(dx, dy), cx = m/2, cy = cx;
                    for (int i=0; i<=m; i++) {
                        out(x0, y0, color);
                        if ((cx += dx) >= m) { cx -= m; x0 += ix; }
                        if ((cy += dy) >= m) { cy -= m; y0 += iy; }
                    }
                };
    for (int i=0; i<=9; i++) {
        line(project(0, i/9.), project(1, i/9.), 0xFF00FF);
        line(project(i/9., 0), project(i/9., 1), 0xFF00FF);
    }

    if (debug_name != "") saveImage(debug, debug_name);
    for (int i=0; i<9; i++) {
        for (int j=0; j<9; j++) {
            if (data[i*9+j]) {
                printf(" %i", data[i*9+j]);
            } else {
                printf(" .");
            }
        }
        printf("\n");
    }
    printf("\n");

    auto avail = [&](int i, int j)->unsigned {
                     unsigned taken = 0;
                     int block = (i/3*3)*9+(j/3*3);
                     for (int k=0; k<9; k++) {
                         taken |= 1 << data[i*9+k];
                         taken |= 1 << data[k*9+j];
                         taken |= 1 << data[block+k/3*9+k%3];
                     }
                     return 511 - (taken >> 1);
                 };

    for (;;) {
        bool updated = false;
        for (int i=0; i<9; i++) {
            for (int j=0; j<9; j++) {
                if (data[i*9+j] == 0 && changes>0) {
                    unsigned a = avail(i, j);
                    if ((a & (a-1)) == 0) {
                        changes--;
                        int d = 1;
                        while (a>1) {
                            d++;
                            a = a>>1;
                        }
                        data[i*9+j] = d;
                        show(i, j, d-1, 0x000100);
                        updated = true;
                    }
                }
            }
        }
        if (!updated) break;
        break;
    }

    saveImage(out, output_name);

    return 0;
}
