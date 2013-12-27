/*
 * imgCamio.h
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

typedef struct
{
	int gain;     // 0-100 gain value
	int offset;   // 0-255 ofset value
	int time;     // in milliseconds
	int wtime;    // In milliseconds. = time unless double readout, qhyX_setregisters will compile;
	int bin;      // 1-2...
	int width;    // Subframe capture width,  if 0, no subframe
	int height;   // Subframe capture height, if 0, no subframe
	int speed;    // 0 low download speed, 1 high download speed
	int mode;     // 0 For interlaced camera 1 single exposure 2 double
	int amp;      // 0 amp off during capture, 1 amp always on, 2 amp off if exposure > 550m
	int denoise;  // 0 Off, 1 On
	int bytepix;  // Bytes x pixel 1 = 8Bit, 2 = 16Bit
	int bitpix;   // Bits  x pixel 8, 12, 16
	int totsize;  // Total size, qhyX_setregisters will compile;
	int tsize;    // Transfer size as needed for the bulk read, qhyX_setregisters will compile;
	int preview;  // 1 = Focus, 0 = capture;
	int edit;
} qhy_exposure;

typedef struct
{
	int istec;       // 0 = Not driveable tec or no tec 1 = Driveable tec
	int tecerr;      // Error setting / reading PWM
	int tecpwr;      // Basically tecmin - tecmax
	int tecmax;      // 0-255
	int tecauto;     // 0 = Manual, 1 = Seek target temp
	double tectemp;  // Only meaningful when tecauto = 1; 
	double settemp;  // Only meaningful when tecauto = 1; 
} qhy_tecpars;

typedef struct
{
	char camstr[2048];
	char binstr[256];
	char roistr[256];
	char spdstr[256];
	char modstr[256];
	char moddsc[256]; // This is the current label for multi-purpose modstr
	char ampstr[256];
	char snrstr[256];
	char bppstr[256];
	char byrstr[256];
	char tecstr[256];
	char whlstr[256];
} qhy_camui;

// imgcamio "class" methods
void            imgcam_exparcpy(qhy_exposure *copy, qhy_exposure *source);
int             imgcam_iscamera(const char *model);
qhy_tecpars    *imgcam_get_tecp();
qhy_camui      *imgcam_get_camui();
qhy_exposure   *imgcam_get_expar();
qhy_exposure   *imgcam_get_shpar();
int             imgcam_get_camid();
char           *imgcam_get_model();
void            imgcam_set_model(const char *val);
unsigned char *imgcam_get_data();
int             imgcam_loaded();
int             imgcam_connected();
char           *imgcam_get_msg();
void            imgcam_init();
char           *imgcam_init_list(int all);
int             imgcam_connect();
int             imgcam_disconnect();
int             imgcam_reset();
int             imgcam_shoot();
int             imgcam_readout();
int             imgcam_abort();
int             imgcam_settec(int pwm);
int             imgcam_gettec(double *tC, double *mV);
int             imgcam_shutter(int cmd);
int             imgcam_wheel(int pos);


