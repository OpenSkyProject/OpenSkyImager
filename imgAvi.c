/*
 * imgAvi.c
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

#include "avilib.h"
#include "imgBase.h"

static unsigned char *databuffer = NULL;
static int            awidth, aheight;
static char           *avimsg;
static int            isopen = 0;
static char           avifile[2048];
static avi_t           *aviptr = NULL;
static int             frameno = 0;

char *imgavi_get_msg()
{
	return pixmsg;
}

unsigned char *imgavi_get_data()
{
	return databuffer;
}

int imgavi_get_width()
{
	return awidth;
}

int imgavi_get_height()
{
	return aheight;
}

int imgavi_isopen()
{
	return (isopen);
}

char *imgavi_get_avifile()
{
	return avifile;
}

void imgavi_init()
{
	if (isopen)
	{
		imgavi_close();
	}
	isopen = 0;
	if (databuffer != NULL)
	{
		free(databuffer);
		databuffer = NULL;
	}
	if (aviptr != NULL)
	{
		aviptr = NULL;
	}
	avimsg = (char*)realloc(avimsg, 1024);
	avimsg[0]  = '\0';	
	avifile[0] = '\0';	
	awidth  = 0;
	aheight = 0;
	frameno = 0;
}

int imgavi_open(char *filename, int width, int height)
{
	int retval = 0;

	if (isopen)
	{
		imgavi_init();
	}
	avimsg[0] = '\0';
	if (strlen(filename) > 0)
	{
		if (isfile(filename) == 0)
		{
			if ((errno != EACCES) && (errno != ENOTDIR))
			{ 
				if ((aviptr = AVI_open_output_file(filename)) != NULL)
				{ 
					// Get image data just in case
					awidth  = width;
					aheight = height;
					frameno = 0;
					// Allocate suitable databuffer
					databuffer = (unsigned char*)realloc(databuffer, awidth * aheight * 3);
					// Set avi properties accordingly
					AVI_set_video(aviptr, awidth, aheight, 25, "RGB");
					// Internal flags
					isopen = 1;
					retval = 1;
				}
				else
				{
					sprintf(avimsg, "%s", AVI_strerror());
				}
			}
			else if (errno != EACCES)
			{
				strcpy(avimsg, "Unable to write file %s. Check permissions", filename);
			}
			else if (errno != ENOTDIR)
			{
				strcpy(avimsg, "A component of the file path %s is not a folder", filename);
			}
		}
	}
	return (retval);	
}

int imgavi_add(unsigned char *imgdata, int bytepix)
{
	int retval = 0;
	int i, j, pval;
	unsigned char *frmptr = databuffer;
	unsigned char *imgptr = imgdata;
	
	avimsg[0] = '\0';
	if (data != NULL)
	{
		// Convert Data
		for (i = 0; i < height; i++)
		{
			imgptr = imgdata + ((height - i - 1) * bytepix);
			frmptr = databuffer + (i * 3);
			if (bytepix == 1)
			{ 
				for (j = 0; j < width; j++)
				{
					frmptr[0] = *imgptr;
					frmptr[1] = *imgptr;
					frmptr[2] = *imgptr;
					imgptr++;
					frmptr += 3;
				}
			}
			else if (bytepix == 2)
			{
				for (j = 0; j < width; j++)
				{
					pval = (imgptr[0] + imgptr[1] * 256) / 257;
					frmptr[0] = pval;
					frmptr[1] = pval;
					frmptr[2] = pval;
					imgptr += 2;
					frmptr += 3;
				}
			}
		}
		// Proper add into avi
		if ((retval = AVI_write_frame(aviptr, (char *)databuffer, (awidth * aheight * 3), frameno)) == 1)
		{
			frameno++;
		}
		else
		{
			sprintf(avimsg, "%s", AVI_strerror());
		}
	}
	return (retval);	
}

int imgavi_close()
{
	int retval = 0;
		
	avimsg[0] = '\0';	
	if ((retval = AVI_close(aviptr)) == 1)
	{
		aviptr = NULL;
	}
	else
	{
		sprintf(avimsg, "%s", AVI_strerror());
	}
	return (retval);
}
