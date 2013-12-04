/*
 * imgFitsio.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Few parts of this code is adapted from examles in the lib docs: http://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/c_user/node17.html
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

// imgFitsio "class" code
#include <fitsio.h>
#include "imgBase.h"
#include "imgFitsio.h"

static unsigned char *databuffer = NULL;
static int status = 0;
static int internal = 1;
static int datatype, bitpix, bytepix, anynul;
static double nulval = 0.;
static long naxis[2];
static int naxes = 0;
static char *fitmsg;

int imgfit_get_width()
{
	return naxis[0];
}

void imgfit_set_width(int val)
{
	naxis[0] = val;
}

int imgfit_get_height()
{
	return naxis[1];
}

void imgfit_set_height(int val)
{
	naxis[1] = val;
}

int imgfit_get_bytepix()
{
	return bytepix;
}

void imgfit_set_bytepix(int val)
{
	bytepix = (val <= 1) ? 1 : 2;
	bitpix  = (bytepix == 1) ? BYTE_IMG : USHORT_IMG;
	datatype = (bytepix == 1) ? TBYTE : TUSHORT;
}

int imgfit_get_datatype()
{
	return datatype;
}

unsigned char *imgfit_get_data()
{
	return databuffer;
}

void imgfit_set_data(unsigned char *data)
{
	if ((internal == 1) && (databuffer != NULL))
	{
		// When setting a reference to external buffer, we free the internal
		// one to avoid memory leaks
		free(databuffer);
		databuffer = NULL;
	}
	internal = 0;
	databuffer = data;
	status = 0;
	fitmsg[0] = '\0';
	naxes = 2;
}

int imgfit_internal()
{
	return (internal);
}

int imgfit_loaded()
{
	return ((naxes == 2) && (status == 0));
}

char *imgfit_get_msg()
{
	return fitmsg;
}

void imgfit_init()
{
	static int first_time = 1;
	if (internal == 0)
	{
		// When allocated from imgcamio, we only clar the reference
		// Before switching to internal allocation
		// Otherwise realloc will reuse the imgcamio buffer...!!!
		databuffer = NULL;
		internal = 1;
	}
	datatype = TBYTE;
	bitpix = 8;
	bytepix = 1;
	naxes = 0;
	naxis[0] = 0;
	naxis[1] = 0;
	status  = 0;
	if (first_time)
	{
		fitmsg = (char*)realloc(fitmsg, 1024);
		first_time = 0;
	}
	fitmsg[0] = '\0';	
}

int imgfit_load_file(char *filename)
{
	int retval = 1;
	fitsfile *infptr;   
		
	// Reset... all
	imgfit_init();

	// Open the input file
	fits_open_image(&infptr, filename, READONLY, &status);
	if (status == 0) 
	{
		// Get the axis count for the image
		fits_get_img_dim(infptr, &naxes,  &status);
		if (naxes == 2)
		{
			// Get image params
			fits_get_img_param(infptr, naxes, &bitpix, &naxes, naxis, &status);
		     switch(bitpix) 
		    	{
		         case BYTE_IMG:
		             datatype = TBYTE;
		             break;
		         case SHORT_IMG:
		         case USHORT_IMG:
		             datatype = TUSHORT;
		             break;
		         case LONG_IMG:
		             datatype = TLONG;
		             break;
		         case FLOAT_IMG:
		             datatype = TFLOAT;
		             break;
		         case DOUBLE_IMG:
		             datatype = TDOUBLE;
		             break;
		     }
		     bytepix = abs(bitpix) / 8;
		     //printf("naxes=%d,Width=%d,Height=%d,Bitpix=%d, Bytepix=%d, DataType=%d\n", naxes, (int)naxis[0], (int)naxis[1], bitpix, bytepix, datatype);

			// Allocate the correct databuffer (plus bitepix element(s) to ease for/next loops
			databuffer = (unsigned char*)realloc(databuffer, (naxis[0] * naxis[1] * bytepix) + bytepix);
			
			// Read image data 
			fits_read_img(infptr, datatype, 1, (naxis[0] * naxis[1]), &nulval, databuffer, &anynul, &status);
		}
		else
		{
			strcpy(fitmsg, C_("fitsio","Fit read only works on 2 axes images"));
			retval = 0;
		}
		fits_close_file(infptr, &status);
	}
	
	if (status != 0)
	{    
		sprintf(fitmsg, C_("fitsio","cfitsio error: %d"), status);
		retval = 0;
	}
	// Cleanup
	//free(infptr);

	return (retval);
}

int imgfit_save_file(char *filename)
{
	int retval = 1;
	fitsfile *ofptr;   
	char fname[2048] = "!";	// ! for deleting existing file and create new
	strcat(fname, filename);

	// Create the new file
	fits_create_file(&ofptr, fname, &status); 
	if (status == 0)
	{
		fits_create_img(ofptr, bitpix, naxes, naxis, &status);
		// Header update must go here
		//fits_update_key(fptr, TSTRING, "SOFTWARE", PROGNAME,"", &status);
		//fits_update_key(fptr, TSTRING, "CAMERA", CAMERANAME,"", &status);
		//fits_update_key(fptr, TLONG,   "GAIN", &gain,"", &status);
		//fits_update_key(fptr, TLONG,   "OFFSET", &offset,"", &status);
		//fits_update_key(fptr, TLONG,   "AMP", &amp,"", &status);
		//fits_update_key(fptr, TLONG,   "EXPOSURE", &exposuretime, "Total Exposure Time", &status);
		//fits_update_key(fptr, TDOUBLE, "SENSTEMP", &temp, "Sensor temperature", &status);
		//
		fits_write_img(ofptr, datatype, 1, (naxis[0] * naxis[1]), databuffer, &status);
		fits_close_file(ofptr, &status);
	}

	if (status != 0)
	{    
		sprintf(fitmsg, C_("fitsio","cfitsio error: %d"), status);
		retval = 0;
	}
	//free(ofptr);
	
	return (retval);
}


