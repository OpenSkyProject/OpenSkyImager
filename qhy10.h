/*
 * qhy10.h
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *	Modified by Daniel Holler (astrodan02@gmail.com)
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

void qhy10_init();
int  qhy10_iscamera();
int  qhy10_reset();
int  qhy10_setregisters(qhy_exposure *expar);
void qhy10_decode(unsigned char *databuffer);

