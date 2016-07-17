/*
 * qhy5ii.h
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Device access code is based on original QHY code from https://github.com/qhyccd-lzr
 *
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

#include "guider.h"

void   minicam5_init();
int    minicam5_iscamera();
int    minicam5_reset();
int    minicam5_setregisters(qhy_exposure *expar);
void   minicam5_decode(unsigned char *databuffer);
int    minicam5_AbortCapture();
int    minicam5_bonjour();
int    minicam5_setTec(int pwm, int fan);
int    minicam5_guide(enum GuiderAxis, enum GuiderMovement);
