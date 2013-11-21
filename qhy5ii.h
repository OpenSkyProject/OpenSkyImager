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

void   qhy5ii_init();
int    qhy5ii_iscamera();
int    qhy5ii_reset();
int    qhy5ii_setregisters(qhy_exposure *expar);
void   qhy5ii_decode(unsigned char *databuffer);
int    qhy5ii_AbortCapture();
int    qhy5ii_bonjour();
int    qhy5ii_set_imgsize(int wdt, int hgt);
double qhy5lii_GetTemp();

// Not meant to be used elsewhere
int    qhy5ii_SetSpeed(int i);
int    qhy5ii_SetUSBTraffic(int i);
int    qhy5lii_SetHDR(int on);
int    qhy5lii_SetDepth(int Bpp);
int    qhy5ii_SetExposureTime(int etime);
int    qhy5ii_SetGain(int gain);
void   qhy5lii_SetGainMono(double gain);
void   qhy5lii_SetGainColor(double gain, double RG, double BG);
void   qhy5ii_set_Resolution();
void   qhy5lii_set_1280x960();
void   qhy5lii_set_1024x768();
void   qhy5lii_set_800x600();
void   qhy5lii_set_640x480();
void   qhy5lii_set_320x240();
void   qhy5liiInitRegs();
double qhy5lii_setPLL(unsigned char clk);

