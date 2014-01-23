/*
 * qhycore.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Device access code is based on original QHYCCD.inc code from 
 * https://github.com/qhyccd-lzr
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
#ifndef MAX
	#define MAX(x,y) ((x)>(y))?(x):(y)
#endif
#ifndef MIN
	#define MIN(x,y) ((x)<(y))?(x):(y)
#endif	
#define STORE_WORD_BE(var, val) *(var) = ((val) >> 8) & 0xff; *((var) + 1) = (val) & 0xff
#define FWPATH "/etc/qhyccd"

// Base qhy routines
typedef struct
{
	int vid;
	int pid;
	int shortexp;
	int buftimes;
	int buftimef;
} qhy_campars;

typedef struct
{
	int info;     // Camera info read, not implemented so far
	int read;     // Read
	int write;    // Write
	int iread;    // iRead
	int iwrite;   // iWrite
	int bulk;     // Image download (bulk read)
	int aux;      // for cmos??
} qhy_endp;

typedef struct
{
	int sendregs; // Send registers to camers
	int startexp; // Start exposure
	int getdc201; // Read temp from DC201
	int setdc201; // Set pwm on dc201
	int shutter;  // Shutter command
	int wheel;    // Wheel command
} qhy_req;

// Functions prototypes
///////////////////////////
// Init
///////////////////////////
void qhy_core_init();
///////////////////////////
// Properties
///////////////////////////
qhy_endp             *qhy_core_getendp();
qhy_req              *qhy_core_getreq();
qhy_campars          *qhy_core_getcampars();
///////////////////////////
// Basic Communication functions
///////////////////////////
int qhy_cameraIO( int dir, int req, unsigned char *buf, int size, unsigned int value, unsigned int index); 
int qhy_cameraiIO(int dir, unsigned char *buf, int size);
int qhy_opencamera();
int qhy_OpenCamera();
int qhy_CloseCamera();

//////////////////////////
// Misc
//////////////////////////
char          *qhy_getFwPath();
char          *qhy_core_msg();
void 		 qhy_swap(unsigned char *x);
unsigned char qhy_MSB(unsigned int i);
unsigned char qhy_LSB(unsigned int i);

int qhy_cmosStartExposure(int exposuretime);
int qhy_cmosAbortCapture(int transfer_size);
int qhy_cmosDumpImage(int transfer_size);

int qhy_ccdStartExposure(int exposuretime);
int qhy_ccdAbortCapture();

int qhy_getPatch (int totalsize, int refsize, int buffercamera);
int qhy_getTrigStatus();
int qhy_getCameraStatus();
int qhy_setDC201(int pwm, int fan);
int qhy_getDC201(double *tC, double *mV);
int qhy_getDC201_i(double *tC, double *mV);
int qhy_setDC201_i(int pwm, int fan);
int qhy_Shutter(int cmd);
int qhy_setColorWheel(int Pos);
int qhy_getImgData(int transfer_size, unsigned char *databuffer, int *errcode, int *length_transferred);


int             qhy_EepromRead(unsigned char addr, unsigned char* data, unsigned short len);
int             qhy_I2CTwoWrite(uint16_t addr,unsigned short value);
unsigned short qhy_I2CTwoRead(uint16_t addr);

