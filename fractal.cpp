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

#include "build/config.h"
#include "sierpinski.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <algorithm>
#include <chrono>
#include <complex>
#include <filesystem>
#include <future>
#include <iomanip>
#include <thread>
#include <typeinfo>
#include <vector>
std::string set = "mandelbrot";
std::vector<std::string> et_sets = {"mandelbrot", "tricorn",
                                    "burning_ship"}; // escape-time sets
unsigned long int iterations = 1024;
unsigned long int auto_iterations = 16384;
long double d = 2;
unsigned int radius = 4; // probably shouldn't change this
int resolution = 500;
bool multisample = false;
int r = 2;
int g = 4;
int b = 8;
unsigned int threads = std::thread::hardware_concurrency();
bool automatic_iters = true;
#ifdef USE_LONGDOUBLE
long double zoom = default_zoom;
#else
double zoom = default_zoom;
#endif
std::complex<long double> aspect_zoom(zoom, zoom);
long double x_pos = 0;
long double y_pos = 0;
SDL_BlendMode blend = SDL_BLENDMODE_NONE;
SDL_Texture *rendered;
bool d_is_int;
std::complex<int> prevsize;

char *str_to_char(std::string str) {
  char *out = new char[str.size() + 1];
  strcpy(out, str.c_str());
  return out;
}

int load_icon(SDL_Window *window) {
  SDL_RWops *file = SDL_RWFromFile("icon.bmp", "rb");
  SDL_Surface *image = SDL_LoadBMP_RW(file, SDL_TRUE);

  if (!image) {
    SDL_Log("Failed to load icon");
  }

  SDL_SetWindowIcon(window, image);
  SDL_FreeSurface(image);

  return 0;
}

int in_mand_set(std::complex<long double> c) {
  std::complex<long double> z(0, 0);
  for (unsigned int i = 0; i < iterations; i++) {
    z = std::pow(z, d) + c;
    if (std::norm(z) > 4) {
      return i;
    }
  }
  return 0;
}

int in_mand_set_orbit(std::complex<long double> c) {
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

int in_mand_set2(std::complex<long double> c) {
  std::complex<long double> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    long double xtemp = z.real() * z.real() - z.imag() * z.imag() +
                        c.real(); // z = std::pow(z, d) + c;
    z.imag(2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);
    if (std::norm(z) > radius) {
      return iterations - i;
    }
  }
  return 0;
}

double in_mand_set_norm(std::complex<long double> c) {
  std::complex<long double> z(0, 0);
  unsigned int i = iterations;
  while (i) {
    i--;
    long double xtemp = z.real() * z.real() - z.imag() * z.imag() +
                        c.real(); // z = std::pow(z, d) + c;
    z.imag(2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);
    double modulus = sqrt(z.real() * z.real() + z.imag() * z.imag());

    if (modulus > radius) {
      double mu = iterations - i - (log(log(modulus))) / log(2);
      return mu;
    }
  }
  return 0;
}

int auto_iters() {
  long double f = sqrt(0.001 + 2 * std::min(abs((-aspect_zoom.real() + x_pos) -
                                           (aspect_zoom.real() + x_pos)),
                                       abs((-aspect_zoom.imag() + y_pos) -
                                           (aspect_zoom.imag() + y_pos))));

  iterations = floor(223 / f);
  return 0;
}

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

std::vector<unsigned int> hue_to_rgb(double h) {
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
  return std::vector<unsigned int>{red, green, blue};
}

int in_ship_set(std::complex<double> c) {
  std::complex<double> z(0, 0);
  for (unsigned int i = 0; i < iterations; i++) {
    double xtemp = z.real() * z.real() - z.imag() * z.imag() + c.real();
    z.imag(abs(2 * z.real() * z.imag()) + c.imag());
    z.real(xtemp);
    if (std::norm(z) > 4) {
      return i;
    }
  }
  return 0;
}

int in_tric_set(std::complex<double> c) {
  std::complex<double> z(0, 0);
  for (unsigned int i = 0; i < iterations; i++) {
    double xtemp = z.real() * z.real() - z.imag() * z.imag() + c.real();
    z.imag(-2 * z.real() * z.imag() + c.imag());
    z.real(xtemp);
    if (std::norm(z) > 4) {
      return i;
    }
  }
  return 0;
}

int screenshot(SDL_Renderer *renderer, SDL_Window *window) {
  int win_w, win_h;
  SDL_GetWindowSize(window, &win_w, &win_h);

  std::filesystem::create_directory("screenshots");
  std::string filename = "screenshots/" + set + "_";

  filename = filename + std::to_string(x_pos) + "," + std::to_string(y_pos) +
             "," + std::to_string(zoom) + "," + std::to_string(iterations) +
             "," + std::to_string(r) + "," + std::to_string(g) + "," +
             std::to_string(b) + "-" + std::to_string(win_w) + "x" +
             std::to_string(win_h) + ".bmp";
  char *filenamec = str_to_char(filename);

  SDL_Surface *image = SDL_CreateRGBSurface(0, win_w, win_h, 32, 0, 0, 0, 0);
  SDL_RenderReadPixels(renderer, NULL, 0, image->pixels, image->pitch);
  SDL_SaveBMP(image, filenamec);
  SDL_FreeSurface(image);
  return 0;
}

int render_escape_time_boundary(SDL_Renderer *renderer,
                                SDL_Window *window) { // WIP
  //   iterations = sqrt(2 * 1/std::min(aspect_zoom.real(),
  //   aspect_zoom.imag()));

  int xc = fpclassify(x_pos);
  int yc = fpclassify(y_pos);
  int zoomc = fpclassify(zoom);
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

  int xc = fpclassify(x_pos);
  int yc = fpclassify(y_pos);
  int zoomc = fpclassify(zoom);
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

std::vector<double> render_normalized(unsigned int starty, unsigned int endy,
                                      unsigned int ix, int win_w, int win_h,
                                      unsigned int iz) {
  std::vector<double> renderedvec;

  double iters;

  // start the main render loop
  for (double x = ix; x < win_w; x += iz) {
    for (double y = starty; y < endy; y += 1) {
      iters = 0;
      long double x_point = std::lerp(-aspect_zoom.real() + x_pos,
                                      aspect_zoom.real() + x_pos, x / win_w);
      long double y_point = std::lerp(-aspect_zoom.imag() + y_pos,
                                      aspect_zoom.imag() + y_pos, y / win_h);
      if (set == "mandelbrot") {
        iters = in_mand_set_norm(std::complex<long double>(x_point, y_point));
      } else if (set == "tricorn") {
        iters = in_tric_set(std::complex<double>(x_point, y_point));
      } else if (set == "burning_ship") {
        iters = in_ship_set(std::complex<double>(x_point, y_point));
      }

      renderedvec.push_back(iters);
    }
  }

  return renderedvec;
}

int render(SDL_Renderer *renderer, SDL_Window *window) {
  // if current fractal uses escape time, render it.
  if (find(et_sets.begin(), et_sets.end(), set) != et_sets.end()) {
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);

    rendered = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                 SDL_TEXTUREACCESS_TARGET, win_w, win_h);
    SDL_SetRenderTarget(renderer, rendered);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    int chunksize = win_h / threads;

    int xc = fpclassify(x_pos);
    int yc = fpclassify(y_pos);
    int zoomc = fpclassify(zoom);
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
    }

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
    if (automatic_iters)
      auto_iters();

    std::stringstream log;
    log << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
        << x_pos << "," << y_pos << ":" << zoom << " (" << iterations << ")";

    std::string logs = log.str();

    char *logc = str_to_char(logs);
    SDL_Log(logc);

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
      std::vector<double> renderedvec;
      std::vector<std::future<std::vector<double>>> futures;

      // render on other threads
      for (unsigned int i = 0; i < threads - 1; i += 1) {
        futures.push_back(std::async(render_normalized, chunksize * i,
                                     chunksize * (i + 1), ix, win_w, win_h, 4));
      }

      // do some rendering on the main thread
      std::vector<double> lastrender = render_normalized(
          chunksize * (threads - 1), win_h, ix, win_w, win_h, 4);

      // wait for the threads
      for (unsigned int i = 0; i < threads - 1; i += 1) {
        std::vector<double> temprendered = futures[i].get();
        renderedvec.insert(renderedvec.end(), temprendered.begin(),
                           temprendered.end());
      }

      renderedvec.insert(renderedvec.end(), lastrender.begin(),
                         lastrender.end());

      double iters;
      unsigned int ticker = 0;
      unsigned int starty;
      unsigned int endy;

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
              std::vector<unsigned int> rgb =
                  hue_to_rgb(std::fmod(iters, 360));
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
    }

    SDL_SetRenderTarget(renderer, NULL);

    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    SDL_SetWindowTitle(window,
                       str_to_char("Cool Fractal Viewer (" +
                                   std::to_string(diff.count()) + " s)"));

  } else if (set == "sierpinski") {
    SDL_SetWindowTitle(window, "Cool Fractal Viewer (Rendering)");
    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();

    rendered = render_sierpinski(renderer, window, iterations, rendered);

    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    SDL_SetWindowTitle(window,
                       str_to_char("Cool Fractal Viewer (" +
                                   std::to_string(diff.count()) + " s)"));
  }
  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
    int
    main(int argc, char *argv[]) {
  bool quit = false;
  if (threads == 0) {
    threads = 1;
  }
  SDL_Event event;

  SDL_Init(SDL_INIT_VIDEO);
  atexit(SDL_Quit);

  if (multisample) {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
  }

  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

  SDL_Window *window =
      SDL_CreateWindow("Cool Fractal Viewer", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 256, 256, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
  SDL_SetRenderDrawBlendMode(renderer, blend);

  if (typeid(zoom) == typeid(long double)) {
    SDL_Log("Using long precision");
  } else if (typeid(zoom) == typeid(double)) {
    SDL_Log("Using standard precision");
  } else {
    SDL_Log("Using other precision");
  }

  SDL_Log(str_to_char("Using " + std::to_string(threads) + " threads"));

  if (argc >= 4) {
    x_pos = strtod(argv[1], nullptr);
    y_pos = strtod(argv[2], nullptr);
    zoom = strtod(argv[3], nullptr);
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

  while (!quit) {
    SDL_WaitEvent(&event);

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
      case SDLK_BACKSLASH:
        automatic_iters = !automatic_iters;
        if (automatic_iters) {
          render(renderer, window);
        }
        break;
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button ==
          SDL_BUTTON_LEFT) { // center screen on mouse cursor upon left-click
        int win_w, win_h;
        SDL_GetWindowSize(window, &win_w, &win_h);

        x_pos +=
            (double(event.button.x) / win_w - 0.5) * aspect_zoom.real() * 2;
        y_pos +=
            (double(event.button.y) / win_h - 0.5) * aspect_zoom.imag() * 2;
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

        double ratioA = abs((-zoom + x_pos) - (zoom + x_pos)) /
                        abs((-zoom + y_pos) - (zoom + y_pos));
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

        double ratioA = abs((-zoom + x_pos) - (zoom + x_pos)) /
                        abs((-zoom + y_pos) - (zoom + y_pos));
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
    // render frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, rendered, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_FlushEvents(SDL_KEYDOWN, SDL_MOUSEWHEEL);
  }

  return 0;
}
