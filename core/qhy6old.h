/*
 * qhy6old.h
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original author of device access code Tom Vandeneede formerly Astrosoft.be
 * Copyright owner of device access code QHYCCD Astronomy http://www.qhyccd.com/
 * as per: http://qhyccd.com/ccdbbs/index.php?topic=1154.msg6531#msg6531
 * Original code: Tom Vandeneede formerly Astrosoft.be.
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

void qhy6old_init();
int  qhy6old_iscamera();
int  qhy6old_reset();
int  qhy6old_setregisters(qhy_exposure *expar);
void qhy6old_decode(unsigned char *databuffer);
float add_field(unsigned short *databuffer, int x1, int y1, int x2, int y2);

