/*
 * imgPixbuf.c
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

// imgPixbuf "class" code
#include <gtk/gtk.h>
#include "imgBase.h"
#include "imgPixbuf.h"

static GdkPixbuf *pixbuf = NULL;
static GdkPixbuf *hstbuf = NULL;
static GdkPixbuf *roibuf = NULL;
int pwidth, pheight, pdebayer;
static char     *pixmsg;
static int       loaded;
static double hst[256];


/* for FWHM */
extern GtkWidget *lbl_fbkfwhm;
extern char fwhmfbk[];
extern int capture;

char *imgpix_get_msg()
{
	return pixmsg;
}

GdkPixbuf *imgpix_get_data()
{
	return pixbuf;
}

int imgpix_get_width()
{
	return (pwidth / ((pdebayer > 0) ? 2 : 1));
}

int imgpix_get_height()
{
	return (pheight / ((pdebayer > 0) ? 2 : 1));
}

GdkPixbuf *imgpix_get_histogram(int scale)
{
	int rowstride;
	int row, col;
	guchar *pixels, *p;
	
	rowstride = gdk_pixbuf_get_rowstride (hstbuf);
	pixels    = gdk_pixbuf_get_pixels (hstbuf);
	
	for (row = 255; row >= 0; row--)
	{
		for (col = 0; col < 256; col++)
		{
			p = pixels + row * rowstride + col * 3;
			if ((hst[col] * scale) > (255 - row))
			{
				p[0] = 0;
				p[1] = 0;
				p[2] = 0;
			}
			else
			{
				p[0] = 200;
				p[1] = 200;
				p[2] = 200;
			}
		}
	}
	return hstbuf;
}

void imgpix_init_histogram()
{
	int rowstride;
	int row, col;
	guchar *pixels, *p;
	
	rowstride = gdk_pixbuf_get_rowstride (hstbuf);
	pixels    = gdk_pixbuf_get_pixels (hstbuf);
	
	for (row = 255; row >= 0; row--)
	{
		for (col = 0; col < 256; col++)
		{
			p = pixels + row * rowstride + col * 3;
			p[0] = 200;
			p[1] = 200;
			p[2] = 200;
		}
	}
}

GdkPixbuf *imgpix_get_roi(int size)
{
	int rowstride;
	int row, col;
	guchar *pixels, *p;
	
	roibuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, size, size);
	rowstride = gdk_pixbuf_get_rowstride (roibuf);
	pixels    = gdk_pixbuf_get_pixels (roibuf);
	
	for (row = 0; row < size; row++)
	{
		for (col = 0; col < size; col++)
		{
			p = pixels + row * rowstride + col * 4;
			p[0] = ((row == 0) || (row == (size - 1)) || (col == 0) || (col == (size - 1))) ? 150 : 0;
			p[1] = 0;
			p[2] = 0;
			p[3] = ((row == 0) || (row == (size - 1)) || (col == 0) || (col == (size - 1))) ? 255 : 0;
		}
	}
	return roibuf;
}

int imgpix_loaded()
{
	return (loaded);
}

void imgpix_init()
{
	int col;
	if (pixbuf != NULL)
	{
		g_object_unref(pixbuf);
		pixbuf = NULL;
	}
	pixmsg = (char*)realloc(pixmsg, 1024);
	pixmsg[0] = '\0';	
	// Init histogram data array
	for (col = 0; col < 256; col++)
	{
		hst[col] = 0;
	}
	// Init histogram 
	if (hstbuf != NULL)
	{
		g_object_unref(hstbuf);
		hstbuf = NULL;
	}
	hstbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 256, 256);
	imgpix_init_histogram();
	loaded = 0;	
}

int imgpix_load(unsigned char *databuffer, int width, int height, int bytepix, int debayer, int maxadu, int minadu)
{
	int retval = 1;
	int rowstride, bpp;
	guchar *pixels, *p;
	int row, col, pix, size = (width * height);
	double resample = 1;
	unsigned char *pix8;
	unsigned char *pix16;
    /* for FWHM */
    int ret, fwhm, peak;
	
	// Reset... all
	imgpix_init();
	pwidth  = width;
	pheight = height; 
	pdebayer = debayer;
	// Failsafe
	resample = (maxadu >= minadu) ? (maxadu - minadu + 1) / 255. : 1.;
	
	if (debayer == 0)
	{
		// Mono
		// Creates an empty pixbuf using appropriate geometry
		if (pixbuf != NULL)
		{
			g_object_unref(pixbuf);
			pixbuf = NULL;
		}
		pixbuf    = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
		rowstride = gdk_pixbuf_get_rowstride (pixbuf);
		pixels    = gdk_pixbuf_get_pixels (pixbuf);
		bpp       = gdk_pixbuf_get_n_channels (pixbuf);
	
		if (bytepix == 1)
		{
			pix8 = databuffer;
			for (row = 0; row < height; row++)
			{
				for (col = 0; col < width; col++)
				{
					// This is because fits image data is upside down
					p = pixels + (height - row - 1) * rowstride + (width - col - 1) * bpp;
					pix = (*pix8 - minadu) / resample;
					pix = MIN(255, MAX(pix, 0));
					p[0] = pix;
					p[1] = pix;
					p[2] = pix;
					// Move along input array
					hst[*pix8] += 1;
					pix8++;
				}
			}
		}
		else if (bytepix == 2)
		{
			int pixval;

			pix8 = databuffer;
			for (row = 0; row < height; row++)
			{
				for (col = 0; col < width; col++)
				{
					// This is because fits image data is upside down
					p = pixels + (height - row - 1) * rowstride + (width - col - 1) * bpp;
					// Move along input array while reading
					pixval = *pix8; pix8++;
					pixval += *pix8 * 256; pix8++;
					hst[pixval / 257] += 1;
					pixval = (pixval - minadu) / resample;
					pixval = MIN(255, MAX(pixval, 0));
					p[0] = pixval;
					p[1] = pixval;
					p[2] = pixval;
				}
			}
		}
		else
		{
			strcpy(pixmsg, C_("pixbuf","Only works with 8/16bpp images"));
			retval = 0;
		}
	}
	else
	{
		// Color	
		// Pattern
		char mask[4];
		int endh = (height / 2), endw = (width / 2);
		int pixH, pixR, pixG, pixB;
		GdkPixbuf *pixtmp;
		
		// Creates an empty pixbuf using appropriate geometry
		pixtmp    = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, endw, endh);
		rowstride = gdk_pixbuf_get_rowstride (pixtmp);
		pixels    = gdk_pixbuf_get_pixels (pixtmp);
		bpp       = gdk_pixbuf_get_n_channels (pixtmp);

		switch (debayer)
		{
			case	1:
				//GBRG
				mask[0] = 0; mask[1] = -1; mask[2] = 1; mask[3] = 0;
				break;
			case	2:
				//RGGB
				mask[0] = 1; mask[1] = 0; mask[2] = 0; mask[3] = -1;
				break;
			case	3:
				//GRBG
				mask[0] = 0; mask[1] = 1; mask[2] = -1; mask[3] = 0;
				break;
			case	4:
				//BGGR
				mask[0] = -1; mask[1] = 0; mask[2] = 0; mask[3] = 1;
				break;
		}
		// Debayer super-simple 
		if (bytepix == 1)
		{
			for (row = 0; row < (endh - 1); row++)
			{
				for (col = 0; col < endw; col++)
				{
					// This is because fits image data is upside down
					p = pixels + (endh - row - 1) * rowstride + (endw - col - 1) * bpp;
					pixH = 0;	
					// From current fit row
					for (pix = 0; pix < 2; pix++)
					{
						pix8 = (databuffer + row * width * 2 + col * 2) + pix;
						pixH += *pix8;
						if (mask[pix] == 0)
						{
							// Green pix
							pixG = (*pix8 - minadu) / resample;
						}
						else if (mask[pix] == 1)
						{
							// Red pix
							pixR = (*pix8 - minadu) / resample;
							pixR = MIN(255, MAX(pixR, 0));
						}
						else if (mask[pix] == -1)
						{
							// Blue pix
							pixB = (*pix8 - minadu) / resample;
							pixB = MIN(255, MAX(pixB, 0));
						}
					}
					// From next row
					for (pix = 0; pix < 2; pix++)
					{
						pix8 = (databuffer + ((row * width * 2) + width) + col * 2) + pix;
						pixH += *pix8;
						if (mask[pix + 2] == 0)
						{
							// Green pix
							pixG  += ((*pix8 - minadu) / resample);
							pixG  /= 2;
							pixG = MIN(255, MAX(pixG, 0));
						}
						else if (mask[pix + 2] == 1)
						{
							// Red pix
							pixR = (*pix8 - minadu) / resample;
							pixR = MIN(255, MAX(pixR, 0));
						}
						else if (mask[pix + 2] == -1)
						{
							// Blue pix
							pixB = (*pix8 - minadu) / resample;
							pixB = MIN(255, MAX(pixB, 0));
						}
					}
					p[0] = pixR; //R
					p[1] = pixG; //G
					p[2] = pixB; //B
					//if ((row == 250) && (col == 250))
					//	printf("R: %d, G: %d, B: %d\n", pixR, pixG, pixB);
					hst[pixH / 4] += 1;
				}
			}
			for (col = 0; col < endw; col++)
			{
				p = pixels + col * bpp;
				p[0] = 0; //R
				p[1] = 0; //G
				p[2] = 0; //B
				p = pixels + (endh - 1) * rowstride + col * bpp;
				p[0] = 0; //R
				p[1] = 0; //G
				p[2] = 0; //B
			}
			pixbuf = gdk_pixbuf_scale_simple(pixtmp, width, height, GDK_INTERP_BILINEAR);
			g_object_unref(pixtmp);
		}
		else if (bytepix == 2)
		{
			for (row = 0; row < (endh - 1); row++)
			{
				for (col = 0; col < endw; col++)
				{
					// This is because fits image data is upside down
					p = pixels + (endh - row - 1) * rowstride + (endw - col - 1) * bpp;
					// From current fit row
					pixH = 0;
					for (pix = 0; pix < 2; pix++)
					{
						pix8 = databuffer + row * width * 4 + col * 4 + pix * 2;
						pix16 = pix8 + 1;
						pixH += (*pix8 + *pix16 * 256);
						if (mask[pix] == 0)
						{
							// Green pix
							pixG = ((*pix8 + *pix16 * 256) - minadu) / resample;
						}
						else if (mask[pix] == 1)
						{
							// Red pix
							pixR = ((*pix8 + *pix16 * 256) - minadu) / resample;
							pixR = MIN(255, MAX(pixR, 0));
						}
						else if (mask[pix] == -1)
						{
							// Blue pix
							pixB = ((*pix8 + *pix16 * 256) - minadu) / resample;
							pixB = MIN(255, MAX(pixB, 0));
						}
					}
					// From next row
					for (pix = 0; pix < 2; pix++)
					{
						pix8 = databuffer + ((row * width * 4) + (width * 2)) + col * 4 + pix * 2;
						pix16 = pix8 + 1;
						pixH += (*pix8 + *pix16 * 256);
						if (mask[pix + 2] == 0)
						{
							// Green pix
							pixG += (((*pix8 + *pix16 * 256) - minadu) / resample);
							pixG /= 2;
							pixG = MIN(255, MAX(pixG, 0));
						}
						else if (mask[pix + 2] == 1)
						{
							// Red pix
							pixR = ((*pix8 + *pix16 * 256) - minadu) / resample;
							pixR = MIN(255, MAX(pixR, 0));
						}
						else if (mask[pix + 2] == -1)
						{
							// Blue pix
							pixB = ((*pix8 + *pix16 * 256) - minadu) / resample;
							pixB = MIN(255, MAX(pixB, 0));
						}
					}
					p[0] = pixR; //R
					p[1] = pixG; //G
					p[2] = pixB; //B
					//if ((row == 800) && (col == 850))
					//	printf("R: %d, G: %d, B: %d\n", p[0], p[1], p[2]);
					hst[pixH / 1028] += 1;
				}
			}
			for (col = 0; col < endw; col++)
			{
				p = pixels + col * bpp;
				p[0] = 0; //R
				p[1] = 0; //G
				p[2] = 0; //B
				p = pixels + (endh - 1) * rowstride + col * bpp;
				p[0] = 0; //R
				p[1] = 0; //G
				p[2] = 0; //B
			}
			pixbuf = gdk_pixbuf_scale_simple(pixtmp, width, height, GDK_INTERP_BILINEAR);
			g_object_unref(pixtmp);
		}
		else
		{
			strcpy(pixmsg, C_("pixbuf","Only works with 8/16bpp images"));
			retval = 0;
		}
	}

    /* only work on focus mode */
    if (!capture) {
        if ((ret = imgpix_calc_fwhm(debayer,width/2,height/2,200,200,&fwhm,&peak)) == 0)
            sprintf(fwhmfbk, "FWHM=%d,Peak=%d,FWHM/Peak=%d",fwhm,peak,fwhm/peak);
        else
            sprintf(fwhmfbk, "imgpix_calc_fwhm got error:%d",ret);
    } else {
        fwhmfbk[0] = '\0';
    }
    gtk_label_set_text(GTK_LABEL(lbl_fbkfwhm), (gchar *) fwhmfbk);	

	if (retval)
	{
		// "Resize" to fit histogram "height"
		for (col = 0; col < 256; col++)
		{
			hst[col] = hst[col] / size * 256.;
		}
	}
	loaded = retval;
	return (retval);
}

int imgpix_calc_fwhm(int debayer, int x, int y, int width, int height, int *fwhm, int *peak)
{
	int rowstride, bpp;
	guchar *pixels, *p;
	int row, col, size = (width * height);
    int i = 0;

    if (pixbuf == NULL)
        return ERR_BUFFER_EMPTY;

    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    pixels    = gdk_pixbuf_get_pixels (pixbuf);
    bpp       = gdk_pixbuf_get_n_channels (pixbuf);
    int buf_width = gdk_pixbuf_get_width(pixbuf);
    int buf_height= gdk_pixbuf_get_height(pixbuf);

    if (width > buf_width || height > buf_height)
        return ERR_PARAM;

    if (x < width / 2)
        x = width / 2;
    if (x > buf_width - width / 2)
        x = buf_width - width / 2;
    if (y < height / 2)
        y = height / 2;
    if (y > buf_height - height / 2)
        y = buf_height - height / 2;

    unsigned char *fwhm_buf_ptr = (unsigned char *)malloc(size);
    if (fwhm_buf_ptr == NULL)
        return ERR_MALLOC;

    /* get a Luminance buffer */
    if (debayer == 0) {     //mono
        for (row = y - height / 2; row < y + height / 2; row++) {
            for (col = x - width / 2; col < x + width / 2; col++) {
                *(fwhm_buf_ptr + i++) = *(unsigned char *)(pixels + rowstride * row + col * bpp);
            }
        }
    } else {                //color
        //Luminance (standard, objective): (0.2126*R) + (0.7152*G) + (0.0722*B)
        for (row = y - height / 2; row < y + height / 2; row++) {
            for (col = x - width / 2; col < x + width / 2; col++) {
                p = pixels + rowstride * row + col * bpp;
                *(fwhm_buf_ptr + i++) = (unsigned char)(0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]);
            }
        }
    }

    /* draw a red rectangle */
    for (col = x - width / 2; col < x + width / 2; col++) {
        row = y - height / 2;
        p = pixels + rowstride * row + col * bpp;
        p[0]=255; p[1]=0; p[2]=0;
        row = y + height / 2;
        p = pixels + rowstride * row + col * bpp;
        p[0]=255; p[1]=0; p[2]=0;
    }

    for (row = y - height / 2; row < y + height / 2; row++) {
        col = x - width / 2;
        p = pixels + rowstride * row + col * bpp;
        p[0]=255; p[1]=0; p[2]=0;
        col = x + width / 2;
        p = pixels + rowstride * row + col * bpp;
        p[0]=255; p[1]=0; p[2]=0;
    }

    /* calc the peak */
    *peak = 0;
    for(i = 0; i < size; i++)
        if (fwhm_buf_ptr[i] > *peak)
            *peak = fwhm_buf_ptr[i];

    /* calc the fwhm */
    *peak /= 2;
    *fwhm = 0;
    for (i = 0; i < size; i++)
        if (fwhm_buf_ptr[i] > *peak)
            (*fwhm)++;
    *peak *= 2;

    free(fwhm_buf_ptr);
    return OK;
}
