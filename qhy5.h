/*
 * qhy5.h
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original author of device access code Tom Vandeneede formerly Astrosoft.be
 * Copyright owner of device access code QHYCCD Astronomy http://www.qhyccd.com/
 * as per: http://qhyccd.com/ccdbbs/index.php?topic=1154.msg6531#msg6531
 * Original code: Copyright(c) 2009 Geoffrey Hausheer.
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

void qhy5_init();
int  qhy5_iscamera();
int  qhy5_reset();
int  qhy5_setregisters(qhy_exposure *expar);
void qhy5_decode(unsigned char *databuffer);
int  qhy5_bonjour();

