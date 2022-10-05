/*
  Cool Fractal Viewer
  Copyright (C) 2022 David Cole

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "build/config.h" // NOTE: add the following: TODO: use t to change fractal type (sier/mand)
#include "sierpinski.h" // TODO: make color change not cause mabelbort set recalculation
#include "icon.xpm" // NOTE: TODO: fix memory leaky
#include "load_xpm.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <future>
#include <iomanip>
#include <sstream>
#include <thread>
#include <typeinfo>
#include <vector>
typedef long double long_double;
#if (PRECISION_T == mpfr_float)
#include <boost/multiprecision/mpfr.hpp>
using namespace boost::multiprecision;
#endif
typedef PRECISION_T precision_t; // what type to use for most major calculations, the more precise the deeper you can zoom but the slower it will most likely be.
typedef long double normalized_t; // which type to use for normalized iteration output, more precise means more smoothness but less speed.
// #include <boost/math/special_functions/gamma.hpp>
unsigned int set = 0;
unsigned int color =
    2; // make color an int because i don't wanna come up with
       // names for the colorschemes and maybe it's better for performance idk
constexpr unsigned int total_colors =
    3; // how many colorschemes have actually been programmed in
constexpr unsigned int sets = 4;
constexpr unsigned int et_sets = 3; // i'll let you guess
const std::vector<std::string> set_names = {"mandelbrot", "tricorn",
                                            "burning_ship", "julia", "sierpinski"};
bool normalized = true;
unsigned long int iterations = 1024;
// unsigned long int auto_iterations = 16384; // WIP
long double d = 2;             // WIP
constexpr unsigned int radius = 4; // probably shouldn't change this
constexpr bool multisample = false;
constexpr int r = 2;
constexpr int g = 4;
constexpr int b = 8;
unsigned int threads = std::thread::hardware_concurrency();
bool automatic_iters = true;
precision_t zoom = default_zoom;
std::complex<precision_t> aspect_zoom(zoom, zoom);
precision_t x_pos = 0;
precision_t y_pos = 0;
const SDL_BlendMode blend = SDL_BLENDMODE_NONE;
SDL_Texture *rendered;
bool d_is_int;
std::complex<int> prevsize;
bool quit = false; // you are stupid if you change this to true
const double log_2 = log(2);
std::complex<precision_t> julia_c;
// std::vector<double> bench;

precision_t naive_lerp(precision_t a, precision_t b, precision_t t) {
  return a + t * (b - a);
}

char *str_to_char(std::string str) { // call delete[] on anything made with this when done with it
  char *out = new char[str.size() + 1];
  strcpy(out, str.c_str());
  return out;
}

int load_icon(SDL_Window *window) {
  SDL_Surface *image = IMG_ReadXPMFromArray(icon_xpm);

  if (!image) {
    SDL_Log("Failed to load icon");
  }

  SDL_SetWindowIcon(window, image);
  SDL_FreeSurface(image);

  return 0;
}

int in_mand_set(std::complex<long double> c) { // this one's unused, try going down two functions
  std::complex<long double> z(0, 0);
  for (unsigned int i = 0; i < iterations; i++) {
    z = std::pow(z, d) + c;
    if (std::norm(z) > 4) {
      return i;
    }
  }
  return 0;
}

int in_mand_set_orbit(std::complex<long double> c) { // this is also unused, go down one function
  std::complex<long double> z(0, 0);
  std::complex<long double> zfast(0, 0);
  unsigned int i = 0;
  while (true) {
    i++;
    long double xtemp = z.real() * z.real() - z.imag() * z.imag() +
                        c.real(); // z = std::pow(z, d) + c;
    z.imag(2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);

    xtemp = z.real() * z.real() - z.imag() * z.imag() +
            c.real(); // z = std::pow(z, d) + c;
    zfast.imag(2 * z.real() * z.imag() + c.imag());
    zfast.real(xtemp);

    xtemp = z.real() * z.real() - z.imag() * z.imag() +
            c.real(); // z = std::pow(z, d) + c;
    zfast.imag(2 * z.real() * z.imag() + c.imag());
    zfast.real(xtemp);

    if (z == zfast) {
      return i;
    }
  }
}

int in_mand_set2(std::complex<precision_t> c) {
  std::complex<precision_t> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() +
                        c.real(); // z = std::pow(z, d) + c;
    z.imag(2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);
    if (std::norm(z) > radius) {
      return iterations - i;
    }
  }
  return 0;
}

normalized_t in_mand_set_norm(std::complex<precision_t> c) {
  std::complex<precision_t> z(0, 0); // try playing around with this
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() + c.real();
    z.imag(2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);
    normalized_t modulus = (normalized_t)sqrt(z.real() * z.real() + z.imag() * z.imag());

    if (modulus > radius) {
      return iterations - i - (log(log(modulus))) / log_2;
    }
  }
  return 0;
}

normalized_t in_julia_set_norm(std::complex<precision_t> z) {
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() + julia_c.real();
    z.imag(2 * z.real() * z.imag() + julia_c.imag());
    z.real(xtemp);
    normalized_t modulus = (normalized_t)sqrt(z.real() * z.real() + z.imag() * z.imag());

    if (modulus > radius) {
      return iterations - i - (log(log(modulus))) / log_2;
    }
  }
  return 0;
}

normalized_t in_ship_set_norm(std::complex<precision_t> c) {
  std::complex<precision_t> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() + c.real();

    z.imag(abs(2 * z.real() * z.imag()) + c.imag());
    z.real(xtemp);
    normalized_t modulus = (normalized_t)sqrt(z.real() * z.real() + z.imag() * z.imag());

    if (modulus > radius) {
      return iterations - i - (log(log(modulus))) / log_2;
    }
  }
  return 0;
}

normalized_t in_tric_set_norm(std::complex<precision_t> c) {
  std::complex<precision_t> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() + c.real();

    z.imag(-2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);
    normalized_t modulus = (normalized_t)sqrt(z.real() * z.real() + z.imag() * z.imag());

    if (modulus > radius) {
      return iterations - i - (log(log(modulus))) / log_2;
    }
  }
  return 0;
}

int auto_iters() {
  long double f = (long double)sqrt(0.001 + 2 * std::min(abs((-aspect_zoom.real() + x_pos) -
                                                (aspect_zoom.real() + x_pos)),
                                            abs((-aspect_zoom.imag() + y_pos) -
                                                (aspect_zoom.imag() + y_pos)))); // NOTE: TODO: maybe too precise?

  iterations = floor(223 / f);
  return 0;
}

/*
int auto_iters2(SDL_Window *window) { // WIP
  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);

  auto_iters();
  iterations *= 2;

  int iters = 0;

  for (double i = 0; i < 1; i += 0.01) {
    long double x_point =
        std::lerp(-aspect_zoom.real() + x_pos, aspect_zoom.real() + x_pos, i);
    long double y_point =
        std::lerp(-aspect_zoom.imag() + y_pos, aspect_zoom.imag() + y_pos, i);

    iters = std::max(in_mand_set2(std::complex<long double>(x_point, y_point)),
                     iters);
  }

  for (double i = 0; i < 1; i += 0.01) {
    long double x_point = std::lerp(-aspect_zoom.real() + x_pos,
                                    aspect_zoom.real() + x_pos, 1 - i);
    long double y_point =
        std::lerp(-aspect_zoom.imag() + y_pos, aspect_zoom.imag() + y_pos, i);

    iters = std::max(in_mand_set2(std::complex<long double>(x_point, y_point)),
                     iters);
  }

  for (double i = 0; i < 1; i += 0.01) {
    long double x_point =
        std::lerp(-aspect_zoom.real() + x_pos, aspect_zoom.real() + x_pos, i);
    long double y_point =
        std::lerp(-aspect_zoom.imag() + y_pos, aspect_zoom.imag() + y_pos, 0.5);

    iters = std::max(in_mand_set2(std::complex<long double>(x_point, y_point)),
                     iters);
  }

  for (double i = 0; i < 1; i += 0.01) {
    long double x_point =
        std::lerp(-aspect_zoom.real() + x_pos, aspect_zoom.real() + x_pos, 0.5);
    long double y_point =
        std::lerp(-aspect_zoom.imag() + y_pos, aspect_zoom.imag() + y_pos, i);

    iters = std::max(in_mand_set2(std::complex<long double>(x_point, y_point)),
                     iters);
  }

  iterations = iters;

  return 0;
}
*/

std::vector<unsigned int> hue_to_rgb(double h, double v) {
  v = std::fmin(v, 1);
  unsigned int red, green, blue;
  if (h <= 60) {
    red = 255;
    green = h * 4.25;
    blue = 0;
  } else if (h <= 120) {
    red = (60 - (h - 60)) * 4.25;
    green = 255;
    blue = 0;
  } else if (h <= 180) {
    red = 0;
    green = 255;
    blue = (h - 120) * 4.25;
  } else if (h <= 240) {
    red = 0;
    green = (60 - (h - 180)) * 4.25;
    blue = 255;
  } else if (h <= 300) {
    red = (h - 240) * 4.25;
    green = 0;
    blue = 255;
  } else {
    red = 255;
    green = 0;
    blue = (60 - (h - 300)) * 4.25;
  }

  red *= v;
  green *= v;
  blue *= v;

  return std::vector<unsigned int>{red, green, blue};
}

int in_ship_set(std::complex<precision_t> c) {
  std::complex<double> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() + c.real();
    z.imag(double(abs(2 * z.real() * z.imag()) + c.imag()));
    z.real((double)xtemp);

    if (std::norm(z) > radius) {
      return iterations - i;
    }
  }
  return 0;
}

int in_tric_set(std::complex<precision_t> c) {
  std::complex<double> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    double xtemp = double(z.real() * z.real() - z.imag() * z.imag() + c.real()); // maybe change this precision
    z.imag(double(-2 * z.real() * z.imag() + c.imag()));
    z.real(xtemp);

    if (std::norm(z) > radius) {
      return iterations - i;
    }
  }
  return 0;
}

int in_julia_set(std::complex<precision_t> z) {
  unsigned int i = iterations;
  while (i) {
    i--;
    precision_t xtemp = z.real() * z.real() - z.imag() * z.imag() + julia_c.real();
    z.imag(2 * z.real() * z.imag() + julia_c.imag());
    z.real(xtemp);

    if (std::norm(z) > radius) {
      return iterations - i;
    }
  }
  return 0;
}

int screenshot(SDL_Renderer *renderer, SDL_Window *window) {
  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);

  std::filesystem::create_directory("screenshots");
  std::stringstream filenameSS;
  filenameSS << std::setprecision(std::numeric_limits<long double>::digits10 + 2) << "screenshots/" << set_names[set] << "_" << x_pos << "," << y_pos <<
             "," << zoom << "," << std::to_string(iterations) <<
             "," << std::to_string(r) << "," + std::to_string(g) << "," <<
             std::to_string(b) + "-" << std::to_string(win_w) << "x" <<
             std::to_string(win_h) << ".bmp";

  std::string filenameS = filenameSS.str();
  char *filenameC = str_to_char(filenameS);

  SDL_Surface *image = SDL_CreateRGBSurface(0, win_w, win_h, 32, 0, 0, 0, 0);
  SDL_RenderReadPixels(renderer, NULL, 0, image->pixels, image->pitch);
  SDL_SaveBMP(image, filenameC);
  SDL_FreeSurface(image);

  delete[] filenameC;
  return 0;
}

/*
int render_escape_time_boundary(SDL_Renderer *renderer,
                                SDL_Window *window) { // WIP
  //   iterations = sqrt(2 * 1/std::min(aspect_zoom.real(),
  //   aspect_zoom.imag()));

  int xc = std::fpclassify(x_pos);
  int yc = std::fpclassify(y_pos);
  int zoomc = std::fpclassify(zoom);
  if (xc == FP_NAN || xc == FP_INFINITE || yc == FP_NAN || yc == FP_INFINITE ||
      zoomc == FP_NAN || zoomc == FP_INFINITE) {
    x_pos = 0;
    y_pos = 0;
    zoom = 2;
    SDL_Log("Illegal position detected; resetting it");
  } else if (iterations == 0) {
    iterations = 256;
    SDL_Log("0 iterations detected; resetting it");
  }

  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);

  rendered = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                               SDL_TEXTUREACCESS_TARGET, win_w, win_h);
  SDL_SetRenderTarget(renderer, rendered);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  std::stringstream log;
  log << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
      << x_pos << "," << y_pos << ":" << zoom << " (" << iterations << ")";

  std::string logs = log.str();

  char *logc = str_to_char(logs);
  SDL_Log(logc);
  delete[] logc;
  SDL_SetWindowTitle(window, "Cool Fractal Viewer (Rendering)");
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();

  double ratioA = abs((-zoom + x_pos) - (zoom + x_pos)) /
                  abs((-zoom + y_pos) - (zoom + y_pos));
  double ratioB = double(win_w) / win_h;

  if (ratioB > ratioA) {
    double Xratio = ratioB / ratioA;
    aspect_zoom.imag(zoom);
    aspect_zoom.real(zoom * Xratio);
  } else {
    double Yratio = ratioA / ratioB;
    aspect_zoom.real(zoom);
    aspect_zoom.imag(zoom * Yratio);
  }

  int iters;
  //     SDL_Log(str_to_char("arribiata: " + to_string(i)));
  for (double x = 0.0; x < win_w; x += 1) {
    for (double y = 0.0; y < win_h; y += 1) {
      iters = 0;
      //        double x_point = std::lerp(-zoom + x_pos -  double (win_w) /
      //        win_h, zoom + x_pos + double (win_w) / win_h, x); double
      //        y_point = std::lerp(-zoom + y_pos - 0.5, zoom + y_pos + 0.5,
      //        y);
      long double x_point = std::lerp(-aspect_zoom.real() + x_pos,
                                      aspect_zoom.real() + x_pos, x / win_w);
      long double y_point = std::lerp(-aspect_zoom.imag() + y_pos,
                                      aspect_zoom.imag() + y_pos, y / win_h);
      if (set == "mandelbrot") {
        iters = in_mand_set2(std::complex<long double>(x_point, y_point));
      } else if (set == "tricorn") {
        iters = in_tric_set(std::complex<double>(x_point, y_point));
      } else if (set == "burning_ship") {
        iters = in_ship_set(std::complex<double>(x_point, y_point));
      }

      if (iters == 0) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawPoint(renderer, x, y);
      } else {
        SDL_SetRenderDrawColor(renderer, r * iters % 255, g * iters % 255,
                               b * iters % 255, 255);
        SDL_RenderDrawPoint(renderer, x, y);
      }
    }
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, rendered, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, rendered);
  }
  SDL_SetRenderTarget(renderer, NULL);

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::chrono::duration<double> diff = end - start;
  SDL_SetWindowTitle(window, str_to_char("Cool Fractal Viewer (" +
                                         std::to_string(diff.count()) + " s)"));
  return 0;
}

int render_escape_time_i(SDL_Renderer *renderer, SDL_Window *window) {
  //   iterations = sqrt(2 * 1/std::min(aspect_zoom.real(),
  //   aspect_zoom.imag()));

  int xc = std::fpclassify(x_pos);
  int yc = std::fpclassify(y_pos);
  int zoomc = std::fpclassify(zoom);
  if (xc == FP_NAN || xc == FP_INFINITE || yc == FP_NAN || yc == FP_INFINITE ||
      zoomc == FP_NAN || zoomc == FP_INFINITE) {
    x_pos = 0;
    y_pos = 0;
    zoom = 2;
    SDL_Log("Illegal position detected; resetting it");
  } else if (iterations == 0) {
    iterations = 256;
    SDL_Log("0 iterations detected; resetting it");
  }

  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);

  rendered = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                               SDL_TEXTUREACCESS_TARGET, win_w, win_h);
  SDL_SetRenderTarget(renderer, rendered);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  std::stringstream log;
  log << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
      << x_pos << "," << y_pos << ":" << zoom << " (" << iterations << ")";

  std::string logs = log.str();

  char *logc = str_to_char(logs);
  SDL_Log(logc);
  SDL_SetWindowTitle(window, "Cool Fractal Viewer (Rendering)");
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();

  double ratioA = abs((-zoom + x_pos) - (zoom + x_pos)) /
                  abs((-zoom + y_pos) - (zoom + y_pos));
  double ratioB = double(win_w) / win_h;

  if (ratioB > ratioA) {
    double Xratio = ratioB / ratioA;
    aspect_zoom.imag(zoom);
    aspect_zoom.real(zoom * Xratio);
  } else {
    double Yratio = ratioA / ratioB;
    aspect_zoom.real(zoom);
    aspect_zoom.imag(zoom * Yratio);
  }

  int iters;
  for (int i = 0; i < 4; i += 1) {
    int ix = 0;
    switch (i) {
    case 1:
      ix = 2;
      break;
    case 2:
      ix = 1;
      break;
    case 3:
      ix = 3;
      break;
    }
    //     SDL_Log(str_to_char("arribiata: " + to_string(i)));
    for (double x = ix; x < win_w; x += 4) {
      for (double y = 0.0; y < win_h; y += 1) {
        iters = 0;
        //        double x_point = std::lerp(-zoom + x_pos -  double (win_w) /
        //        win_h, zoom + x_pos + double (win_w) / win_h, x); double
        //        y_point = std::lerp(-zoom + y_pos - 0.5, zoom + y_pos + 0.5,
        //        y);
        long double x_point = std::lerp(-aspect_zoom.real() + x_pos,
                                        aspect_zoom.real() + x_pos, x / win_w);
        long double y_point = std::lerp(-aspect_zoom.imag() + y_pos,
                                        aspect_zoom.imag() + y_pos, y / win_h);
        if (set == "mandelbrot") {
          iters = in_mand_set2(std::complex<long double>(x_point, y_point));
        } else if (set == "tricorn") {
          iters = in_tric_set(std::complex<double>(x_point, y_point));
        } else if (set == "burning_ship") {
          iters = in_ship_set(std::complex<double>(x_point, y_point));
        }

        if (iters == 0) {
          SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
          SDL_RenderDrawPoint(renderer, x, y);
          switch (i) {
          case 0:
            SDL_RenderDrawPoint(renderer, x + 1, y);
            SDL_RenderDrawPoint(renderer, x + 2, y);
            SDL_RenderDrawPoint(renderer, x + 3, y);
            break;
          case 1:
            SDL_RenderDrawPoint(renderer, x + 1, y);
            break;
          }
        } else {
          SDL_SetRenderDrawColor(renderer, r * iters % 255, g * iters % 255,
                                 b * iters % 255, 255);
          SDL_RenderDrawPoint(renderer, x, y);
          switch (i) {
          case 0:
            SDL_RenderDrawPoint(renderer, x + 1, y);
            SDL_RenderDrawPoint(renderer, x + 2, y);
            SDL_RenderDrawPoint(renderer, x + 3, y);
            break;
          case 1:
            SDL_RenderDrawPoint(renderer, x + 1, y);
            break;
          }
        }
      }
    }
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, rendered, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, rendered);
  }
  SDL_SetRenderTarget(renderer, NULL);

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::chrono::duration<double> diff = end - start;
  SDL_SetWindowTitle(window, str_to_char("Cool Fractal Viewer (" +
                                         std::to_string(diff.count()) + " s)"));
  return 0;
}
*/

std::vector<normalized_t> compute_escape_time(unsigned int starty, unsigned int endy, // TODO: make this NOT USE FLOAT VECTIR
                                        unsigned int ix, int win_w, int win_h,
                                        unsigned int iz) {
  std::vector<normalized_t> renderedvec;
  renderedvec.reserve(std::floor((win_w+ix)/iz) * (endy-starty));

  unsigned int iters;

  // start the main render loop
  for (double x = ix; x < win_w; x += iz) {
    for (double y = starty; y < endy; y += 1) {
      iters = 0;
      precision_t x_point = naive_lerp(-aspect_zoom.real() + x_pos,
                                      aspect_zoom.real() + x_pos, x / win_w);
      precision_t y_point = naive_lerp(-aspect_zoom.imag() + y_pos,
                                      aspect_zoom.imag() + y_pos, y / win_h);
      switch (set) {
      case 0:
        iters = in_mand_set2(std::complex<precision_t>(x_point, y_point));
        break;
      case 1:
        iters = in_tric_set(std::complex<precision_t>(x_point, y_point));
        break;
      case 2:
        iters = in_ship_set(std::complex<precision_t>(x_point, y_point));
        break;
      case 3:
        iters = in_julia_set(std::complex<precision_t>(x_point, y_point));
        break;
      }

      renderedvec.push_back(iters);
    }
  }

  return renderedvec;
}

std::vector<normalized_t> compute_normalized(unsigned int starty, unsigned int endy,
                                       unsigned int ix, int win_w, int win_h,
                                       unsigned int iz) {
  std::vector<normalized_t> renderedvec;
  renderedvec.reserve(std::floor((win_w+ix)/iz) * (endy-starty));

  normalized_t iters;

  // start the main render loop
  for (double x = ix; x < win_w; x += iz) {
    for (double y = starty; y < endy; y += 1) {
      iters = 0;
      precision_t x_point = naive_lerp(-aspect_zoom.real() + x_pos,
                                      aspect_zoom.real() + x_pos, (precision_t)x / win_w);
      precision_t y_point = naive_lerp(-aspect_zoom.imag() + y_pos,
                                      aspect_zoom.imag() + y_pos, (precision_t)y / win_h);
      switch (set) {
      case 0:
        iters = in_mand_set_norm(std::complex<precision_t>(x_point, y_point));
        break;
      case 1:
        iters = in_tric_set_norm(std::complex<precision_t>(x_point, y_point));
        break;
      case 2:
        iters = in_ship_set_norm(std::complex<precision_t>(x_point, y_point));
        break;
      case 3:
        iters = in_julia_set_norm(std::complex<precision_t>(x_point, y_point));
        break; // TODO: NOTE: maybe not necessary, this break.
      }

      renderedvec.push_back(iters);
    }
  }

  return renderedvec;
}

int render(SDL_Renderer *renderer, SDL_Window *window) {
  bool render_incomplete = false;
  // if current fractal uses escape time, render it.
  if (set <= et_sets) {
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);

    rendered = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                 SDL_TEXTUREACCESS_TARGET, win_w, win_h);
    SDL_SetRenderTarget(renderer, rendered);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    int chunksize = win_h / threads;

    /*int xc = std::fpclassify(x_pos); // NOTE: custom precision no likey
    int yc = std::fpclassify(y_pos);
    int zoomc = std::fpclassify(zoom);
    if (xc == FP_NAN || xc == FP_INFINITE || yc == FP_NAN ||
        yc == FP_INFINITE || zoomc == FP_NAN || zoomc == FP_INFINITE) {
      x_pos = 0;
      y_pos = 0;
      zoom = 2;
      SDL_Log("Illegal position detected; resetting it");
    } else if (iterations == 0 and !automatic_iters) {
      iterations = 1;
      SDL_Log("0 iterations detected; resetting it");
    } else if (zoom == 0) {
      zoom = 2;
      SDL_Log("0 zoom detected; resetting it");
    }*/

    double ratioA = double(abs((-zoom + x_pos) - (zoom + x_pos)) /
                    abs((-zoom + y_pos) - (zoom + y_pos)));
    double ratioB = double(win_w) / win_h;

    if (ratioB > ratioA) {
      double Xratio = ratioB / ratioA;
      aspect_zoom.imag(zoom);
      aspect_zoom.real(zoom * Xratio);
    } else {
      double Yratio = ratioA / ratioB;
      aspect_zoom.real(zoom);
      aspect_zoom.imag(zoom * Yratio);
    }
    if (automatic_iters)
      auto_iters();

    std::stringstream log;
    log << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
        << x_pos << "," << y_pos << ":" << zoom << " (" << iterations << ")";

    std::string logs = log.str();

    char *logc = str_to_char(logs);
    SDL_Log("%s", logc);
    delete[] logc;

    SDL_SetWindowTitle(window, "Cool Fractal Viewer (Rendering)");
    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    for (unsigned int ie = 0; ie < 4; ie += 1) {
      unsigned int ix = 0;
      switch (ie) {
      case 1:
        ix = 2;
        break;
      case 2:
        ix = 1;
        break;
      case 3:
        ix = 3;
        break;
      }
      // SDL_Log("debug: STARTING RENDER ITERATION");

      std::vector<normalized_t> renderedvec;

      std::vector<std::future<std::vector<normalized_t>>> futures;

      // render on other threads
      for (unsigned int i = 0; i < threads - 1; i += 1) {
        if (normalized) {
          futures.push_back(std::async(std::launch::async, compute_normalized,
                                       chunksize * i, chunksize * (i + 1), ix,
                                       win_w, win_h, 4));
        } else {
          futures.push_back(std::async(std::launch::async, compute_escape_time,
                                       chunksize * i, chunksize * (i + 1), ix,
                                       win_w, win_h, 4));
        }
      }

      // do some rendering on the main thread
      std::vector<normalized_t> lastrender;
      lastrender.reserve(std::floor((win_w+ix)/4) * (win_h-chunksize * (threads - 1)));
      if (normalized) {
        lastrender = compute_normalized(chunksize * (threads - 1), win_h, ix,
                                        win_w, win_h, 4);
      } else {
        lastrender = compute_escape_time(chunksize * (threads - 1), win_h, ix,
                                         win_w, win_h, 4);
      }

      // wait for the threads
      renderedvec.reserve(win_w * win_h);
      for (unsigned int i = 0; i < threads - 1; i += 1) {
        std::vector<normalized_t> temprendered = futures[i].get();
        renderedvec.insert(renderedvec.end(), temprendered.begin(),
                           temprendered.end());
      }

      renderedvec.insert(renderedvec.end(), lastrender.begin(),
                         lastrender.end());

      double iters;
      unsigned int ticker = 0;
      unsigned int starty; // TODO: NOTE: TODO: consider merging threads to primary
      unsigned int endy;
      std::vector<unsigned int> rgb;

      // SDL_Log("debug: ACTUALLY RENDERING");

      for (unsigned int chunk = 0; chunk < threads; chunk += 1) {
        if (chunk < threads - 1) {
          starty = chunksize * chunk;
          endy = chunksize * (chunk + 1);
        } else {
          starty = chunksize * (threads - 1);
          endy = win_h;
        }
        for (double x = ix; x < win_w; x += 4) {
          for (double y = starty; y < endy; y += 1) {
            iters = renderedvec[ticker];
            if (iters == 0) {
              SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
              SDL_RenderDrawPoint(renderer, x, y);
              switch (ie) {
              case 0:
                SDL_RenderDrawPoint(renderer, x + 1, y);
                SDL_RenderDrawPoint(renderer, x + 2, y);
                SDL_RenderDrawPoint(renderer, x + 3, y);
                break;
              case 1:
                SDL_RenderDrawPoint(renderer, x + 1, y);
                break;
              }
            } else {
              switch (color) {
              case 0:
                rgb = hue_to_rgb(std::fmod(iters, 360), 1);
                break;
              case 1:
                rgb = hue_to_rgb((iters / iterations) * 360, 1);
                break;
              case 2:
                rgb = hue_to_rgb((iters / iterations) * 360,
                                 (10 * iters) / iterations);
                break;
              case 3:
                unsigned int grayscale = (iters / iterations) * 255;
                rgb =
                    std::vector<unsigned int>{grayscale, grayscale, grayscale};
                break;
              }
              SDL_SetRenderDrawColor(renderer, rgb[2], rgb[1], rgb[0], 255);
              SDL_RenderDrawPoint(renderer, x, y);
              switch (ie) {
              case 0:
                SDL_RenderDrawPoint(renderer, x + 1, y);
                SDL_RenderDrawPoint(renderer, x + 2, y);
                SDL_RenderDrawPoint(renderer, x + 3, y);
                break;
              case 1:
                SDL_RenderDrawPoint(renderer, x + 1, y);
                break;
              }
            }
            ticker += 1;
          }
        }
      }
      SDL_SetRenderTarget(renderer, NULL);
      SDL_RenderCopy(renderer, rendered, NULL, NULL);
      SDL_RenderPresent(renderer);
      SDL_SetRenderTarget(renderer, rendered);

      SDL_PumpEvents();
      SDL_Event events[1];
      if (SDL_PeepEvents(events, 1, SDL_PEEKEVENT, SDL_QUIT, SDL_QUIT)) {
        render_incomplete = true;
        break;
      }
      if (SDL_PeepEvents(events, 1, SDL_PEEKEVENT, SDL_KEYDOWN,
                         SDL_MOUSEWHEEL)) {
        render_incomplete = true;
        if (events[0].type == SDL_KEYDOWN) {
          switch (
              events[0]
                  .key.keysym.sym) { // maybe do something for SDLK_BACKSLASH?
          case SDLK_s:
          case SDLK_KP_ENTER:
            render_incomplete = false;
            break;
          }
        }
        if (render_incomplete) {
          break;
        }
      }

      renderedvec = std::vector<normalized_t>();
      lastrender = std::vector<normalized_t>();
      futures = std::vector<std::future<std::vector<normalized_t>>>();

    }

    SDL_SetRenderTarget(renderer, NULL);

    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    char* windowtitle = str_to_char("Cool Fractal Viewer (" +
                                   std::to_string(diff.count()) + " s)");

    SDL_SetWindowTitle(window, windowtitle);

    delete[] windowtitle;

    /*
    bench.push_back(diff.count());

    double count = 0;
    for (double x: bench) {
      count += x;
    }
    count /= bench.size();

    SDL_SetWindowTitle(window,
                       str_to_char("Cool Fractal Bencher (" +
                                   std::to_string(count) + " s)"));
    */

    if (render_incomplete) {
      SDL_SetWindowTitle(window, "Cool Fractal Viewer (RENDER INCOMPLETE)");
      return 1;
    }
  } else if (set == 4) {

    SDL_SetWindowTitle(window, "Cool Fractal Viewer (Rendering)");
    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now(); // TODO: fix biggie big: NOTE: pressing random key (like w or some shit) while rendering will make the render cease to render! check if the key is actually important first.

    rendered = render_sierpinski(renderer, window, iterations, rendered);

    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    char* windowtitle = str_to_char("Cool Fractal Viewer (" +
                                   std::to_string(diff.count()) + " s)");

    SDL_SetWindowTitle(window,
                       windowtitle);

    delete[] windowtitle;
  }
  return 0;
}

int handle_events(SDL_Event event, SDL_Renderer *renderer, SDL_Window *window) {
  switch (event.type) {
  case SDL_QUIT:
    quit = true;
    break;
  case SDL_KEYDOWN:
    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
      quit = true;
      break;
    case SDLK_s:
      screenshot(renderer, window);
      break;
    case SDLK_KP_ENTER:
      screenshot(renderer, window);
      break;
    case SDLK_DOWN: // down
      y_pos += zoom;
      render(renderer, window);
      break;
    case SDLK_KP_2: // down keypad
      y_pos += zoom;
      render(renderer, window);
      break;
    case SDLK_KP_1: // down left keypad
      y_pos += zoom;
      x_pos -= zoom;
      render(renderer, window);
      break;
    case SDLK_LEFT: // left
      x_pos -= zoom;
      render(renderer, window);
      break;
    case SDLK_KP_4: // left keypad
      x_pos -= zoom;
      render(renderer, window);
      break;
    case SDLK_KP_7: // left up keypad
      x_pos -= zoom;
      y_pos -= zoom;
      render(renderer, window);
      break;
    case SDLK_UP: // up
      y_pos -= zoom;
      render(renderer, window);
      break;
    case SDLK_KP_8: // up keypad
      y_pos -= zoom;
      render(renderer, window);
      break;
    case SDLK_KP_9: // up right keypad
      y_pos -= zoom;
      x_pos += zoom;
      render(renderer, window);
      break;
    case SDLK_RIGHT: // right
      x_pos += zoom;
      render(renderer, window);
      break;
    case SDLK_KP_6: // right keypad
      x_pos += zoom;
      render(renderer, window);
      break;
    case SDLK_KP_3: // right down keypad
      x_pos += zoom;
      y_pos += zoom;
      render(renderer, window);
      break;
    case SDLK_PLUS: // zoom in
      zoom /= 2;
      render(renderer, window);
      break;
    case SDLK_KP_PLUS: // zoom in keypad
      zoom /= 2;
      render(renderer, window);
      break;
    case SDLK_EQUALS: // zoom in
      zoom /= 2;
      render(renderer, window);
      break;
    case SDLK_MINUS: // zoom out
      zoom *= 2;
      render(renderer, window);
      break;
    case SDLK_KP_MINUS: // zoom out keypad
      zoom *= 2;
      render(renderer, window);
      break;
    case SDLK_KP_DIVIDE: // decrease iterations keypad
      if (!automatic_iters) {
        iterations /= 1.5;
        render(renderer, window);
      }
      break;
    case SDLK_KP_MULTIPLY: // increase iterations keypad
      if (!automatic_iters) {
        iterations *= 1.5;
        render(renderer, window);
      }
      break;
    case SDLK_LEFTBRACKET: // decrease iterations
      if (!automatic_iters) {
        iterations /= 1.5;
        render(renderer, window);
      }
      break;
    case SDLK_RIGHTBRACKET: // increase iterations
      if (!automatic_iters) {
        iterations *= 1.5;
        render(renderer, window);
      }
      break;
    case SDLK_r: // reset position
      x_pos = 0;
      y_pos = 0;
      zoom = 2;
      render(renderer, window);
      break;
    case SDLK_KP_0: // reset position keypad
      x_pos = 0;
      y_pos = 0;
      zoom = 2;
      render(renderer, window);
      break;
    case SDLK_F11: // fullscreen
      if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowSize(window, prevsize.real(), prevsize.imag());
      } else {
        SDL_DisplayMode DM;
        SDL_GetDesktopDisplayMode(0, &DM);
        int win_w, win_h;
        SDL_GetWindowSize(window, &win_w, &win_h);
        prevsize = std::complex<int>(win_w, win_h);

        int w = DM.w;
        int h = DM.h;
        SDL_SetWindowSize(window, w, h);
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
      }
      break;
    case SDLK_BACKSLASH: {
      unsigned long int temp_iters = iterations;
      automatic_iters = !automatic_iters;
      if (automatic_iters) {
        auto_iters();
      }
      if (iterations != temp_iters) {
        render(renderer, window);
      }
      break;
    }
    case SDLK_c:
      color += 1;
      if (color > total_colors) {
        color = 0;
      }
      render(renderer, window);
      break;
    case SDLK_v:
      set += 1;
      if (set > et_sets) {
        set = 0;
      } if (set == 3) {
        julia_c = std::complex<precision_t>(x_pos, y_pos);
      }
      render(renderer, window);
      break;
    case SDLK_f:
      if (set) {
        set -= 1;
      } else {
        set = et_sets;
      }
      if (set > et_sets) {
        set = 0;
      } if (set == 3) {
        julia_c = std::complex<precision_t>(x_pos, y_pos);
      }
      render(renderer, window);
      break;
    case SDLK_b:
      normalized = !normalized;
      render(renderer, window);
      break;
    /*case SDLK_g: // this is the dev key, for developer things.
      //for (unsigned int i = 0; i < 10000; i++) {
      //  render(renderer, window);
      //}
      return 1;
      break; */
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    if (event.button.button ==
        SDL_BUTTON_LEFT) { // center screen on mouse cursor upon left-click
      int win_w, win_h;
      SDL_GetWindowSize(window, &win_w, &win_h);

      x_pos += (double(event.button.x) / win_w - 0.5) * aspect_zoom.real() * 2;
      y_pos += (double(event.button.y) / win_h - 0.5) * aspect_zoom.imag() * 2;
      render(renderer, window);
    }
    break;
  case SDL_MOUSEWHEEL:
    if (event.wheel.y > 0) { // zoom in on mouse cursor on scroll up
      int win_w, win_h;
      SDL_GetWindowSize(window, &win_w, &win_h);

      int mouse_x, mouse_y;
      SDL_GetMouseState(&mouse_x, &mouse_y);

      x_pos += (double(mouse_x) / win_w - 0.5) * aspect_zoom.real() * 2;
      y_pos += (double(mouse_y) / win_h - 0.5) * aspect_zoom.imag() * 2;
      zoom /= 2;

      double ratioA = double(abs((-zoom + x_pos) - (zoom + x_pos)) /
                      abs((-zoom + y_pos) - (zoom + y_pos))); // maybe moving around the casting will make it faster? static cast maybe?
      double ratioB = double(win_w) / win_h;

      if (ratioB > ratioA) {
        double Xratio = ratioB / ratioA;
        aspect_zoom.real(zoom);
        aspect_zoom.imag(zoom);
        aspect_zoom.real(aspect_zoom.real() * Xratio);
      } else {
        double Yratio = ratioA / ratioB;
        aspect_zoom.real(zoom);
        aspect_zoom.imag(zoom);
        aspect_zoom.imag(aspect_zoom.imag() * Yratio);
      }

      x_pos -= (double(mouse_x) / win_w - 0.5) * aspect_zoom.real() * 2;
      y_pos -= (double(mouse_y) / win_h - 0.5) * aspect_zoom.imag() * 2;

      render(renderer, window);
    } else if (event.wheel.y <
               0) { // zoom out away from mouse cursor on scroll down
      int win_w, win_h;
      SDL_GetWindowSize(window, &win_w, &win_h);

      int mouse_x, mouse_y;
      SDL_GetMouseState(&mouse_x, &mouse_y);

      x_pos += (double(mouse_x) / win_w - 0.5) * aspect_zoom.real() * 2;
      y_pos += (double(mouse_y) / win_h - 0.5) * aspect_zoom.imag() * 2;
      zoom *= 2;

      double ratioA = double(abs((-zoom + x_pos) - (zoom + x_pos)) /
                      abs((-zoom + y_pos) - (zoom + y_pos)));
      double ratioB = double(win_w) / win_h;

      if (ratioB > ratioA) {
        double Xratio = ratioB / ratioA;
        aspect_zoom.real(zoom);
        aspect_zoom.imag(zoom);
        aspect_zoom.real(aspect_zoom.real() * Xratio);
      } else {
        double Yratio = ratioA / ratioB;
        aspect_zoom.real(zoom);
        aspect_zoom.imag(zoom);
        aspect_zoom.imag(aspect_zoom.imag() * Yratio);
      }

      x_pos -= (double(mouse_x) / win_w - 0.5) * aspect_zoom.real() * 2;
      y_pos -= (double(mouse_y) / win_h - 0.5) * aspect_zoom.imag() * 2;

      render(renderer, window);
    }
    break;
  case SDL_WINDOWEVENT:
    switch (event.window.event) {
    case SDL_WINDOWEVENT_RESIZED: // rerender on resize
      render(renderer, window);
      break;
    }
  }

  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
    int
    main(int argc, char *argv[]) {
  if (threads == 0) {
    threads = 1;
  }

  SDL_Init(SDL_INIT_VIDEO);
  atexit(SDL_Quit);

  // disable events we don't care about
  SDL_EventState(SDL_DROPFILE, SDL_DISABLE);
  SDL_EventState(SDL_DROPTEXT, SDL_DISABLE);
  SDL_EventState(SDL_DROPBEGIN, SDL_DISABLE);
  SDL_EventState(SDL_DROPCOMPLETE, SDL_DISABLE);
  SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
  SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
  SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
  SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_DISABLE);
  SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
  SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
  SDL_EventState(SDL_KEYUP, SDL_DISABLE);
  SDL_EventState(SDL_TEXTEDITING, SDL_DISABLE);
  SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);
  SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
  SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);

  if (multisample) {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
  }

  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

  SDL_Window* window =
      SDL_CreateWindow("Cool Fractal Viewer", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 256, 256, SDL_WINDOW_RESIZABLE);
  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
  SDL_SetRenderDrawBlendMode(renderer, blend);

  std::printf("Cool Fractal Viewer v%u.%u"
      "\n  Copyright (C) 2022 David Cole\n  This program "
      "is free software: you\n  can redistribute it and/or modify\n  it "
      "under the terms of the GNU General Public License as published by\n "
      " the Free Software Foundation, either version 3 of the License, "
      "or\n  (at your option) any later version.\n\n  You should have "
      "received a copy of the GNU General Public License\n  along with "
      "this program.  If not, see <https://www.gnu.org/licenses/>.\n\n", fractal_VERSION_MAJOR, fractal_VERSION_MINOR);

  if (typeid(zoom) == typeid(long double)) {
    SDL_Log("Using long precision");
  } else if (typeid(zoom) == typeid(double)) {
    SDL_Log("Using standard precision");
  } else {
    SDL_Log("Using other precision");
  }

  SDL_Log("Using %u threads", threads);

  if (argc >= 3) {
    x_pos = std::stold(argv[1], nullptr);
    y_pos = std::stold(argv[2], nullptr);
    if (argc >= 4) {
    zoom = std::stold(argv[3], nullptr);
    }
  }

  std::stringstream dtest1;
  dtest1 << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
         << d;
  std::stringstream dtest2;
  dtest2 << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
         << int(d);

  if (dtest1.str() == dtest2.str()) {
    d_is_int = true;
  } else {
    d_is_int = false;
  }

  load_icon(window);

  if (multisample) {
    int Buffers, Samples;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &Buffers);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &Samples);
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

  render(renderer, window);

  SDL_Event event;
//   bool dev_flipper = false;
  while (!quit) {
    SDL_WaitEvent(&event);
    SDL_FlushEvents(SDL_KEYDOWN, SDL_MOUSEWHEEL);

    handle_events(event, renderer, window);

    /*if(handle_events(event, renderer, window)) { // do dev things when dev key is pressed, code is very leaky
      SDL_Log("dev key pressed");
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);

      window =
      SDL_CreateWindow("Cool Fractal Viewer", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 10240, 10240, SDL_WINDOW_HIDDEN);

      renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

      render(renderer, window);
      screenshot(renderer, window);

      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);

      window =
      SDL_CreateWindow("Cool Fractal Viewer", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 256, 256, SDL_WINDOW_RESIZABLE);

      renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

      render(renderer, window);
    }*/

    // render frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, rendered, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  return 0;
}
