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
#include "imgAvi.h"

static unsigned char *databuffer = NULL;
static unsigned char *framebuffer = NULL;
static int             awidth, aheight, bytepix, doalloc;
static char           *avimsg;
static int             isopen = 0;
static char            avifile[2048];
static avi_t          *aviptr = NULL;
static int             frameno = 0;

unsigned int   imgavi_get_maxsize()
{
	return (unsigned int)AVI_max_size();
}

char *imgavi_get_name()
{
	return avifile;
}

void imgavi_set_name(char *filename)
{
	strcpy(avifile, filename);
	if (strstr(avifile, ".avi") == NULL)
	{
		strcat(avifile, ".avi");
	}
}


char *imgavi_get_msg()
{
	return avimsg;
}

unsigned char *imgavi_get_data()
{
	return databuffer;
}

void imgavi_set_data(unsigned char *data)
{
	databuffer = data;
}

int imgavi_get_width()
{
	return awidth;
}

void imgavi_set_width(int val)
{
	doalloc = (awidth != val) ? 1 : doalloc;
	awidth = val;
}

int imgavi_get_height()
{
	return aheight;
}

void imgavi_set_height(int val)
{
	doalloc = (aheight != val) ? 1 : doalloc;
	aheight = val;
}

int imgavi_get_bytepix()
{
	return bytepix;
}

void imgavi_set_bytepix(int val)
{
	doalloc = (bytepix != val) ? 1 : doalloc;
	bytepix = (val <= 1) ? 1 : 2;
}

int imgavi_isopen()
{
	return (isopen);
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
		// Databuffer is never allocated here
		// Hence we only clear the pointer when no longer needed
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
	bytepix = 1;
	doalloc = 1;
	frameno = 0;
}

int imgavi_open()
{
	int retval = 0;

	if (isopen)
	{
		imgavi_close();
	}
	avimsg[0] = '\0';
	if (strlen(avifile) > 0)
	{
		if (mkpath(avifile, 0))
		{
			if (isfile(avifile) == 0)
			{
				if ((errno != EACCES) && (errno != ENOTDIR))
				{ 
					if ((aviptr = AVI_open_output_file(avifile)) != NULL)
					{ 
						// Reset frameno
						frameno = 0;
						// Allocate suitable framebuffer, eventually
						if (doalloc == 1)
						{
							framebuffer = (unsigned char*)realloc(framebuffer, awidth * aheight * 3);
							doalloc = 0;
						}
						// Set avi properties accordingly
						AVI_set_video(aviptr, awidth, aheight, 10, "RGB");
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
					sprintf(avimsg, C_("avi","Unable to write file %s. Check permissions"), avifile);
				}
				else if (errno != ENOTDIR)
				{
					sprintf(avimsg, C_("avi","A component of the file path %s is not a folder"), avifile);
				}
			}
		}
		else
		{
			sprintf(avimsg, C_("avi","A component of the file path %s could not be created, check permissions"), avifile);
		}
	}
	return (retval);	
}

int imgavi_add()
{
	int retval = 0;
	int i, j, pval;
	unsigned char *frmptr = framebuffer;
	unsigned char *imgptr = databuffer;
	
	avimsg[0] = '\0';
	if (databuffer != NULL)
	{
		// Convert Data
		for (i = 0; i < aheight; i++)
		{
			// We invert top / bottom and left / right so imgptr is eol
			imgptr = databuffer + ((aheight - i) * bytepix * awidth);
			frmptr = framebuffer + (i * 3 * awidth);
			if (bytepix == 1)
			{ 
				for (j = 0; j < awidth; j++)
				{
					frmptr[0] = *imgptr;
					frmptr[1] = *imgptr;
					frmptr[2] = *imgptr;
					imgptr--;
					frmptr += 3;
				}
			}
			else if (bytepix == 2)
			{
				for (j = 0; j < awidth; j++)
				{
					pval = (imgptr[0] + imgptr[1] * 256) / 257;
					frmptr[0] = pval;
					frmptr[1] = pval;
					frmptr[2] = pval;
					imgptr -= 2;
					frmptr += 3;
				}
			}
		}
		// Proper add into avi
		if (AVI_write_frame(aviptr, (char *)framebuffer, (awidth * aheight * 3), frameno) == 0)
		{
			frameno++;
			retval = 1;
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
	if (aviptr != NULL)
	{
		if (AVI_close(aviptr) == 0)
		{
			aviptr = NULL;
			isopen = 0;
			retval = 1;
		}
		else
		{
			sprintf(avimsg, "%s", AVI_strerror());
		}
	}
	return (retval);
}
