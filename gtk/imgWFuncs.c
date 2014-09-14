/*
 * imgWFuncs.c
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
 * 
 * Code used to "chase" the brightest point in fwhm_calc is derived and adapted
 * from lin_guider project. http://sourceforge.net/projects/linguider/
 * My deepest thanks to "Galaxy Master" for sharing his code under the same 
 * license.
 * 
 */

#include "imgWindow.h"
#include "imgWFuncs.h"
#include "imgWCallbacks.h"
#include <sys/time.h>

static int intTrue  = 1;
static int intFalse = 0;

void fithdr_init(fit_rowhdr *hdr, int hdrsz)
{
	int i;
	
	// Blank all
	for (i = 0; i < hdrsz; i++)
	{
		hdr[i].name[0]    = '\0'; 
		hdr[i].dtype      = '\0'; 
		hdr[i].svalue[0]  = '\0'; 
		hdr[i].ivalue     = 0; 
		hdr[i].dvalue     = 0.; 
		hdr[i].comment[0] = '\0'; 
	}
	// Default values
	//SWCREATE
	strcpy(hdr[HDR_SWCREATE].name, "SWCREATE"); 
	strcpy(hdr[HDR_SWCREATE].svalue, APPNAM); 
	strcat(hdr[HDR_SWCREATE].svalue, " ("); 
	strcat(hdr[HDR_SWCREATE].svalue, APPVER); 
	strcat(hdr[HDR_SWCREATE].svalue, ")"); 
	hdr[HDR_SWCREATE].dtype = 'S'; 
	//INSTRUME
	strcpy(hdr[HDR_INSTRUME].name, "INSTRUME"); 
	hdr[HDR_INSTRUME].dtype = 'S'; 
	strcpy(hdr[HDR_INSTRUME].comment, "Name of instrument (camera)"); 
	//EXPTIME
	strcpy(hdr[HDR_EXPTIME].name, "EXPTIME"); 
	hdr[HDR_EXPTIME].dtype = 'F'; 
	strcpy(hdr[HDR_EXPTIME].comment, "[s] Total integration time"); 
	strcpy(hdr[HDR_EXPOSURE].name, "EXPOSURE"); 
	hdr[HDR_EXPOSURE].dtype = 'F'; 
	strcpy(hdr[HDR_EXPOSURE].comment, "[s] Total integration time"); 
	//GAIN
	strcpy(hdr[HDR_GAIN].name, "GAIN"); 
	hdr[HDR_GAIN].dtype = 'I'; 
	strcpy(hdr[HDR_GAIN].comment, "Program value (0-100)"); 
	//OFFSET
	strcpy(hdr[HDR_OFFSET].name, "OFFSET"); 
	hdr[HDR_OFFSET].dtype = 'I'; 
	strcpy(hdr[HDR_OFFSET].comment, "Program value (0-255)"); 
	//XBINNING
	strcpy(hdr[HDR_XBINNING].name, "XBINNING"); 
	hdr[HDR_XBINNING].dtype = 'I'; 
	strcpy(hdr[HDR_XBINNING].comment, "Pixel binned in X direction"); 
	//YBINNING
	strcpy(hdr[HDR_YBINNING].name, "YBINNING"); 
	hdr[HDR_YBINNING].dtype = 'I'; 
	strcpy(hdr[HDR_YBINNING].comment, "Pixel binned in Y direction"); 
	//CCD-TEMP
	strcpy(hdr[HDR_CCDTEMP].name, "CCD-TEMP"); 
	hdr[HDR_CCDTEMP].dtype = '\0'; // This will be activated if the ccd temp is read 
	strcpy(hdr[HDR_CCDTEMP].comment, "[C] Sensor temperature"); 
	//SET-TEMP
	strcpy(hdr[HDR_SETTEMP].name, "SET-TEMP"); 
	hdr[HDR_SETTEMP].dtype = '\0'; // This will be activated if the ccd temp is read 
	strcpy(hdr[HDR_SETTEMP].comment, "[C] Sensor target temperature"); 
	//PSZX
	strcpy(hdr[HDR_PSZX].name, "XPIXSZ"); 
	hdr[HDR_PSZX].dtype = '\0'; //'F'; 
	strcpy(hdr[HDR_PSZX].comment, "[um] Size of a pixel in X direction"); 
	//PSZY
	strcpy(hdr[HDR_PSZY].name, "YPIXSZ"); 
	hdr[HDR_PSZY].dtype = '\0'; //'F'; 
	strcpy(hdr[HDR_PSZY].comment, "[um] Size of a pixel in Y direction"); 
	//FILTER
	strcpy(hdr[HDR_FILTER].name, "FILTER"); 
	hdr[HDR_FILTER].dtype = 'S'; 
	strcpy(hdr[HDR_FILTER].comment, "Name of light filter used"); 
	//TELESCOP
	strcpy(hdr[HDR_TELESCOP].name, "TELESCOP"); 
	hdr[HDR_TELESCOP].dtype = '\0'; //'S'; 
	strcpy(hdr[HDR_TELESCOP].comment, "Data acquisition telescope"); 
	//FOCALLEN
	strcpy(hdr[HDR_FOCALLEN].name, "FOCALLEN"); 
	hdr[HDR_FOCALLEN].dtype = '\0'; //'I'; 
	strcpy(hdr[HDR_FOCALLEN].comment, "[mm] Telescope focal length"); 
	//APTDIA
	strcpy(hdr[HDR_APTDIA].name, "APTDIA"); 
	hdr[HDR_APTDIA].dtype = '\0'; //'I'; 
	strcpy(hdr[HDR_APTDIA].comment, "[mm] Telescope aperture diameter"); 
	//IPANGX
	strcpy(hdr[HDR_IPANGX].name, "IPANGX"); 
	hdr[HDR_IPANGX].dtype = '\0'; //'F'; 
	strcpy(hdr[HDR_IPANGX].comment, "[arcsec/pix] Sky sampling rate X"); 
	//IPANGY
	strcpy(hdr[HDR_IPANGY].name, "IPANGY"); 
	hdr[HDR_IPANGY].dtype = '\0'; //'F'; 
	strcpy(hdr[HDR_IPANGY].comment, "[arcsec/pix] Sky sampling rate Y"); 
	//IMAGETYP
	strcpy(hdr[HDR_IMAGETYP].name, "IMAGETYP"); 
	hdr[HDR_IMAGETYP].dtype = '\0'; //'S'; 
	strcpy(hdr[HDR_IMAGETYP].comment, "[L|B|D|F] Type of image"); 
	//FRAMENO
	strcpy(hdr[HDR_FRAMENO].name, "FRAMENO"); 
	hdr[HDR_FRAMENO].dtype = 'I'; 
	strcpy(hdr[HDR_FRAMENO].comment, "Frame sequence number"); 
	//SITELAT
	strcpy(hdr[HDR_SITELAT].name, "SITELAT"); 
	hdr[HDR_SITELAT].dtype = '\0'; //'S'; 
	strcpy(hdr[HDR_SITELAT].comment, "Site latitude"); 
	//SITELONG
	strcpy(hdr[HDR_SITELONG].name, "SITELONG"); 
	hdr[HDR_SITELONG].dtype = '\0'; //'S'; 
	strcpy(hdr[HDR_SITELONG].comment, "Site longitude"); 
	//DATE-OBS
	strcpy(hdr[HDR_DATEOBS].name, "DATE-OBS"); 
	hdr[HDR_DATEOBS].dtype = 'S'; 
	strcpy(hdr[HDR_DATEOBS].comment, "[UTC] Date/Time at the start of the exposure"); 
	//DATE-OBS
	strcpy(hdr[HDR_DATE].name, "DATE"); 
	hdr[HDR_DATE].dtype = 'D'; 
	//OBSERVER
	strcpy(hdr[HDR_OBSERVER].name, "OBSERVER"); 
	if (getusername() != NULL)
	{
		hdr[HDR_OBSERVER].dtype = 'S'; 
		strcpy(hdr[HDR_OBSERVER].svalue, getusername());
	}
	else
	{
		hdr[HDR_OBSERVER].dtype = '\0'; 
	}
	strcpy(hdr[HDR_OBSERVER].comment, "Observer name"); 
	//OBJECT
	strcpy(hdr[HDR_OBJECT].name, "OBJECT"); 
	hdr[HDR_OBJECT].dtype = '\0'; //'S'; 
	strcpy(hdr[HDR_OBJECT].comment, "Object name"); 
}

void get_filename(char **filename, int mode, char* flt)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(C_("dialog-open","Open File"), (GtkWindow *) window, ((mode == 1) ? GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN), "_Cancel", GTK_RESPONSE_CANCEL, ((mode == 1) ? "_Save" : "_Open"), GTK_RESPONSE_ACCEPT, NULL);
	GtkFileFilter *filter = gtk_file_filter_new();

	if (flt != NULL)
	{
		gtk_file_filter_add_pattern(filter, flt);
		gtk_file_filter_set_name(filter, flt);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{

		*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}
	
	gtk_widget_destroy(dialog);
}

void set_img_fit()
{
	int tgtw, tgth;
	double wratio, hratio;

	// Thread safe read
	g_rw_lock_reader_lock(&pixbuf_lock);
	if (imgpix_loaded())
	{
		// Get allocated size
		GtkAllocation *alloc = g_new0 (GtkAllocation, 1);
		gtk_widget_get_allocation(GTK_WIDGET(swindow), alloc);
		tgtw = alloc->width;
		tgth = alloc->height;
		//Take scrollbar width into account
		gtk_widget_get_allocation (GTK_WIDGET(gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(swindow))), alloc);
		tgtw -= alloc->width; //+ 5;
		gtk_widget_get_allocation (GTK_WIDGET(gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(swindow))), alloc);
		tgth -= alloc->height; //+ 10;
		// Cleanup
		g_free(alloc);

		// Resize ratios
		wratio = ((double) imgpix_get_width() / tgtw);
		hratio = ((double) imgpix_get_height() / tgth);
		//printf("w: %d, h: %d, wr: %f, hr: %f\n", tgtw, tgth, wratio, hratio);
		// Choose final size
		if (wratio > hratio)
		{
			imgratio = wratio;
			tgth = imgpix_get_height() / wratio;
		}
		else
		{
			imgratio = hratio;
			tgtw = imgpix_get_width() / hratio;
		}	
		//printf("w: %d, h: %d\n", tgtw, tgth);
		// Actual resize
		GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(imgpix_get_data(), tgtw, tgth, ((imgratio < 1) ? GDK_INTERP_TILES : GDK_INTERP_HYPER));
		gtk_image_set_from_pixbuf((GtkImage *) image, tmpbuf);
		g_object_unref(tmpbuf);
		
		if (fwhmv == 1)
		{
			fwhm_show();
		}
	}
	fit = 1;
	g_rw_lock_reader_unlock(&pixbuf_lock);
}

void set_img_full()
{
	// Thread safe read
	g_rw_lock_reader_lock(&pixbuf_lock);
	if (imgpix_loaded())
	{
		gtk_image_set_from_pixbuf((GtkImage *) image, imgpix_get_data());
	}
	fit = 0;
	imgratio = 1;

	if (fwhmv == 1)
	{
		fwhm_show();
	}
	g_rw_lock_reader_unlock(&pixbuf_lock);
}

void set_adu_limits(int bytepix)
{
	if (uibytepix != bytepix)
	{
		if (bytepix == 2)
		{
			scrmaxadu = gtk_range_get_value(GTK_RANGE(hsc_maxadu)) * 257;
			scrminadu = gtk_range_get_value(GTK_RANGE(hsc_minadu)) * 257;
			gtk_range_set_range(GTK_RANGE(hsc_maxadu), 0.0, 65535.0);
			gtk_range_set_range(GTK_RANGE(hsc_minadu), 0.0, 65534.0);
		}
		else
		{
			scrmaxadu = gtk_range_get_value(GTK_RANGE(hsc_maxadu)) / 257;
			scrminadu = gtk_range_get_value(GTK_RANGE(hsc_minadu)) / 257;
			gtk_range_set_range(GTK_RANGE(hsc_maxadu), 0.0, 255.0);
			gtk_range_set_range(GTK_RANGE(hsc_minadu), 0.0, 254.0);
		}
		gtk_range_set_value(GTK_RANGE(hsc_maxadu), scrmaxadu);
		gtk_range_set_value(GTK_RANGE(hsc_minadu), scrminadu);
		uibytepix = bytepix;
	}
}

void load_image_from_data()
{
	int retval = 0;
	
	g_rw_lock_reader_lock(&thd_caplock);
	if (imgfit_loaded())
	{
		// Set hourglass
		//gdk_window_set_cursor(GDK_WINDOW(window->window), watchCursor);
		// Ui update after byterpix
		if (tmrimgrefresh != -1)
		{
			g_source_remove(tmrimgrefresh);
		}
		if (tmrfrmrefresh != -1)
		{
			g_source_remove(tmrfrmrefresh);
		}
		set_adu_limits(imgfit_get_bytepix());
		
		int debayer = gtk_combo_box_get_active(GTK_COMBO_BOX(cmb_debayer));
		if (imgfit_internal() == 0)
		{
			// No debayer for captured frames if bin > 1
			debayer = (imgcam_get_shpar()->bin == 1) ? debayer : 0;
		}
		if (fwhmv == 1)
		{
			fwhm_calc();
		}
		
		// Actual pixbuffer load (thread safe)
		g_rw_lock_writer_lock(&pixbuf_lock);
		retval = imgpix_load(imgfit_get_data(), imgfit_get_width(), imgfit_get_height(), imgfit_get_bytepix(), debayer, scrmaxadu, scrminadu);
		g_rw_lock_writer_unlock(&pixbuf_lock);
				
		if ((fifomode) && (retval))
		{
			printf("Fifo: PREVIEW=New preview image available\n");
		}

		if (retval == 1)
		{	
			tmrfrmrefresh = g_timeout_add(1, (GSourceFunc) tmr_frm_refresh, NULL);
		}
		else if (strlen(imgpix_get_msg()) != 0)
		{
			tmrfrmrefresh = g_timeout_add(1, (GSourceFunc) tmr_imgstatus_pixmsg, NULL);			
		}
		// Reset
		//gdk_window_set_cursor(GDK_WINDOW(window->window), NULL);
	}
	g_rw_lock_reader_unlock(&thd_caplock);
}

void load_histogram_from_data()
{
	int tgtw, tgth;

	// Thread safe read
	g_rw_lock_reader_lock(&pixbuf_lock);
	if (imgpix_loaded())
	{
		// Get allocated size
		GtkAllocation *alloc = g_new0 (GtkAllocation, 1);
		gtk_widget_get_allocation(GTK_WIDGET(frm_histogram), alloc);
		tgtw = alloc->width -5;
		tgth = alloc->height -5;
		// Cleanup
		g_free(alloc);

		// Actual resize
		GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(imgpix_get_histogram(gtk_spin_button_get_value(GTK_SPIN_BUTTON(spn_histogram)) * 10), tgtw, tgth, GDK_INTERP_HYPER);
		gtk_image_set_from_pixbuf((GtkImage *) histogram, tmpbuf);
		g_object_unref(tmpbuf);
	}
	g_rw_lock_reader_unlock(&pixbuf_lock);
}

void load_histogram_from_null()
{
	int tgtw, tgth;
	double wratio, hratio;

	// Thread safe read
	g_rw_lock_reader_lock(&pixbuf_lock);
	if (imgpix_loaded())
	{
		// Get allocated size
		GtkAllocation *alloc = g_new0 (GtkAllocation, 1);
		gtk_widget_get_allocation(GTK_WIDGET(frm_histogram), alloc);
		tgtw = alloc->width -5;
		tgth = alloc->height -5;
		// Cleanup
		g_free(alloc);
		if (fwhmv == 1)
		{
			// Resize ratios
			wratio = ((double) fwhms / tgtw);
			hratio = ((double) fwhms / tgth);
			// Choose final size
			if (wratio > hratio)
			{
				icoratio = wratio;
				tgth = fwhms / wratio;
			}
			else
			{
				icoratio = hratio;
				tgtw = fwhms / hratio;
			}
			// Actual resize
			imgpix_init_histogram();
			GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(imgpix_get_roi_data(fwhmx, fwhmy, fwhms), tgtw, tgth, GDK_INTERP_HYPER);
			gtk_image_set_from_pixbuf((GtkImage *) histogram, tmpbuf);
			g_object_unref(tmpbuf);
		}
		else
		{
			// Resize ratios
			wratio = ((double) imgpix_get_width() / tgtw);
			hratio = ((double) imgpix_get_height() / tgth);
			// Choose final size
			if (wratio > hratio)
			{
				icoratio = wratio;
				tgth = imgpix_get_height() / wratio;
			}
			else
			{
				icoratio = hratio;
				tgtw = imgpix_get_width() / hratio;
			}
			// Actual resize
			imgpix_init_histogram();
			GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(imgpix_get_data(), tgtw, tgth, ((imgratio < 1) ? GDK_INTERP_TILES : GDK_INTERP_HYPER));
			gtk_image_set_from_pixbuf((GtkImage *) histogram, tmpbuf);
			g_object_unref(tmpbuf);
		}
	}
	g_rw_lock_reader_unlock(&pixbuf_lock);
}

void fwhm_center(int setx, int sety, int abspos)
{
	int roix, roiy;
	double imrat = ((abspos == 0) ? imgratio : 1);
	int roisize = fwhms / imrat;
	int width = (imgpix_get_width() / imrat), height = (imgpix_get_height() / imrat);

	// Center on image data regardless of "fit to screen"
	fwhmx = setx * imrat;
	fwhmy = sety * imrat;

	// Roi position depending on "fit to screen"
	roix = ((fwhmx - (fwhms / 2)) / imrat);
	roiy = ((fwhmy - (fwhms / 2)) / imrat);

	// check ROI is fully inside frame and fix if needed
	// Move centroid position relative to ROI accordingly
	if( roix < (roisize / 2) )
	{
		roix = (roisize / 2);
	}
	if( roiy < (roisize / 2) )
	{
		roiy = (roisize / 2);
	}
	if( roix + roisize + (roisize / 2) > width )
	{
		roix = width - roisize - (roisize / 2) - 1;
	}
	if( roiy + roisize + (roisize / 2) > height )
	{
		roiy = height - roisize - (roisize / 2) - 1;
	}	

	// Center on image data regardless of "fit to screen"
	fwhmx = (roix * imrat) + (fwhms / 2);
	fwhmy = (roiy * imrat) + (fwhms / 2);
}

void fwhm_show()
{
	int roisize = fwhms / imgratio;
	int roix = ((fwhmx - (fwhms / 2)) / imgratio), roiy = ((fwhmy - (fwhms / 2)) / imgratio);
	int lblx = roix, lbly = roiy;
	int width = (imgpix_get_width() / imgratio), height = (imgpix_get_height() / imgratio);
	GdkPixbuf *pixbuf;

	// check ROI is fully inside frame and fix if needed
	// Move centroid position relative to ROI accordingly
	if( roix < (roisize / 2) )
	{
		roix = (roisize / 2);
	}
	if( roiy < (roisize / 2) )
	{
		roiy = (roisize / 2);
	}
	if( roix + roisize + (roisize / 2) > width )
	{
		roix = width - roisize - (roisize / 2) - 1;
	}
	if( roiy + fwhms + (roisize / 2) > height )
	{
		roiy = height - roisize - (roisize / 2) - 1;
	}
	lblx = roix;
	lbly = roiy;

	// Center on image data regardless of "fit to screen"
	fwhmx = (roix * imgratio) + (fwhms / 2);
	fwhmy = (roiy * imgratio) + (fwhms / 2);

	// Set flag "is visible"	
	fwhmv = 1;

	pixbuf = imgpix_get_roi_square(roisize);
	gtk_image_set_from_pixbuf((GtkImage *) fwhmroi, pixbuf);
	g_object_unref(pixbuf);
	gtk_fixed_move(GTK_FIXED(fixed), fwhmroi, roix, roiy);
	
	// Write label
	//sprintf(fwhmfbk, "FWHM=%05.2F, Peak=%d, FWHM/Peak=%05.2F", afwhm, pfwhm, (afwhm / (double)pfwhm));
	sprintf(fwhmfbk, "HFD=%05.2F, Peak=%d", afwhm, pfwhm);
	gtk_label_set_text(GTK_LABEL(lbl_fbkfwhm), (gchar *) fwhmfbk);
	
	// Move Label
	if ((roiy - fwhmlblh - 10) > 0)
	{
		lbly = (roiy - fwhmlblh - 10);
	}
	else
	{
		lbly = (roiy + roisize + 10);
	}
	if ((roix + fwhmlblw) > width)
	{
		lblx = (roix - fwhmlblw + roisize);
	}
	gtk_fixed_move(GTK_FIXED(fixed), lbl_fbkfwhm, lblx, lbly);
}

void fwhm_hide()
{
	// Set flag "is visible"	
	fwhmv = 0;

	// Clear Roi
	gtk_image_clear(GTK_IMAGE(fwhmroi));
	
	// Clear label
	fwhmfbk[0] = '\0';
	gtk_label_set_text(GTK_LABEL(lbl_fbkfwhm), (gchar *) fwhmfbk);
	gtk_fixed_move(GTK_FIXED(fixed), lbl_fbkfwhm, 0, 0);

}

void fwhm_calc()
{
	int i, j, k;
	int roix = (fwhmx - (fwhms / 2)), roiy = (fwhmy - (fwhms / 2));
	int width = imgfit_get_width(), height = imgfit_get_height(), bytepix = imgfit_get_bytepix();
	int resx, resy, prex = roix, prey = roiy, mass, threshold, pval, resp;
	//double vfwhm, ofwhm;
	double flux;
	int roi[fwhmp];
	unsigned char *porigin = NULL, *psrc = NULL;

	// check ROI is fully inside frame and fix if needed
	if( roix < (fwhms / 2) )
	{
		roix = (fwhms / 2);
	}
	if( roiy < (fwhms / 2) )
	{
		roiy = (fwhms / 2);
	}
	if( roix + fwhms + (fwhms / 2) > width )
	{
		roix = width - fwhms - (fwhms / 2) - 1;
	}
	if( roiy + fwhms + (fwhms / 2) > height )
	{
		roiy = height - fwhms - (fwhms / 2) - 1;
	}	

	for ( k = 0; k < 10; k++ )
	{
		// This brings us to the first byte of the pixel that is ROI(0,0)
		// Beware! Screen image is upside down and left right reversed
		porigin = imgfit_get_data() + ((((height- roiy - 1) * width) + (width - roix - 1)) * bytepix);

		// Read 
		if (bytepix == 1)
		{
			for( j = 0; j < fwhms; j++ )
			{
				// databuffer pointer need to move on full frame width
				psrc = porigin - (j * width);
				for( i = 0; i < fwhms; i++ )
				{
					// ROI element is relative to ROI size
					roi[((j * fwhms) + i)] = psrc[0];
					psrc--;
				}
			}
		}
		else if (bytepix == 2)
		{
			for( j = 0; j < fwhms; j++ )
			{
				// databuffer pointer need to move on full frame width
				psrc = porigin - (j * width * bytepix);
				for( i = 0; i < fwhms; i++ )
				{
					// ROI element is relative to ROI size
					roi[((j * fwhms) + i)] = psrc[0] + psrc[1] * 256;
					psrc -= bytepix;
				}
			}
		}

		// Find threshold value
		threshold = 0;
		for( i = 0; i < fwhmp; i++ )
		{
			threshold += roi[i];
		}
		threshold /= fwhmp;

		resx = 0;
		resy = 0;
		mass = 0;
		// Calculating offset in ROI for the maximum
		for( j = 0; j < fwhms; j++ )
		{
			for( i = 0; i < fwhms; i++ )
			{
				pval = roi[((j * fwhms) + i)] - threshold;
				pval = pval < 0 ? 0 : pval;

				resx += i * pval;
				resy += j * pval;

				mass += pval;
			}
		}

		mass = (mass == 0) ? 1. : mass;

		// Result coordinates of the centroid, relative to ROI 
		resx /= mass;
		resy /= mass;

		// Pixel value of the centroid
		resp = roi[((resy * fwhms) + resx)];

		// Move ROI to have star_pos in the center (if possibile)
		roix = roix + (resx - (fwhms / 2));
		roiy = roiy + (resy - (fwhms / 2));
		
		//Result coordinates of the centroid, relative to full image
		fwhmx = roix + (fwhms / 2);
		fwhmy = roiy + (fwhms / 2);
	
		// check ROI is fully inside frame and fix if needed
		// Move centroid position relative to ROI accordingly
		if( roix < (fwhms / 2) )
		{
			resx += (fwhms / 2) - roix;
			roix = (fwhms / 2);
		}
		if( roiy < (fwhms / 2) )
		{
			resy += (fwhms / 2) - roiy;
			roiy = (fwhms / 2);
		}
		if( roix + fwhms + (fwhms / 2) > width )
		{
			resx -= (roix + fwhms + (fwhms / 2) - width);
			roix = width - fwhms - (fwhms / 2) - 1;
		}
		if( roiy + fwhms + (fwhms / 2) > height )
		{
			resy -= (roiy + fwhms + (fwhms / 2) - height);
			roiy = height - fwhms - (fwhms / 2) - 1;
		}	

		if ((prex == roix) && (prey == roiy))
		{
			break;
		}
		prex = roix;
		prey = roiy;
	}

	// This brings us to the first byte of the pixel that is ROI(0,0)
	porigin = imgfit_get_data() + ((((height- roiy - 1) * width) + (width - roix - 1)) * bytepix);

	// Now read ROI pixels again and calculate fwhm
	// Read 
	if (bytepix == 1)
	{
		for( j = 0; j < fwhms; j++ )
		{
			// databuffer pointer need to move on full frame width
			psrc = porigin - (j * width);
			for( i = 0; i < fwhms; i++ )
			{
				// ROI element is relative to ROI size
				roi[((j * fwhms) + i)] = psrc[0];
				psrc--;
			}
		}
	}
	else if (bytepix == 2)
	{
		for( j = 0; j < fwhms; j++ )
		{
			// databuffer pointer need to move on full frame width
			psrc = porigin - (j * width * bytepix);
			for( i = 0; i < fwhms; i++ )
			{
				// ROI element is relative to ROI size
				roi[((j * fwhms) + i)] = psrc[0] + psrc[1] * 256;
				psrc -= bytepix;
			}
		}
	}

	// Half flux diameter (http://www005.upp.so-net.ne.jp/k_miyash/occ02/halffluxdiameter/halffluxdiameter_en.html)
	// First subtract the average and clamp to 0 (to mimic subtract bck noise)
	// Then calculate the whole mass (Sum Vi)
	// Then calculate sum Vi * Di
	mass = 0;
	flux = 0.;
	for( j = 0; j < fwhms; j++ )
	{
		for( i = 0; i < fwhms; i++ )
		{
			pval = roi[((j * fwhms) + i)] - threshold;
			pval = pval < 0 ? 0 : pval;
			mass += pval;
			// Distance from centroid * pixel value
			flux += sqrt(pow(abs(resy - j), 2) + pow(abs(resx - i), 2)) * pval;
			//printf ("pval: %d, resx: %d, resy: %d, x: %d, y: %d, resx-x^2: %f, resy-y^2 %f, dist: %f\n;", pval, resx, resy, i, j, pow(abs(resx - i), 2), pow(abs(resy - j), 2), sqrt(pow(abs(resy - j), 2) + pow(abs(resx - i), 2)));
		}
	}
	// HFD
	afwhm = (flux / mass) * 2;
	
	/*
	// Vertical fwhm
	threshold = resp / 2; 
	vfwhm = 0;
	for( j = resy; j < fwhms; j++ )
	{
		if (roi[((j * fwhms) + resx)] < threshold)
		{
			vfwhm *= 2.;
			break;
		}
		else
		{
			vfwhm++;
		}
	}

	// Horizontal fwhm
	ofwhm = 0;
	for( i = resx; i < fwhms; i++ )
	{
		if (roi[((resy * fwhms) + i)] < threshold)
		{
			ofwhm *= 2.;
			break;
		}
		else
		{
			ofwhm++;
		}
	}

	afwhm = (vfwhm + ofwhm) / 2.;
	*/
	pfwhm = resp;
}

void tec_init_graph()
{
	int rowstride;
	int row, col, rows, cols;
	guchar *pixels, *p;
	
	rowstride = gdk_pixbuf_get_rowstride (tecpixbuf);
	pixels    = gdk_pixbuf_get_pixels (tecpixbuf);
	rows      = gdk_pixbuf_get_height(tecpixbuf);
	cols      = gdk_pixbuf_get_width(tecpixbuf);
	
	for (row = 0; row < rows; row++)
	{
		for (col = 0; col < cols; col++)
		{
			p = pixels + row * rowstride + col * 3;
			p[0] = 200;
			p[1] = 200;
			p[2] = 200;
		}
	}
	tec_show_graph();
}

void tec_print_graph()
{
	int rowstride;
	int row, col, rows, cols;
	guchar *pixels, *p;
	
	rowstride = gdk_pixbuf_get_rowstride (tecpixbuf);
	pixels    = gdk_pixbuf_get_pixels (tecpixbuf);
	rows      = gdk_pixbuf_get_height(tecpixbuf);
	cols      = gdk_pixbuf_get_width(tecpixbuf);

	// Shift pixels left one col
	for (row = 0; row < rows; row++)
	{
		for (col = 0; col < cols; col++)
		{
			p = pixels + row * rowstride + col * 3;
			p[0] = p[3];
			p[1] = p[4];
			p[2] = p[5];
		}
	}
	for (row = 0; row < rows; row++)
	{
		p = pixels + row * rowstride + (cols - 1) * 3;
		p[0] = (imgcam_get_tecp()->tectemp <= (-row + 30)) ? 200 : 130;
		p[1] = (imgcam_get_tecp()->tectemp <= (-row + 30)) ? 200 : 160;
		p[2] = 200;
	}	
	tec_show_graph();
}

void tec_show_graph()
{
	int tgtw, tgth;

	// Get allocated size
	GtkAllocation *alloc = g_new0 (GtkAllocation, 1);
	gtk_widget_get_allocation(GTK_WIDGET(frm_tecgraph), alloc);
	tgtw = (alloc->width > 5) ? alloc->width - 5 : alloc->width;
	tgth = (alloc->height > 5) ? alloc->height - 5 : alloc->height;
	// Cleanup
	g_free(alloc);

	GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(tecpixbuf, tgtw, tgth, GDK_INTERP_HYPER);
	gtk_image_set_from_pixbuf((GtkImage *) tecgraph, tmpbuf);
	g_object_unref(tmpbuf);
}

void combo_setlist(GtkWidget *cmb, char *str)
{	
	char tmp[256];
	
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(cmb)) != -1)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), 0);
	}
	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cmb))));
	
	if (strchr(str, '|') != NULL)
	{
		char *pch;
		char *saveptr;
		int val4 = 0;
		if (strchr(str, ':') != NULL)
		{
			//Format val1|val2|...:active
			sscanf(str, "%[^:]:%d", tmp, &val4);
			pch = strtok_r(tmp, "|", &saveptr);
			while (pch != NULL)
			{
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb), pch);
				pch = strtok_r(NULL, "|", &saveptr);
			}
			gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), val4);
		}
		else
		{
			// Format val1|val2|...
			pch = strtok_r(str, "|", &saveptr);
			while (pch != NULL)
			{
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb), pch);
				pch = strtok_r(NULL, "|", &saveptr);
			}
			gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), 0);
		}
	}
	else if (strchr(str, ':') != NULL)
	{
		int val1 = 0, val2 = 0, val3 = 0,val4 = 0, i;
		//Format start:end:step:active
		sscanf(str, "%d:%d:%d:%d", &val1, &val2, &val3, &val4);
		for (i = val1; i <= val2; i+=val3) 
		{
			sprintf(tmp, "%d", i);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb), tmp);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), val4);
	}
}

void combo_getlist(GtkWidget *cmb, char *str)
{	
	GtkListStore *list_store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cmb)));
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *tmpstr;
	int i, count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL);

	str[0] = '\0';
	path = gtk_tree_path_new_first();
	gtk_tree_model_get_iter(GTK_TREE_MODEL(list_store), &iter, path);
	gtk_tree_path_free (path);
	for (i = 0; i < count; i++)
	{
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 0, &tmpstr, -1);
		sprintf(str,"%s|%s", str, tmpstr);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
	g_free(tmpstr);
	if (strlen(str) > 0)
	{
		memmove(str,str+1, strlen(str)-1);
		str[strlen(str)-1] = '\0';
	}
}

void combo_ttylist(GtkWidget *cmb)
{
	char ttylist[2048];

	getComList(ttylist);
	combo_setlist(cmb, ttylist);
}

void cfwmsgdestroy(int response)
{
	int i;
	
	gtk_dialog_response(GTK_DIALOG(cfwmsg), GTK_RESPONSE_NONE);
	gtk_widget_destroy(cfwmsg);
	
	if (imgcfw_get_mode() == 99)
	{
		// In this case the tec thread was set to pause,
		// we need to re-enable it
		g_rw_lock_reader_unlock(&thd_teclock);
	}	
	
	//printf("Response = %d\n", response);
	if (response == 1)
	{
		// All ok, select the filter name for the naming convention (if config is complete and valid)
		//printf("Combo elements %d, slots %d\n", gtk_combo_box_element_count(cmb_flt), imgcfw_get_slotcount());
		if (gtk_combo_box_element_count(cmb_flt) == imgcfw_get_slotcount())
		{
			gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_flt), imgcfw_get_slot());
		}
	}
	else
	{
		sprintf(imgmsg, C_("cfw","Filter wheel reported error"));
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
	// Enable active slot buttons
	for (i = 0; i < CFW_SLOTS; i++)
	{
		gtk_widget_set_sensitive(cmb_cfwwhl[i], (i < imgcfw_get_slotcount()));
		gtk_widget_set_sensitive(cmd_cfwwhl[i], (i < imgcfw_get_slotcount()));
	}
}

void filenaming(char *thdfit)
{
	struct tm now;
	time_t localt;
	char thdtdmark[32];
	char thdsuffix[32];
	
	// Filename calculations 
	// tab deactivated in capture mode, no need to lock
	if ((fitdateadd == 1) || (fittimeadd == 1))
	{
		strcpy(thdfit, g_build_path(G_DIR_SEPARATOR_S, fitfolder, fitbase, NULL));
		localt = time(NULL);
		now    = *(localtime(&localt));
		if (fitdateadd == 1)
		{
			if (strftime(thdtdmark, 32, "_%Y%m%d" ,&now) > 0)
			{
				strcat(thdfit, thdtdmark);
			}
		}
		if (fittimeadd == 1)
		{
			if (strftime(thdtdmark, 32, "_%H%M%S" ,&now) > 0)
			{
				strcat(thdfit, thdtdmark);
			}
		}
		if (strlen(fitflt) > 0)
		{
			sprintf(thdsuffix, "_%s", fitflt);
			strcat(thdfit, thdsuffix);
		}
	}
	else if (audelanaming == 1)
	{
		localt = time(NULL);
		now    = *(localtime(&localt));
		if (strftime(thdtdmark, 32, "_%Y-%m-%d" ,&now) > 0)
		{
			strcpy(thdfit, g_build_path(G_DIR_SEPARATOR_S, fitfolder, thdtdmark+1, G_DIR_SEPARATOR_S, NULL));
			// Check if basefolder exists
			if(isdir(thdfit) == 0)
			{
				// Folders does not exist, create it
				mkpath(thdfit, 0);
			}
			// Base folder/ + date/ + base file name
			strcat(thdfit, fitbase);
			strcat(thdfit, "_");
			strcat(thdfit, imgcam_get_model());
			if (strlen(fitflt) > 0)
			{
				sprintf(thdsuffix, "_%s", fitflt);
				strcat(thdfit, thdsuffix);
			}
			strcat(thdfit, thdtdmark);
			if (strftime(thdtdmark, 32, "_%H-%M-%S" ,&now) > 0)
			{
				strcat(thdfit, thdtdmark);
			}
		}
	}
	else
	{
		strcpy(thdfit, g_build_path(G_DIR_SEPARATOR_S, fitfolder, fitbase, NULL));
		if (strlen(fitflt) > 0)
		{
			sprintf(thdsuffix, "_%s", fitflt);
			strcat(thdfit, thdsuffix);
		}
	}
}

void shotsnaming(char *thdfit, int thdshots)
{
	char thdsuffix[32];

	if (irisnaming == 1)
	{
		sprintf(thdsuffix, "_%d", thdshots);
	}
	else	
	{						
		sprintf(thdsuffix, "_%04d", thdshots);
	}
	strcat(thdfit, thdsuffix);
	fithdr[HDR_FRAMENO].ivalue = thdshots;
}

int wrtavihdr(char *filename, fit_rowhdr *hdr, int hdrsz)
{
	int i, retval = 1;
	FILE *pfile = NULL;
	char row[256], num[32];
	
	if (strstr(filename, ".avi") != NULL)
	{
		filename[strcspn(filename, ".avi")] = '\0';
	}
	strcat(filename, ".txt");
	
	// Folder structure is already there due to filenaming
	if ((retval = ((pfile = fopen(filename, "w")) != NULL)) == 1)
	{
		if (hdr != NULL)
		{
			for (i = 0; i < hdrsz; i++)
			{
				if ((hdr[i].dtype != '\0') && (hdr[i].dtype != 'D'))
				{
					// If hdr[i].dtype == '\0' key is ignored
					strcpy(row, hdr[i].name);
					strcat(row, "=");
					switch ((int)hdr[i].dtype)
					{
						case 'S':
							strcat(row, "'");
							strcat(row, hdr[i].svalue);
							strcat(row, "'");
							if (strlen(hdr[i].comment) > 0)
							{
								strcat(row, " /");
								strcat(row, hdr[i].comment);
							}
							break;
						case 'I':
							sprintf(num, "%d", hdr[i].ivalue);
							strcat(row, num);
							if (strlen(hdr[i].comment) > 0)
							{
								strcat(row, " /");
								strcat(row, hdr[i].comment);
							}
							break;
						case 'F':
							sprintf(num, "%1.3f", hdr[i].dvalue);
							strcat(row, num);
							if (strlen(hdr[i].comment) > 0)
							{
								strcat(row, " /");
								strcat(row, hdr[i].comment);
							}
							break;
						case 'D':
							// Date is a nonsense here
							break;
						case 'B':
							if (hdr[i].ivalue == 0)
							{
								strcat(row, "F");
							}
							else
							{
								strcat(row, "T");
							}
							if (strlen(hdr[i].comment) > 0)
							{
								strcat(row, " /");
								strcat(row, hdr[i].comment);
							}
							break;
					}
					strcat(row, "\n");
					// Write into file
					if ((retval = (fputs(row, pfile) != EOF)) == 0)
					{
						// On error exit loop
						break;
					}
				}
			}
		}
		retval = (fclose(pfile) != EOF);
	}
	return (retval);	
}

gpointer thd_capture_run(gpointer thd_data)
{
	int thdrun = 1, thderror = 0, thdhold = 0, thdmode = 0, thdshoot = 0;
	int thdpreshots = shots, thdexp = 0, thdtlmode = 0, thdsavejpg = 0;
	int thdtimer = 0, thdtimeradd = 0;
	int avimaxframes = 0, writeavih = 0;
	int thdreadok = 1;
	char thdfit[2048];
	char thdtdmark[32];
	time_t ref, last;
	struct tm now;
	struct timeval clks, clke;
	struct timeval clkws, clkwe;
	GThread *thd_pixbuf = NULL;
	GThread *thd_fitsav = NULL;
	GThread *thd_avisav = NULL;

	gettimeofday(&clks, NULL);
	g_rw_lock_reader_lock(&thd_caplock);
	thdmode = capture;
	thdsavejpg = savejpg;
	thdrun = run;
	thdtlmode = tlenable + tlcalendar;
	if ((thdtlmode == 2) && (thdrun == 1))
	{
		// In FULL TimeLapse mode only start / end / interval count
		ref = time(NULL);
		thdrun = ((thdrun == 1) && (difftime(mktime(&tlend), ref) > 0));
	}
	else if ((thdmode == 1) && (thdrun == 1))
	{
		// In capture mode also total shots count
		thdrun = ((thdrun == 1) && (expnum > (shots - thdpreshots)));
	}
	g_rw_lock_reader_unlock(&thd_caplock);		

	//No matter what, avi is init just in case (needed only once)
	imgavi_init();

	// If we are in FULL timelapse mode
	if ((thdtlmode == 2) && (thdrun == 1))
	{
		ref = time(NULL);
		// Wait for the start time to arrive
		while (difftime(mktime(&tlstart), ref) > 0)
		{
			// We yeld to other threads until 500ms are passed by
			g_usleep(500000);
			// Check if we're still in run condition
			g_rw_lock_reader_lock(&thd_caplock);
			thdrun = run;
			thdmode = capture;
			thdsavejpg = savejpg;
			g_rw_lock_reader_unlock(&thd_caplock);		
			if (thdrun == 0)
			{
				// User abort
				break;
			}
			ref = time(NULL);
		}
	}
	if ((imgcam_get_camui()->shutterMode == 1) && imgcam_get_camid() < 1000)
	{
		// For QHY only
		// We close shutter just in case it's open because of camera position
		// It's a noop for camera that don't feature a mechanical shutter
		imgcam_shutter(1);
		last = time(NULL);
	}
	// First time read, just in case.
	if ((tecrun == 1) && (imgcam_get_tecp()->istec == 2) )
	{
		// Camera only allow temp read with no concurrent access
		imgcam_gettec(&imgcam_get_tecp()->tectemp, NULL, NULL, NULL); 			
		// UI update
		if (tmrtecrefresh != -1)
		{
			g_source_remove(tmrtecrefresh);
		}
		tmrtecrefresh = g_timeout_add(1, (GSourceFunc) tmr_tecstatus_write, NULL);			
	}
	while ((thdrun == 1) && (thderror == 0))
	{
		// To ensure the "sh(oot)" copy of the ex(posure) params is done clean
		if ((tecrun == 1) && (imgcam_get_tecp()->istec == 2))
		{
			ref = time(NULL);
			if (difftime(ref, last) > 2)
			{
				// Camera only allow temp read with no concurrent access
				// Not more that once every 3 seconds, similar to tec reading threads for other models
				imgcam_gettec(&imgcam_get_tecp()->tectemp, NULL, NULL, NULL); 			
				// UI update
				if (tmrtecrefresh != -1)
				{
					g_source_remove(tmrtecrefresh);
				}
				tmrtecrefresh = g_timeout_add(1, (GSourceFunc) tmr_tecstatus_write, NULL);			
				// Reference update
				last = time(NULL);
			}
		}
		g_rw_lock_writer_lock(&thd_caplock);
		// Fit header date-obs (date of exposure start)
		ref  = time(NULL);
		now  = *(gmtime(&ref));
		if (strftime(thdtdmark, 32, "%FT%T" ,&now) > 0)
		{
			strcpy(fithdr[HDR_DATEOBS].svalue , thdtdmark);
		}
		// Shoot
		writeavih = imgcam_get_expar()->edit;
		// We also need to pause the tec thread
		g_rw_lock_reader_lock(&thd_teclock);
		thdshoot = imgcam_shoot();
		g_rw_lock_reader_unlock(&thd_teclock);
		expose = thdshoot;
		readout = 0;
		thdrun = run;
		thdexp = imgcam_get_shpar()->wtime;
		if (thdreadok == 1)
		{
			// Get the time for a complete loop (first loop is invalid result)
			gettimeofday(&clke, NULL);
			fps = (clke.tv_sec - clks.tv_sec + 0.000001 * (clke.tv_usec - clks.tv_usec));
			gettimeofday(&clks, NULL);
		}
		g_rw_lock_writer_unlock(&thd_caplock);
		if (thdshoot)
		{
			// Ok, exposuring
			// Active wait for exp time to elapse, unless user abort	
			if (thdexp > 1000)
			{
				thdtimer = 0;
				expfract = 0;
				tmrexpprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_exp_progress_refresh, NULL);
				while (thdtimer < (thdexp))
				{
					// We yeld to other threads until 500ms are passed by
					thdtimeradd = 0;
					gettimeofday(&clkws, NULL);
					while (thdtimeradd < 500)
					{
						//g_thread_yield();
						g_usleep(100000);
						gettimeofday(&clkwe, NULL);
						// Get elapsed ms
						thdtimeradd = ((clkwe.tv_sec - clkws.tv_sec) * 1000 + 0.001 * (clkwe.tv_usec - clkws.tv_usec));
					}
					thdtimer += thdtimeradd;
					// Reset loop start so we count all time that's yeld to
					// other thread less explicitly (locks)
					// Hopefully counting time with better accuracy
					gettimeofday(&clkws, NULL);
					expfract = (double)thdtimer / (double)thdexp;
					expfract = (expfract > 1) ? 1 : (expfract < 0) ? 0 : expfract;
					tmrexpprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_exp_progress_refresh, NULL);
					// Check if we're still in run condition
					g_rw_lock_reader_lock(&thd_caplock);
					thdrun = run;
					thdmode = capture;
					thdsavejpg = savejpg;
					g_rw_lock_reader_unlock(&thd_caplock);		
					if (thdrun == 0)
					{
						// User abort
						g_rw_lock_writer_lock(&thd_caplock);
						expose = 0;
						g_rw_lock_writer_unlock(&thd_caplock);		
						break;
					}
					gettimeofday(&clkwe, NULL);
					// Get elapsed ms
					thdtimeradd = ((clkwe.tv_sec - clkws.tv_sec) * 1000 + 0.001 * (clkwe.tv_usec - clkws.tv_usec));
					thdtimer += thdtimeradd;
				}
				//char *buf = NULL;
				//printf("Exposure end : %s\n", gettimestamp(buf));
				expfract = 1.0;
			}
			else if (thdexp > 100)
			{
				g_usleep((thdexp * 1000));
				//char *buf = NULL;
				//printf("Exposure end : %s\n", gettimestamp(buf));
				expfract = -1.0;
			}
			else
			{
				//char *buf = NULL;
				//printf("Exposure end : %s\n", gettimestamp(buf));
				expfract = -1.0;
			}
			tmrexpprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_exp_progress_refresh, NULL);
			if (thdrun)
			{
				// Prevent tec readig during frame transfer
				g_rw_lock_reader_lock(&thd_teclock);
				expose = 0;
				readout = 1;
				// Unless aborted during exposure time
				if (imgcam_readout())
				{
					thdreadok = 1;
					// Free tec reading
					g_rw_lock_reader_unlock(&thd_teclock);
					//printf("Readout ok\n");
					g_rw_lock_writer_lock(&thd_caplock);
					readout = 0;
					thdsavejpg = savejpg;

					// The fit data structure is also used anyway to store preview params
					imgfit_init();
					imgfit_set_width(imgcam_get_shpar()->width);
					imgfit_set_height(imgcam_get_shpar()->height);
					imgfit_set_bytepix(imgcam_get_shpar()->bytepix);
					imgfit_set_data(imgcam_get_data());
					
					if (fwhmv == 1)
					{
						// We only cal here, display is done in main 
						// thread (tmr_capture_progress_refresh)
						fwhm_calc();
					}
					
					if ((thdmode == 1) && (runerr == 0))
					{
						// UI flags update in save mode
						shots++;
						filenaming(thdfit);
						if ((savefmt == 2) || (savefmt == 3) || (savefmt == 4))
						{
							// Avi
							if ((imgcam_get_shpar()->width != imgavi_get_width()) || (imgcam_get_shpar()->height != imgavi_get_height()) || (imgcam_get_shpar()->bytepix != imgavi_get_bytepix()) || (shots > avimaxframes))
							{
								writeavih = 2;
								// Close current and create a new one
								imgavi_set_width(imgcam_get_shpar()->width);
								imgavi_set_height(imgcam_get_shpar()->height);
								imgavi_set_bytepix(imgcam_get_shpar()->bytepix);
								avimaxframes = imgavi_get_maxsize() / (imgavi_get_width() * imgavi_get_height() * 6) + shots - 10;
								//printf("Max frames: %d\n", avimaxframes);
								shotsnaming(thdfit, shots);
								imgavi_set_name(thdfit);
								run = imgavi_open();
								runerr = (run == 0);
							}
							if (writeavih > 0)
							{
								// Write avi header file
								if (writeavih == 1)
								{
									shotsnaming(thdfit, shots);
								}
								wrtavihdr(thdfit, fithdr, FITHDR_SLOTS);
							}
							else if (thdsavejpg == 1)
							{
								shotsnaming(thdfit, shots);
							}
							imgavi_set_data(imgcam_get_data());
						}

						if ((savefmt == 1) || (savefmt == 3))
						{
							// Fit
							shotsnaming(thdfit, shots);
							imgfit_set_name(thdfit);
						}
					
						if (thdtlmode == 2)
						{
							// If we are in FULL TimeLapse mode
							ref = time(NULL);
							shotfract = ((double)difftime(ref, mktime(&tlstart)) / (double)difftime(mktime(&tlend), mktime(&tlstart)));
							shotfract = (shotfract > 1.0) ? 1.0 : shotfract;
						}
						else
						{
							shotfract = ((double)(shots - thdpreshots) / (double)expnum);
						}
					}
					g_rw_lock_writer_unlock(&thd_caplock);		

					//printf("run: %d, runerr: %d, expnum: %d, shots: %d\n", run, runerr, expnum, shots);

					// Threaded
					if ((thdmode == 1) && (runerr == 0))
					{
						// Save mode
						if ((savefmt == 1) || (savefmt == 3))
						{
							// Fit
							if (thd_fitsav != NULL)
							{
								g_thread_join(thd_fitsav);							
								thd_fitsav = NULL;
							}
							// Starts background save of fit file
							thd_fitsav = g_thread_new("Fitsave", thd_fitsav_run, NULL);
						}
						if ((savefmt == 2) || (savefmt == 3))
						{
							// Avi (for savefmt == 4, save it's done in the pixbuf run, save from preview)
							if (thd_avisav != NULL)
							{
								g_thread_join(thd_avisav);
								thd_avisav = NULL;
							}
							// Starts background save of avi frame
							thd_avisav = g_thread_new("Avisave", thd_avisav_run, NULL);
						}
					}

					if (thd_pixbuf != NULL)
					{
						// Checks and wait if the pixbuf from previous frame is completed
						g_thread_join(thd_pixbuf);
						thd_pixbuf = NULL;
					}
					// Starts backgroung elaboration of pixbuf
					thd_pixbuf = g_thread_new("Pixbuf", thd_pixbuf_run, (((thdmode == 1) && (thdsavejpg == 1)) ? (gpointer)thdfit : NULL));
				}
				else
				{
					// Free tec reading even in case of error
					g_rw_lock_writer_unlock(&thd_teclock);
					//run = 0;
					//runerr = 1;
					g_rw_lock_writer_lock(&thd_caplock);
					readout = 0;
					thdreadok = 0;
					g_rw_lock_writer_unlock(&thd_caplock);
				}
				//UI update
				if (tmrcapprgrefresh != -1)
				{
					g_source_remove(tmrcapprgrefresh);
				}
				tmrcapprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_capture_progress_refresh, (thdreadok == 1) ? &intTrue : &intFalse);
			}
			// Determine if loop has to stop
			g_rw_lock_reader_lock(&thd_caplock);
			thdmode = capture;
			thdhold = hold;
			thdrun = run;
			thderror = runerr;
			thdtlmode = tlenable + tlcalendar;
			if ((thdtlmode == 2) && (thdrun == 1))
			{
				// If we are in FULL TimeLapse mode
				ref = time(NULL);
				thdrun = ((thdrun == 1) && (difftime(mktime(&tlend), ref) > 0));
			}
			else if ((thdmode == 1) && (thdrun == 1))
			{
				thdrun = ((thdrun == 1) && (expnum > (shots - thdpreshots)));
			}
			g_rw_lock_reader_unlock(&thd_caplock);
			if ((tecrun == 1) && (imgcam_get_tecp()->istec == 1))
			{
				if (thdexp < 500)
				{
					// If tec is in auto mode we must leave some room for the 
					// poor camera cpu to process tec read 
					g_usleep(400000);
				}
			}
			// If we are in tlmode, even bare tl mode
			if ((thdtlmode > 0) && (thdrun == 1))
			{
				// Wait for the tlperiod seconds in 500ms jumps
				thdtimer = 0;
				while (thdtimer < (tlperiod * 1000))
				{
					// We yeld to other threads until 500ms are passed by
					g_usleep(500000);
					thdtimer += 500;
					// Check if we're still in run condition
					g_rw_lock_reader_lock(&thd_caplock);
					thdrun = run;
					thdmode = capture;					
					g_rw_lock_reader_unlock(&thd_caplock);		
					if (thdrun == 0)
					{
						// User abort
						break;
					}
				}
			}
			// If we are in pause mode
			while ((thdhold == 1) && (thdrun == 1))
			{
				// We yeld to other threads until 1000ms are passed by
				g_usleep(1000000);
				g_rw_lock_reader_lock(&thd_caplock);
				thdhold = hold;
				thdmode = capture;
				thdrun = run;
				thdtlmode = tlenable + tlcalendar;
				if ((thdtlmode == 2) && (thdrun == 1))
				{
					// If we are in FULL TimeLapse mode
					ref = time(NULL);
					thdrun = ((thdrun == 1) && (difftime(mktime(&tlend), ref) > 0));
				}
				else if ((thdmode == 1) && (thdrun == 1))
				{
					thdrun = ((thdrun == 1) && (expnum > (shots - thdpreshots)));
				}
				g_rw_lock_reader_unlock(&thd_caplock);
			}
		}
		else
		{
			thderror = 1;
		}
	} // While end
	g_rw_lock_reader_lock(&thd_caplock);
	thdrun = run;
	g_rw_lock_reader_unlock(&thd_caplock);		

	if ((thdmode == 1) && ((savefmt == 2) || (savefmt == 3)))
	{
		//No matter TL mode (and error status), if avi is saved it must be ended properly
		if (thdreadok == 1)
		{			
			if (thd_avisav != NULL)
			{
				// Checks and wait if the last frame add is completed
				g_thread_join(thd_avisav);
				thd_avisav = NULL;
			}
		}
		if (imgavi_isopen())
		{
			imgavi_close();
		}
	} 
	else if (savefmt == 4)
	{
		//No matter TL mode (and error status), if avi is saved it must be ended properly
		if (thdreadok == 1)
		{			
			if (thd_pixbuf != NULL)
			{
				// Checks and wait if the last frame add is completed
				g_thread_join(thd_pixbuf);
				thd_pixbuf = NULL;
			}
		}
		if (imgavi_isopen())
		{
			imgavi_close();
		}	
	}

	if ((thdrun > 0) || (thderror == 1))
	{
		g_rw_lock_writer_lock(&thd_caplock);
		run = 0;
		runerr = thderror;
		g_rw_lock_writer_unlock(&thd_caplock);
		// If the thrad ended, ensure the gui is consistent
		if (tmrcapprgrefresh != -1)
		{
			g_source_remove(tmrcapprgrefresh);
		}
		tmrcapprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_capture_progress_refresh, (thdreadok == 1) ? &intTrue : &intFalse);
	}

	expfract = 0.0;
	tmrexpprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_exp_progress_refresh, NULL);
	
	if (thdreadok == 1)
	{	
		if (thd_fitsav != NULL)
		{
			g_thread_join(thd_fitsav);							
			thd_fitsav = NULL;
		}

		if (thd_pixbuf != NULL)
		{
			// Checks and wait if the pixbuf from previous frame is completed
			g_thread_join(thd_pixbuf);
			thd_pixbuf = NULL;
		}
	}
	if ((imgcam_get_camui()->shutterMode == 1) && imgcam_get_camid() < 1000)
	{
		// For QHY only
		// It's a noop for camera that don't feature a mechanical shutter
		// Close
		imgcam_shutter(1);
		// Release
		imgcam_shutter(2);
	}
	
	g_rw_lock_writer_lock(&thd_caplock);
	run = 0;
	g_rw_lock_writer_unlock(&thd_caplock);
	return 0;
}

gpointer thd_pixbuf_run(gpointer thd_data)
{
	// Stop updating anything that may interfere
	if (tmrimgrefresh != -1)
	{
		g_source_remove(tmrimgrefresh);
	}
	if (tmrhstrefresh != -1)
	{
		g_source_remove(tmrhstrefresh);
	}
	// Update image and graph / preview from the main thread (safer)
	load_image_from_data();
	if (savefmt == 4)
	{
		g_rw_lock_reader_lock(&thd_caplock);
		// Save AVI data from preview
		imgavi_add_from_preview((unsigned char*)gdk_pixbuf_get_pixels(imgpix_get_data()), gdk_pixbuf_get_rowstride(imgpix_get_data()), gdk_pixbuf_get_n_channels(imgpix_get_data()));
		g_rw_lock_reader_unlock(&thd_caplock);
	}
	if ((savejpg == 1) && (thd_data != NULL))
	{
		// Save the captures image to jpg
		char prwfile[2048];
		char prwhst[2048];
		
		sprintf(prwfile, "%s.jpg", (char *)thd_data);
		sprintf(prwhst , "%s.txt", (char *)thd_data);
		if ((imgpix_save_data(prwfile)) && (imgpix_save_histogram_data(prwhst)))
		{
			printf("Fifo: CAPVIEW=New image available (%s)\n", (char *)thd_data);
		}
		else
		{
			printf("Fifo: ERROR=Save capview failed\n");
		}
	}
	tmrhstrefresh = g_timeout_add(1, (GSourceFunc) tmr_hst_refresh, NULL);
	return 0;
}

gpointer thd_fitsav_run(gpointer thd_data)
{
	// Save fit goes here
	g_rw_lock_reader_lock(&thd_caplock);
	int retval = imgfit_save_file(NULL, fithdr, FITHDR_SLOTS);
	g_rw_lock_reader_unlock(&thd_caplock);

	if (retval == 0)
	{
		g_rw_lock_writer_lock(&thd_caplock);
		runerr = 1;
		g_rw_lock_writer_unlock(&thd_caplock);
	}
	return 0;
}

gpointer thd_avisav_run(gpointer thd_data)
{
	g_rw_lock_reader_lock(&thd_caplock);
	int retval = imgavi_add();
	g_rw_lock_reader_unlock(&thd_caplock);

	if (retval == 0)
	{
		g_rw_lock_writer_lock(&thd_caplock);
		runerr = 1;
		g_rw_lock_writer_unlock(&thd_caplock);
	}
	return 0;
}


