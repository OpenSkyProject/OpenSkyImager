/*
 * dsi2pro.h
 *
 *  Created on: 23.01.2014
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original author of device access code by Maxim Parygin
 * Hints got from lin_guider by Galaxy Master (http://galaxymstr.users.sourceforge.net/)
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

void    dsi2pro_init();
int     dsi2pro_iscamera();
int     dsi2pro_OpenCamera();
int     dsi2pro_CloseCamera();
int     dsi2pro_StartExposure(qhy_exposure *expar);
int     dsi2pro_AbortCapture();
double  dsi2pro_GetTemp();
char   *dsi2pro_core_msg();
int     dsi2pro_reset();
int     dsi2pro_getImgData();
void    dsi2pro_decode(unsigned char *databuffer);

