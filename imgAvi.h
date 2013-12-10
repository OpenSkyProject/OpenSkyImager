/*
 * imgAvi.c
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

char           *imgavi_get_msg();
unsigned char *imgavi_get_data();
int            imgavi_get_width();
int            imgavi_get_height();
int            imgavi_isopen();
char           *imgavi_get_avifile();
void           imgavi_init();
int            imgavi_open(char *filename, int width, int height);
int            imgavi_add(unsigned char *data, int bytepix);
