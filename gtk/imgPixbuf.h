/*
 * imgPixbuf.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * This file is part of "OpenSkyImager".
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// imgPixbuf "class" methods
char       *imgpix_get_msg();
int         imgpix_get_width();
int         imgpix_get_height();
GdkPixbuf   *imgpix_get_data();
int         imgpix_save_data(char *path);
GdkPixbuf   *imgpix_get_histogram(int scale);
int         imgpix_save_histogram_data(char *path);
GdkPixbuf   *imgpix_get_crosshair(int size);
GdkPixbuf   *imgpix_get_roi_square(int size);
GdkPixbuf   *imgpix_get_roi_data(int centerx, int centery, int size);
int         imgpix_loaded();
void        imgpix_init();
void        imgpix_init_histogram();
int         imgpix_load(unsigned char *databuffer, int width, int height, int bytepix, int debayer, int maxadu, int minadu);

