/*
 * imgBase.h
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <wait.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "tools.h"

#define APPICO "osi.png"
#define ORZHND "handle_v.png"
#define VRTHND "handle_o.png"
#define CRSSICO "crosshair.png"
#define APPNAM "OpenSkyImager"
#define APPTIT "OpenSkyImager (c) 2013 JP & C AstroSoftware"
#define APPVER "0.9.5"

#ifdef DECLARE_MAIN
	char imgBasePath[PATH_MAX];
	char imgAppIco[PATH_MAX];
	char imgOrzHnd[PATH_MAX];
	char imgVrtHnd[PATH_MAX];
#else
	extern char imgBasePath[PATH_MAX];
	extern char imgAppIco[PATH_MAX];
	extern char imgOrzHnd[PATH_MAX];
	extern char imgVrtHnd[PATH_MAX];
#endif
