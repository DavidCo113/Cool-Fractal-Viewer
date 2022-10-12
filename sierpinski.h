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

#include <SDL2/SDL.h>
#include <complex>
#include <vector>

inline int render_triangle(SDL_Renderer* renderer,
						   std::complex<double> pos1,
						   std::complex<double> pos2,
						   std::complex<double> pos3,
						   int r,
						   int g,
						   int b) {
	SDL_Vertex vert[3];
	// center
	vert[0].position.x = pos1.real();
	vert[0].position.y = pos1.imag();
	vert[0].color.r = r;
	vert[0].color.g = g;
	vert[0].color.b = b;
	vert[0].color.a = 255;

	// left
	vert[1].position.x = pos2.real();
	vert[1].position.y = pos2.imag();
	vert[1].color.r = r;
	vert[1].color.g = g;
	vert[1].color.b = b;
	vert[1].color.a = 255;

	// right
	vert[2].position.x = pos3.real();
	vert[2].position.y = pos3.imag();
	vert[2].color.r = r;
	vert[2].color.g = g;
	vert[2].color.b = b;
	vert[2].color.a = 255;

	SDL_RenderGeometry(renderer, NULL, vert, 3, NULL, 0);
	return 0;
}

inline std::vector<std::complex<double>> render_invert_triangle(SDL_Renderer* renderer,
																std::complex<double> pos1,
																std::complex<double> pos2,
																std::complex<double> pos3) {
	double x2 = pos2.real();
	double y2 = pos1.imag();
	double x1 = pos3.real() - x2;
	double y1 = pos3.imag() - y2;

	std::complex<double> pos1t(x1 / 2 + x2, y1 + y2);
	std::complex<double> pos2t(x1 / 4 + x2, y1 / 2 + y2);
	std::complex<double> pos3t(x1 / 4 + x1 / 2 + x2, y1 / 2 + y2);

	render_triangle(renderer, pos1t, pos2t, pos3t, 128, 128, 128);
	return {pos1t, pos2t, pos3t};
}

inline std::vector<std::vector<std::complex<double>>> render_sierp_iter(SDL_Renderer* renderer,
																		std::vector<std::complex<double>> inv) {
	std::vector<std::complex<double>> inv2 = inv;
	std::vector<std::complex<double>> inv3 = inv;
	inv.at(0).imag(inv.at(0).imag() + (inv.at(2).imag() - inv.at(0).imag()) * 2);
	inv2.at(1).real(inv2.at(0).real() - (inv2.at(1).real() - inv2.at(2).real()));
	inv2.at(1).imag(inv2.at(0).imag());
	swap(inv2[0], inv2[2]);
	inv3.at(2).real(inv3.at(0).real() - (inv3.at(2).real() - inv3.at(1).real()));
	inv3.at(2).imag(inv3.at(0).imag());
	swap(inv3[0], inv3[1]);
	inv = render_invert_triangle(renderer, inv.at(0), inv.at(1), inv.at(2));
	inv2 = render_invert_triangle(renderer, inv2.at(0), inv2.at(1), inv2.at(2));
	inv3 = render_invert_triangle(renderer, inv3.at(0), inv3.at(1), inv3.at(2));

	return {inv, inv2, inv3};
}

inline SDL_Texture* render_sierpinski(SDL_Renderer* renderer,
									  SDL_Window* window,
									  int iterations,
									  SDL_Texture* rendered) {
	int win_w, win_h;
	SDL_GetWindowSize(window, &win_w, &win_h);

	rendered = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, win_w, win_h);
	SDL_SetRenderTarget(renderer, rendered);

	SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
	SDL_RenderClear(renderer);

	std::vector<std::vector<std::complex<double>>> positions;
	std::vector<std::vector<std::complex<double>>> temp_positions;

	double scale;
	if (win_w < win_h) {
		scale = win_w;
	} else {
		scale = win_h;
	}

	if (iterations >= 1) {
		render_triangle(renderer, std::complex<double>(double(win_w) / 2, double(win_h) / 2 - scale / 2),
						std::complex<double>(double(win_w) / 2 - scale / 2, double(win_h) / 2 + scale / 2),
						std::complex<double>(double(win_w) / 2 + scale / 2, double(win_h) / 2 + scale / 2), 0, 0, 0);
		if (iterations >= 2) {
			std::vector<std::complex<double>> inv = render_invert_triangle(
				renderer, std::complex<double>(double(win_w) / 2, double(win_h) / 2 - scale / 2),
				std::complex<double>(double(win_w) / 2 - scale / 2, double(win_h) / 2 + scale / 2),
				std::complex<double>(double(win_w) / 2 + scale / 2, double(win_h) / 2 + scale / 2));
			if (iterations >= 3) {
				std::vector<std::vector<std::complex<double>>> invs = render_sierp_iter(renderer, inv);
				positions.insert(positions.end(), invs.begin(), invs.end());
			}
		}
	}

	for (int i = 0; i < iterations - 3; i++) {
		while (positions.size()) {
			std::vector<std::vector<std::complex<double>>> out = render_sierp_iter(renderer, positions.at(0));
			positions.erase(positions.begin());
			temp_positions.insert(temp_positions.end(), out.begin(), out.end());
		}
		positions = temp_positions;
	}

	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, rendered, NULL, NULL);
	SDL_RenderPresent(renderer);
	return rendered;
}
