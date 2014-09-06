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

#define FOURMEG     4194304
#define SQR3(x) ((x)*(x)*(x))
#define SQRT3(x) (exp(log(x)/3))

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <glib/gi18n.h>
#include "libusbio.h"
#include "qhycore.h"

static struct libusb_device_handle *hDevice = NULL;
static qhy_endp     endp;
static qhy_req      req;
static qhy_campars  camp;
static char *coremsg;
static char fwpath[2048];

///////////////////////////
// Init
///////////////////////////
void qhy_core_init()
{
	static int first_time = 1;
	if (first_time)
	{
		coremsg = (char*)realloc(coremsg, 1024);
		first_time = 0;
	}
	coremsg[0] = '\0';
	
	strcpy(fwpath, FWPATH);
			
	endp.info  = 0;
	endp.read  = 0;
	endp.write = 0;
	endp.bulk  = 0;
	endp.aux   = 0;

	req.sendregs  = 0;
	req.startexp  = 0;
	req.setdc201  = 0;
	req.getdc201  = 0;
	req.shutter   = 0;
	req.wheel     = 0;
	
	camp.vid        = 0;
	camp.pid        = 0;
	camp.shortexp   = 0;
	camp.buftimes   = 0;
	camp.buftimef   = 0;	
}
///////////////////////////
// Properties
///////////////////////////
qhy_endp     *qhy_core_getendp()
{
	return &endp;
}

qhy_req      *qhy_core_getreq()
{
	return &req;
}

qhy_campars  *qhy_core_getcampars()
{
	return &camp;
}

///////////////////////////
// Private methods
///////////////////////////
double RToDegree(double R)
{
	double 	T;
	double LNR;

	if (R>400) 
	{
		R=400;
	}
	if (R<1) 
	{
		R=1;
	}

	LNR=log(R);

	T=1/( 0.002679+0.000291*LNR+LNR*LNR*LNR*4.28e-7  );

	T=T-273.15;

	return T;
}

double DegreeToR(double degree)
{

	double x,y;
	double R;
	double T;

	double A=0.002679;
	double B=0.000291;
	double C=4.28e-7;

	degree = MAX(degree, -50);
	degree = MIN(degree, 50);

	T=degree+273.15;

	y=(A-1/T)/C;
	x=sqrt( SQR3(B/(3*C))+(y*y)/4);
	R=exp(  SQRT3(x-y/2)-SQRT3(x+y/2));

	return R;
}

double mVToDegree(double V)
{
	double R;
	double T;

	R=33/(V/1000+1.625)-10;
	T=RToDegree(R);
	return T;
}

double DegreeTomV(double degree)
{
	double V;
	double R;

	R=DegreeToR(degree);
	V=33000/(R+10)-1625;

	return V;
}

///////////////////////////
// Communication functions
///////////////////////////

int qhy_cameraIO( int dir, int req, unsigned char *buf, int size, unsigned int value, unsigned int index) 
{
	int rcode;
	rcode = libusb_control_transfer(hDevice, dir, req, value, index, buf, size, 5000);
	if (rcode != size)
	{
		sprintf(coremsg, C_("qhycore","cameraIO failed, error %d"), rcode);
	}
	return (rcode==size);
}

int qhy_cameraiIO(int dir, unsigned char *buf, int size) 
{
	int rcode, i = 0;
	int length_transferred;

	rcode = libusb_bulk_transfer(hDevice, dir, buf, size, &length_transferred, 2000);
	usleep( 5000 );
	while (rcode < 0 && i++ < 10)
	{
		rcode = libusb_bulk_transfer(hDevice, dir, buf, size, &length_transferred, 2000);
		usleep( 5000 );
	}
	if (rcode < 0)
	{
		sprintf(coremsg, C_("qhycore","cameraiIO failed, error %d"), rcode);
	}
	return (rcode==0);
}

int qhy_opencamera() 
{
	int retcode = 1;

	retcode = (libusb_init(NULL) == 0);
	coremsg[0] = '\0';
	if(retcode)
	{
		// Level 0: no messages ever printed by the library (default)
		// Level 1: error messages are printed to stderr
		// Level 2: warning and error messages are printed to stderr
		// Level 3: informational messages are printed to stdout, warning and error messages are printed to stderr
		libusb_set_debug(NULL,0);
		if (open_camera(camp.vid, camp.pid, &hDevice, coremsg))
		{
			libusb_set_auto_detach_kernel_driver(hDevice, 1);
			if ((retcode = libusb_claim_interface(hDevice, 0)) == 0) 
			{
				retcode = 1;
			}
			else
			{
				libusb_close(hDevice);
				sprintf(coremsg, C_("qhycore","Error %d: Could not claim interface."), retcode);
				retcode = 0;
			}
		}
		else
		{
			libusb_close(hDevice);
			retcode = 0;
		}
	}
	else
	{
		sprintf(coremsg, C_("qhycore","Error %d: Could not initialise libusb."), retcode);
		retcode = 0;
	}
	return retcode;
	//return open_camera(camp.vid, camp.pid, &hDevice, coremsg);
}

int qhy_OpenCamera() 
{
	int retcode = 1;
	int config = 0;

	retcode = (libusb_init(NULL) == 0);
	coremsg[0] = '\0';
	if(retcode)
	{
		// Level 0: no messages ever printed by the library (default)
		// Level 1: error messages are printed to stderr
		// Level 2: warning and error messages are printed to stderr
		// Level 3: informational messages are printed to stdout, warning and error messages are printed to stderr
		libusb_set_debug(NULL,0);
		if (open_camera(camp.vid, camp.pid, &hDevice, coremsg))
		{
			libusb_set_auto_detach_kernel_driver(hDevice, 1);
			if ((retcode = libusb_get_configuration(hDevice, &config)) == 0)
			{
				if ((retcode = ((config != 1) ? libusb_set_configuration(hDevice, 1) : 0)) == 0) 
				{
					if ((retcode = libusb_claim_interface(hDevice, 0)) == 0) 
					{
						retcode = 1;
					}
					else
					{
						libusb_close(hDevice);
						sprintf(coremsg, C_("qhycore","Error %d: Could not claim interface."), retcode);
						retcode = 0;
					}
				}
				else
				{
					libusb_close(hDevice);
					sprintf(coremsg, C_("qhycore","Error %d: Could not set device configuration."), retcode);
					retcode = 0;
				
				}
			}
			else
			{
				libusb_close(hDevice);
				sprintf(coremsg, C_("qhycore","Error %d: Could not get device configuration."), retcode);
				retcode = 0;
			
			}
		}
		else
		{
			retcode = 0;
		}
	}
	else
	{
		sprintf(coremsg, C_("qhycore","Error %d: Could not initialise libusb."), retcode);
		retcode = 0;
	}
	return retcode;
}

int qhy_CloseCamera() 
{
	libusb_release_interface(hDevice, 0);
	libusb_close(hDevice);
	libusb_exit(NULL);
	return 1;
}

//////////////////////////
// Misc
//////////////////////////
char *qhy_getFwPath()
{
	return fwpath;
}

char *qhy_core_msg()
{
	return coremsg;
}

void qhy_swap(unsigned char *x) 
{
	char tmp;
	tmp = *x;
	*x=*(x+1);
	*(x+1) = tmp;
}

unsigned char qhy_MSB(unsigned int i)
{
	unsigned int j;
	j=(i&~0x00ff)>>8;
	return j;
}

unsigned char qhy_LSB(unsigned int i)
{
	unsigned int j;
	j=i&~0xff00;
	return j;
}

int qhy_getPatch (int totalsize, int refsize, int buffercamera)
{
	int P_Size, T_Size, Total_P, PatchNumber;
	
	if (totalsize > FOURMEG)
	{
		T_Size = (totalsize / FOURMEG) + (fmod(totalsize, FOURMEG) > 0.);
		while (fmod(totalsize, T_Size) > 0.)
		{
			T_Size += 1;
		}
		P_Size = totalsize / T_Size;
	}
	else
	{
		T_Size = 1;
		P_Size = ((totalsize / refsize) + (fmod(totalsize, refsize) > 0.)) * refsize;
	}
	
	if (buffercamera)
	{
		if (fmod(totalsize, P_Size) != 0.) 
		{
			 Total_P = totalsize / P_Size + 1;
			 PatchNumber = (Total_P * P_Size - totalsize) / 2 + 16;
		}
		else
		{
			 PatchNumber = 16;
		}
	}
	else
	{	
		if (fmod(totalsize, P_Size) != 0.) 
		{
			Total_P = totalsize / P_Size + 1;
			PatchNumber = Total_P * P_Size - totalsize;
		}
		else
		{
			PatchNumber = 0;
		}
	}
	//printf("TotalSize %d, T_Size %d, P_Size %d, PatchNumber %d\n", totalsize, T_Size, P_Size, PatchNumber);
	return PatchNumber;
}

int qhy_ccdStartExposure(int exposuretime) 
{
	unsigned char REG[1];
  	int retcode = 0;

	REG[0] = 100;
   	if (qhy_cameraIO(endp.write, req.startexp , REG , sizeof(REG), 0, 0)) 
   	{
   		retcode = 1;
   	}
   	return retcode;
}

int qhy_ccdAbortCapture()
{
	unsigned char REG[1];

	REG[0]=0xff;
	return qhy_cameraiIO(endp.iwrite, REG, sizeof(REG));
}

int qhy_getTrigStatus()
{
	unsigned char REG[4];
	signed short i = 0;

	if (qhy_cameraiIO(endp.iread, REG, sizeof(REG)))
	{
		i=REG[0];
	}
	return i;
}

int qhy_getCameraStatus()
{
	unsigned char REG[4];
	signed short i = 0;

	if (qhy_cameraiIO(endp.iread, REG, sizeof(REG)))
	{
		i=REG[0];
	}
	return (i == 0);
}

int qhy_cmosStartExposure(int exposuretime) 
{
	unsigned char REG[2];
	int index, value;
 	int retcode = 0;

	REG[0] = 0;
	REG[1] = 100;
 	index = exposuretime >> 16;
	value = exposuretime & 0xffff;
   	if (qhy_cameraIO(endp.aux, req.startexp, REG , sizeof(REG), value, index)) 
   	{
   		retcode = 1;
   	}
   	return retcode;
}

int qhy_cmosAbortCapture(int transfer_size)
{
	int retcode = 0;

	if (qhy_cmosStartExposure(10))
	{
		if (qhy_cmosDumpImage(transfer_size))
		{
			retcode = 1;
		}
	}
	return retcode;
}

int qhy_cmosDumpImage(int transfer_size)
{
	int error = 0, length_transferred = 0, retcode = 1;
	unsigned char databuffer[transfer_size];

	qhy_getImgData(transfer_size, databuffer, &error, &length_transferred);
	if (error < 0)
	{
   		printf( "Error: Could not dump image data from camera!\nError %d\nTransferred data %d\nExpected data %d\n", error, length_transferred, transfer_size);
   		retcode = 0;
	}
	return retcode;
}

int qhy_setDC201(int pwm, int fan) 
{
	unsigned char REG[2];

	if (pwm==0){
		REG[1]=REG[1] &~ 0x80;
		REG[0]=0;
	}
	else
	{
		REG[0]=pwm;
		REG[1]=REG[1] | 0x80;
	}
	if (fan==0) 	
	{
		REG[1]=REG[1] &~ 0x01;
	}
	else
	{
		REG[1]=REG[1] | 0x01;
	}
	return qhy_cameraIO(endp.write, req.setdc201, REG, sizeof(REG), 0, 0);
}

int qhy_getDC201(double *tC, double *mV)
{
	unsigned char REG[4];
	signed short i = 0;
	int retval = 0;

	if (qhy_cameraIO(endp.read, req.getdc201,REG,sizeof(REG), 0, 0));
	{
		i = REG[1]*256+REG[2];
		*mV = 1.024*(float)i;
		*tC = mVToDegree(*mV);
		*tC = round(*tC * 100 + 0.5) / 100;
		retval = 1;
	}
	return (retval);
}

int qhy_getDC201_i(double *tC, double *mV)
{
	unsigned char REG[4] = {0x0, 0x0, 0x0, 0x0};
	signed short i = 0;
	int retval = 0;

	if ((retval = qhy_cameraiIO(endp.iread, REG, sizeof(REG))) == 1)
	{
		i = REG[1]*256+REG[2];
		*mV = 1.024*(float)i;
		*tC = mVToDegree(*mV);
		*tC = round(*tC * 100 + 0.5) / 100;
	}
	return (retval);
}

int qhy_setDC201_i(int pwm, int fan) 
{
	unsigned char REG[3];
	REG[0] = 0x01;

	if (fan==0)
	{
		pwm = 0;
	}
	if( pwm == 0 )	// TEC off
	{
		REG[1]=0;
		REG[2]=REG[2] &~ 0x80;
	}
	else			// TEC manual
	{
		REG[1]=(unsigned char)pwm;
		REG[2]=REG[2] | 0x80;
	}
	if (fan == 0)
	{
		REG[2]=REG[2] &~ 0x01;
	}
	else
	{
		REG[2]=REG[2] | 0x01;
	}

   	return (qhy_cameraiIO(endp.iwrite, REG, sizeof(REG)));
}

int qhy_Shutter(int cmd)
{
	unsigned char REG[1];
	int retval = 0;
	//0=open  1=close  2=free
	REG[0] = cmd;
   	if (qhy_cameraIO(endp.write, req.shutter, REG,  sizeof(REG), 0, 0)) 
   	{
   		retval = 1;
		if (cmd < 2)
		{
			usleep(250 * 1000);
		}
	}
   	return retval;
}

int qhy_setColorWheel(int Pos)
{
	unsigned char REG[1];

	REG[0] = (unsigned char) Pos;    //0,1,2,3,4
   	return (qhy_cameraIO(endp.write, req.wheel, REG,  sizeof(REG), 0, 0));
}

int qhy_getImgData(int transfer_size, unsigned char *databuffer, int *errcode, int *length_transferred)
{
	*errcode = libusb_bulk_transfer( hDevice, endp.bulk, databuffer, transfer_size, length_transferred, ((transfer_size > 15000000) ? 60000: 40000));
	if (*errcode != 0)
	{
		//printf("Bulk errcode: %d\n", *errcode);
		sprintf(coremsg, C_("qhycore","getImgData failed, error %d"), *errcode);
	}
	return (*errcode == 0);
}

int qhy_EepromRead(unsigned char addr, unsigned char* data, unsigned short len)
{
	int rcode;
	rcode = libusb_control_transfer(hDevice, endp.read, 0xca, 0, addr, data, len, 0);
	if (rcode != len)
	{
		sprintf(coremsg, C_("qhycore","EepromRead failed, error %d"), rcode);
	}
	return (rcode==len);
}

int qhy_I2CTwoWrite(uint16_t addr,unsigned short value)
{
	unsigned char data[2];
	int rcode;
	data[0] = qhy_MSB(value);
	data[1] = qhy_LSB(value);

	rcode = libusb_control_transfer(hDevice, endp.write, 0xbb, 0, addr, data, 2, 0);
	if (rcode != 2)
	{
		sprintf(coremsg, C_("qhycore","I2CTwoWrite failed, error %d"), rcode);
	}
	return(rcode==2);
}

unsigned short qhy_I2CTwoRead(uint16_t addr)
{
	unsigned char data[2];

	libusb_control_transfer(hDevice, endp.read, 0xb7, 0, addr, data, 2, 0);
	return data[0] * 256 + data[1];
}

