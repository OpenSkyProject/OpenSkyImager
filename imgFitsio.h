/*
 * imgFitsio.c
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

#include <fitsio.h>
#include "imgBase.h"

// imgFitsio "class" methods
int             imgfit_get_width();
void            imgfit_set_width(int val);
int             imgfit_get_height();
void            imgfit_set_height(int val);
int             imgfit_get_bytepix();
void            imgfit_set_bytepix(int val);
int             imgfit_get_datatype();
unsigned char *imgfit_get_data();
void            imgfit_set_data(unsigned char *data);
int             imgfit_internal();
int             imgfit_loaded();
char           *imgfit_get_msg();
void            imgfit_init();
int             imgfit_load_file(char *filename);
int             imgfit_save_file(char *filename);

