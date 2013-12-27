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

void get_filename(char **filename, int mode, char* flt)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(C_("dialog-open","Open File"), (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, ((mode == 1) ? GTK_STOCK_SAVE : GTK_STOCK_OPEN), GTK_RESPONSE_ACCEPT, NULL);
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
		GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(imgpix_get_data(), tgtw, tgth, GDK_INTERP_TILES);
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
		#if GTK_MAJOR_VERSION == 3
		//tgtw -= 15;
		//tgth -= 15;
		#endif
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
		#if GTK_MAJOR_VERSION == 3
		//tgtw -= 15;
		//tgth -= 15;
		#endif
		GdkPixbuf *tmpbuf = gdk_pixbuf_scale_simple(imgpix_get_data(), tgtw, tgth, GDK_INTERP_HYPER);
		gtk_image_set_from_pixbuf((GtkImage *) histogram, tmpbuf);
		g_object_unref(tmpbuf);
	}
	g_rw_lock_reader_unlock(&pixbuf_lock);
}

void fwhm_show()
{
	int roisize = fwhms / imgratio;
	int roix = ((fwhmx - (fwhms / 2)) / imgratio), roiy = ((fwhmy - (fwhms / 2)) / imgratio);
	int lblx = roix, lbly = roiy;
	int width = (imgpix_get_width() / imgratio), height = (imgpix_get_height() / imgratio);

	// check ROI is fully inside frame and fix if needed
	// Move centroid position relative to ROI accordingly
	if( roix <= 0 )
	{
		roix = 1;
	}
	if( roiy <= 0 )
	{
		roiy = 1;
	}
	if( roix + fwhms >= width )
	{
		roix = width - roisize - 1;
	}
	if( roiy + fwhms >= height )
	{
		roiy = height - roisize - 1;
	}	

	// Set flag "is visible"	
	fwhmv = 1;

	gtk_image_set_from_pixbuf((GtkImage *) fwhmroi, imgpix_get_roi(roisize));
	gtk_fixed_move(GTK_FIXED(fixed), fwhmroi, roix, roiy);
	
	// Write label
	sprintf(fwhmfbk, "FWHM=%05.2F, Peak=%d, FWHM/Peak=%05.2F", afwhm, pfwhm, (afwhm / (double)pfwhm));
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
	if ((roix - (fwhmlblw / 2)) > 0)
	{
		lblx = (roix - (fwhmlblw / 2));
	}
	if ((roix + (fwhmlblw / 2)) > width)
	{
		lblx = (roix - fwhmlblw);
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
	double vfwhm, ofwhm;
	int roi[fwhmp];
	unsigned char *porigin = NULL, *psrc = NULL;

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
		if( roix <= 0 )
		{
			resx += (roix + 1);
			roix = 1;
		}
		if( roiy <= 0 )
		{
			resy += (roiy + 1);
			roiy = 1;
		}
		if( roix + fwhms >= width )
		{
			resx += (roix + fwhms - width - 1);
			roix = width - fwhms - 1;
		}
		if( roiy + fwhms >= height )
		{
			resy += (roiy + fwhms - height - 1);
			roiy = height - fwhms - 1;
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

	// Vertical fwhm
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

void combo_ttylist(GtkWidget *cmb)
{
	char ttylist[2048];

	getComList(ttylist);
	combo_setlist(cmb, ttylist);
}

void cfwmsgdestroy(int response)
{
	gtk_dialog_response(GTK_DIALOG(cfwmsg), GTK_RESPONSE_NONE);
	gtk_widget_destroy(cfwmsg);
	
	if (response == 1)
	{
		// All ok, select the filter name for the naming convention
		gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_flt), imgcfw_get_slot());
	}
	else
	{
		sprintf(imgmsg, C_("cfw","Filter wheel reported error"));
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
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
}

gpointer thd_capture_run(gpointer thd_data)
{
	int thdrun = 1, thderror = 0, thdhold = 0, thdmode = 0, thdshoot = 0;
	int thdpreshots = shots, thdexp = 0, thdtlmode = 0;
	int thdtimer = 0, thdtimeradd = 0;
	int avimaxframes = 0;
	char thdfit[2048];
	time_t ref, last;
	struct timeval clks, clke;
	struct timeval clkws, clkwe;
	GThread *thd_pixbuf = NULL;
	GThread *thd_fitsav = NULL;
	GThread *thd_avisav = NULL;

	gettimeofday(&clks, NULL);
	g_rw_lock_reader_lock(&thd_caplock);
	thdmode = capture;
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
			g_rw_lock_reader_unlock(&thd_caplock);		
			if (thdrun == 0)
			{
				// User abort
				break;
			}
			ref = time(NULL);
		}
	}
	// We close shutter just in case it's open because of camera position
	// It's a noop for camera that don't feature a mechanical shutter
	imgcam_shutter(1);
	last = time(NULL);
	// First time read, just in case.
	if ((tecrun == 1) && (imgcam_get_tecp()->istec == 2))
	{
		// Camera only allow temp read with no concurrent access
		imgcam_gettec(&imgcam_get_tecp()->tectemp, NULL); 			
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
				imgcam_gettec(&imgcam_get_tecp()->tectemp, NULL); 			
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
		thdshoot = imgcam_shoot();
		readout = thdshoot;
		thdrun = run;
		thdexp = imgcam_get_shpar()->wtime;
		//if (thdexp < 1000)
		//{
			// Get the time for a complete loop (first loop is invalid result)
			gettimeofday(&clke, NULL);
			fps = (clke.tv_sec - clks.tv_sec + 0.000001 * (clke.tv_usec - clks.tv_usec));
			gettimeofday(&clks, NULL);
		//}
		//else
		//{
		//	fps = 0.;
		//}
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
					g_rw_lock_reader_unlock(&thd_caplock);		
					if (thdrun == 0)
					{
						// User abort
						g_rw_lock_writer_lock(&thd_caplock);
						readout = 0;
						g_rw_lock_writer_unlock(&thd_caplock);		
						break;
					}
					gettimeofday(&clkwe, NULL);
					// Get elapsed ms
					thdtimeradd = ((clkwe.tv_sec - clkws.tv_sec) * 1000 + 0.001 * (clkwe.tv_usec - clkws.tv_usec));
					thdtimer += thdtimeradd;
				}
				expfract = 1.0;
			}
			else if (thdexp > 100)
			{
				g_usleep((thdexp * 1000));
				expfract = -1.0;
			}
			else
			{
				expfract = -1.0;
			}
			tmrexpprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_exp_progress_refresh, NULL);
			if (thdrun)
			{
				// Prevent tec readig during frame transfer
				g_rw_lock_reader_lock(&thd_teclock);
				// Unless aborted during exposure time
				if (imgcam_readout())
				{
					// Free tec reading
					g_rw_lock_reader_unlock(&thd_teclock);
					//printf("Readout ok\n");
					g_rw_lock_writer_lock(&thd_caplock);
					readout = 0;
					
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
						if ((savefmt == 2) || (savefmt == 3))
						{
							// Avi
							if ((imgcam_get_shpar()->width != imgavi_get_width()) || (imgcam_get_shpar()->height != imgavi_get_height()) || (imgcam_get_shpar()->bytepix != imgavi_get_bytepix()) || (shots > avimaxframes))
							{
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
							// Avi
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
					thd_pixbuf = g_thread_new("Pixbuf", thd_pixbuf_run, NULL);
					
					//UI update
					if (tmrcapprgrefresh != -1)
					{
						g_source_remove(tmrcapprgrefresh);
					}
					tmrcapprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_capture_progress_refresh, NULL);
				}
				else
				{
					// Free tec reading even in case of error
					g_rw_lock_writer_unlock(&thd_teclock);
					run = 0;
					runerr = 1;
				}
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
		if (thd_avisav != NULL)
		{
			// Checks and wait if the last frame add is completed
			g_thread_join(thd_avisav);
			thd_avisav = NULL;
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
		// If the thrad ended naturally, ensure the gui is consistent
		if (tmrcapprgrefresh != -1)
		{
			g_source_remove(tmrcapprgrefresh);
		}
		tmrcapprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_capture_progress_refresh, NULL);
	}

	expfract = 0.0;
	tmrexpprgrefresh = g_timeout_add(1, (GSourceFunc) tmr_exp_progress_refresh, NULL);
	
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

	// It's a noop for camera that don't feature a mechanical shutter
	// Close
	imgcam_shutter(1);
	// Release
	imgcam_shutter(2);
	
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
	tmrhstrefresh = g_timeout_add(1, (GSourceFunc) tmr_hst_refresh, NULL);
	return 0;
}

gpointer thd_fitsav_run(gpointer thd_data)
{
	// Save fit goes here
	g_rw_lock_reader_lock(&thd_caplock);
	int retval = imgfit_save_file(NULL);
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


