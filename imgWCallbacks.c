/*
 * imgWCallbacks.c
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

#include "imgWindow.h"
#include "imgWFuncs.h"
#include "imgWCallbacks.h"

gboolean tmr_imgstatus_wipe (GtkWidget *widget)
{
	
	gtk_statusbar_remove_all(GTK_STATUSBAR(imgstatus), 0);
	tmrstatusbar = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_img_refresh (GtkWidget *widget)
{
	load_image_from_data();
	if (hst == 0)
	{
		load_histogram_from_null();
	}	
	tmrimgrefresh = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_adu_check (GtkWidget *widget)
{
	if (tmrimgrefresh != -1)
	{
		g_source_remove(tmrimgrefresh);
	}
	if (scrmaxadu < scrminadu)
	{
		if (strcmp(gtk_widget_get_name(GTK_WIDGET(widget)), "hsc_maxadu") == 0)
		{
			gtk_range_set_value(GTK_RANGE(widget), scrminadu);
		}
		else if (strcmp(gtk_widget_get_name(widget), "hsc_minadu") == 0)
		{
			gtk_range_set_value(GTK_RANGE(widget), scrmaxadu);
		}
	}
	else
	{
		tmrimgrefresh = g_timeout_add(500, (GSourceFunc) tmr_img_refresh, NULL);
	}
	tmraducheck = -1;
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_frm_refresh (GtkWidget *widget)
{
	if (fit == 1)
	{
		set_img_fit();
	}
	else
	{
		set_img_full();
	}
	tmrfrmrefresh = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_hst_refresh (GtkWidget *widget)
{
	if (hst)
	{
		load_histogram_from_data();
	}
	else
	{
		load_histogram_from_null();
	}
	tmrhstrefresh = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_capture_progress_refresh (GtkWidget *widget)
{
	double tmpshots, tmpfract, tmperr, tmprun, tmpfps;
	
	g_rw_lock_reader_lock(&thd_caplock);
	tmpshots = shots;
	tmpfract = shotfract;
	tmperr = runerr;
	tmprun = run;
	tmpfps = fps;
	g_rw_lock_reader_unlock(&thd_caplock);
	
	if (tmpfps > 0.)
	{
		sprintf(fpsfbk, "Fps:%05.1F", (double)tmpfps);
	}
	else
	{
		fpsfbk[0] = '\0';
	}
	if (tmprun == 1)
	{
	
		if (fwhmv == 1)
		{
			fwhm_show();
		}
		
		if (capture)
		{
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_shots), (int)tmpshots);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbr_expnum), tmpfract);
			if (savefmt == 1)
			{
				/// Message on statusbar about last saved frame (%s)
				sprintf(imgmsg, C_("main","Image: %s saved"), imgfit_get_name());
			}
			else if (savefmt == 2)
			{
				/// Message on statusbar about last frame add to avi (%s)
				sprintf(imgmsg, C_("main","Frame: add to %s"), imgavi_get_name());
			}
			else if (savefmt == 3)
			{
				/// Message on statusbar about last saved frame (%s) + last frame add to avi (%s)
				sprintf(imgmsg, C_("main","Image: %s saved, Frame: add to %s"), imgfit_get_name(), imgavi_get_name());
			}
			// Main image update
			sprintf(imgfbk, "#%04d", (int)tmpshots);
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), (gchar *) imgfbk);	
			gtk_label_set_text(GTK_LABEL(lbl_fbkfps), (gchar *) fpsfbk);	
		}
		else
		{
			// Main image update
			if (strcmp(imgfbk, "|") == 0)
			{
				strcpy(imgfbk, "/");
			}
			else if (strcmp(imgfbk, "/") == 0)
			{
				strcpy(imgfbk, "-");
			}
			else if (strcmp(imgfbk, "-") == 0)
			{
				strcpy(imgfbk, "\\");
			}
			else if (strcmp(imgfbk, "\\") == 0)
			{
				strcpy(imgfbk, "|");
			}
			else
			{
				strcpy(imgfbk, "|");
			}
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), (gchar *) imgfbk);	
			gtk_label_set_text(GTK_LABEL(lbl_fbkfps), (gchar *) fpsfbk);	
		}
	}
	else if (tmperr == 1)
	{
		// Run end with error
		if (strlen(imgfit_get_msg()) > 0)
		{ 
			sprintf(imgmsg, "%s", imgfit_get_msg());
		}
		else if (strlen(imgavi_get_msg()) > 0)
		{ 
			sprintf(imgmsg, "%s", imgavi_get_msg());
		}
		else if (strlen(imgcam_get_msg()) > 0)
		{
			sprintf(imgmsg, "%s", imgcam_get_msg());
		}
		else
		{
			sprintf(imgmsg, C_("main","Error starting capture thread"));
		}
		// Main image update
		gtk_label_set_text(GTK_LABEL(lbl_fbkimg), "");	
		gtk_label_set_text(GTK_LABEL(lbl_fbkfps), "");	
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_run)) == TRUE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_run), FALSE);
		}
	}
	else
	{
		// Run end with no error
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_shots), (int)tmpshots);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbr_expnum), tmpfract);
		if (capture)
		{
			// Message to confirm end of capture thread in capture mode
			if (savefmt == 1)
			{
				/// Message on statusbar about last saved frame (%s)
				sprintf(imgmsg, C_("main","Image: %s saved"), imgfit_get_name());
			}
			else if (savefmt == 2)
			{
				/// Message on statusbar about last frame add to avi (%s)
				sprintf(imgmsg, C_("main","Frame: add to %s"), imgavi_get_name());
			}
			else if (savefmt == 3)
			{
				/// Message on statusbar about last saved frame (%s) + last frame add to avi (%s)
				sprintf(imgmsg, C_("main","Image: %s saved, Frame: add to %s"), imgfit_get_name(), imgavi_get_name());
			}
			/// This goes concat with last saved frame
			strcat(imgmsg, C_("main",", capture end"));
			// Main image update
			sprintf(imgfbk, "#%04d", (int)tmpshots);
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), (gchar *) imgfbk);	
		}
		else
		{
			/// Message to confirm end of capture thread in focus mode
			strcpy(imgmsg, C_("main","Focus end"));
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), "");	
		}
		gtk_label_set_text(GTK_LABEL(lbl_fbkfps), (gchar *) fpsfbk);	
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_run)) == TRUE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_run), FALSE);
		}
	}
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	tmrcapprgrefresh = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_exp_progress_refresh (GtkWidget *widget)
{
	double tmpfract;
	
	g_rw_lock_reader_lock(&thd_caplock);
	tmpfract = expfract;
	g_rw_lock_reader_unlock(&thd_caplock);
	
	if (tmpfract >= 0.0)
	{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbr_exptime), tmpfract);
	}
	else
	{
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pbr_exptime));
	}
	tmrexpprgrefresh = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_tecpwr (GtkWidget *widget)
{
	/// Statusbar feedback message about tec power
	sprintf(imgmsg, C_("main","Tec power set to: %d%%"), (int)gtk_range_get_value(GTK_RANGE(vsc_tecpwr)));
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	g_rw_lock_reader_lock(&thd_teclock);
	imgcam_settec(imgcam_get_tecp()->tecpwr);
	g_rw_lock_reader_unlock(&thd_teclock);
	tmrtecpwr = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_tecstatus_write (GtkWidget *widget)
{
	int pct = 0;
	
	g_rw_lock_reader_lock(&thd_teclock);
	if (imgcam_get_tecp()->istec == 1)
	{	
		if (imgcam_get_tecp()->tecerr == 0)
		{
			pct = (int)(((double)imgcam_get_tecp()->tecpwr / (double)imgcam_get_tecp()->tecmax) * 100.);
			if (imgcam_get_tecp()->tecauto)
			{
				/// Statusbar feedback message about cooling status in automatic mode
				sprintf(imgmsg, C_("main","Tec: %+06.2FC, Target: %+06.2FC, Power: %d%%"), imgcam_get_tecp()->tectemp, imgcam_get_tecp()->settemp, pct);
			}
			else
			{
				/// Satusbar feedback message about cooling in manual mode
				sprintf(imgmsg, C_("main","Tec: %+06.2fC, Power: %d%%"), imgcam_get_tecp()->tectemp, pct);
			}
			// Main image update
			sprintf(tecfbk, "%+06.2fC", imgcam_get_tecp()->tectemp);
			gtk_label_set_text(GTK_LABEL(lbl_fbktec), (gchar *) tecfbk);	
			// Graph update
			tec_print_graph();
			// Slider update
			gtk_range_set_value(GTK_RANGE(vsc_tecpwr), pct);
			gtk_range_set_value(GTK_RANGE(vsc_tectemp), imgcam_get_tecp()->tectemp);
		}
		else
		{
			imgcam_get_tecp()->tecerr = 0;
			sprintf(imgmsg, C_("main","Error communicating with tec"));
		}
	}
	else if (imgcam_get_tecp()->istec == 2)
	{
		/// Satusbar feedback message about temperature only
		sprintf(imgmsg, C_("main","Temp: %+06.2fC"), imgcam_get_tecp()->tectemp);
		// Main image update
		sprintf(tecfbk, "%+06.2fC", imgcam_get_tecp()->tectemp);
		gtk_label_set_text(GTK_LABEL(lbl_fbktec), (gchar *) tecfbk);	
		// Graph update
		tec_print_graph();
		// Slider update
		gtk_range_set_value(GTK_RANGE(vsc_tectemp), imgcam_get_tecp()->tectemp);
	}
	g_rw_lock_reader_unlock(&thd_teclock);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 1, imgmsg);
	tmrtecrefresh = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_tlrefresh (GtkWidget *widget)
{
	g_rw_lock_reader_lock(&thd_teclock);
	int lrun = run, lmode = tlenable + tlcalendar;
	g_rw_lock_reader_unlock(&thd_teclock);
	
	if ((lmode == 2) && (lrun == 1))
	{
		char strstart[32], strend[32];

		strftime(strstart, 32, "%Y/%m/%d %H:%M:%S" ,&tlstart);
		strftime(strend,   32, "%Y/%m/%d %H:%M:%S" ,&tlend);
		/// Starusbar feedback message about timelapse mode
		sprintf(imgmsg, C_("main","TimeLapse mode full, start at: %s, end at: %s, interval: %d seconds"), strstart, strend, tlperiod);
	}
	else if ((lmode == 1) && (lrun == 1))
	{
		sprintf(imgmsg, C_("main","TimeLapse mode simple, interval: %d seconds"), tlperiod);
	}
	else
	{
		tmrtlrefresh = -1;
		sprintf(imgmsg, C_("main","TimeLapse mode finished"));	
	}
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 2, imgmsg);
	
	if (tmrtlrefresh != -1)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void imgstatus_push (GtkStatusbar *statusbar, guint context_id, gchar *text, gpointer user_data)
{
	if (tmrstatusbar != -1)
	{
		g_source_remove(tmrstatusbar);
	}
	tmrstatusbar = g_timeout_add_seconds(3, (GSourceFunc) tmr_imgstatus_wipe, NULL);
}

void cmd_settings_click(GtkWidget *widget, gpointer data)
{
	static int state = 0;
	
	state = (state == 0)? 1: 0;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(tab_right), state);
	if (state)
	{
		gtk_button_set_label(GTK_BUTTON(cmd_settings), C_("main","Hide settings"));
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(cmd_settings), C_("main","Show settings"));
	}
}

/* 
 * Original code taken from eccvs2, http://eccvs.sf.net/ adapted and upgraded to keep up with gtk changes
 */
void cmd_about_click(GtkWidget *widget, gpointer data)
{
	gchar* authors[]     = { "Giampiero Spezzano ", "Clive Rogers ", "Dan Holler ", "Fabrice Phung","Contributors are always welcome ", NULL };
	gchar* artists[]     = { "Wanted, a fancy icon and logo would be great! ", NULL };
	const gchar* translators   = "Giampiero Spezzano (IT), Fabrice Phung (FR-DE), Max Chen (CN)";
	gchar* documenters[] = { "Wanted!! ", NULL };	
	const gchar* comments      = C_("about","OpenSkyImager is a capture program written for Astronomy camera operation");
	const gchar* copyright     = C_("about","Copyright (c) 2013 JP & C AstroSoftware\n\nLicensed under GNU GPL 3.0\n\nThis program is free software: you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of the License, or\nany later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.");
	gchar* name          = APPNAM;
	gchar* version       = APPVER;
	gchar* website       = "http://www.googole.com";
	const gchar* website_lbl   = C_("about","OpenSkyImager website");
	
	gtk_show_about_dialog (GTK_WINDOW(window),
				 		"authors", authors, 
						"artists", artists,
						"documenters", documenters, 
						"translator-credits", translators,
						"logo", icopixbuf, 
						"program-name", name,
						"comments", comments, 
						"copyright", copyright, 
				          "version", version, 
				          "website", website, 
				          "website-label", website_lbl, 
				          NULL);
}
/* 
 * End code taken from eccvs2, http://eccvs.sf.net/ 
 */

gboolean spn_expnum_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_caplock);
	expnum = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);
	return FALSE;
}

gboolean spn_shots_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	if (shots != gtk_spin_button_get_value(spinbutton))
	{
		g_rw_lock_writer_lock(&thd_caplock);
		shots = gtk_spin_button_get_value(spinbutton);
		g_rw_lock_writer_unlock(&thd_caplock);
	}
	return FALSE;
}

void cmd_capture_click(GtkWidget *widget, gpointer data)
{
	static int error = 0;
	
	if (error == 0)
	{
		if ((strlen(fitfolder) > 0) && (strlen(fitbase) > 0))
		{
			g_rw_lock_writer_lock(&thd_caplock);
			capture = (capture == 0)? 1: 0;
			g_rw_lock_writer_unlock(&thd_caplock);
			if (capture)
			{
				gtk_button_set_label(GTK_BUTTON(cmd_capture), C_("main","Capture mode"));
				gtk_widget_set_sensitive(box_filename, 0);
				gtk_widget_set_sensitive(box_cfw, 0);
				fwhm_hide();
			}
			else
			{
				gtk_widget_set_sensitive(box_filename, 1);
				gtk_widget_set_sensitive(box_cfw, 1);
				gtk_button_set_label(GTK_BUTTON(cmd_capture), C_("main","Focus mode"));
			}
		}
		else
		{
			error = 1;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Folder or base name not set!"));
		}
	}
	else
	{
		error = 0;
	}
}

void cmd_load_click(GtkWidget *widget, gpointer data)
{
	char *filename = NULL;
	
	get_filename(&filename, 0, "*.fit");

	if (filename != NULL)
	{
		//gtk_image_clear(GTK_IMAGE(image));
		if (imgfit_load_file(filename))
		{
			if (imgfit_loaded())
			{
				load_image_from_data();
				if (hst)
				{
					load_histogram_from_data();
				}
				else
				{
					load_histogram_from_null();
				}
				// Main image update
				imgfbk[0] = '\0';
				gtk_label_set_text(GTK_LABEL(lbl_fbkimg), (gchar *) imgfbk);
			}
		}
		else if (strlen(imgfit_get_msg()) != 0)
		{
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgfit_get_msg());    
		}
	}
	g_free(filename);
}

void cmd_run_click(GtkWidget *widget, gpointer data)
{
	static int error = 0, kill = 0;
	int brun = 0, breadout = 0;
	
	if (error == 0)
	{
		/*printf("Current exposure paramenters\n");
		printf("gain    : %d\n", imgcam_get_shpar()->gain);
		printf("offset  : %d\n", imgcam_get_shpar()->offset);
		printf("time    : %d\n", imgcam_get_shpar()->time);
		printf("bin     : %d\n", imgcam_get_shpar()->bin);
		printf("width   : %d\n", imgcam_get_shpar()->width);
		printf("height  : %d\n", imgcam_get_shpar()->height);
		printf("speed   : %d\n", imgcam_get_shpar()->speed);
		printf("mode    : %d\n", imgcam_get_shpar()->mode);
		printf("amp     : %d\n", imgcam_get_shpar()->amp);
		printf("bytepix : %d\n", imgcam_get_shpar()->bytepix);
		printf("bitpix  : %d\n", imgcam_get_shpar()->bitpix);
		printf("tsize   : %d\n", imgcam_get_shpar()->tsize);*/
		if (imgcam_connected())
		{
			g_rw_lock_reader_lock(&thd_caplock);
			brun = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)? 1: 0;
			breadout = readout;
			g_rw_lock_reader_unlock(&thd_caplock);
			if (brun == 1)
			{
				if (zerofc)
				{
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_shots), 0.0);
				}
				if (thd_capture != NULL)
				{
					g_thread_unref(thd_capture);
					thd_capture = NULL;
				}
				g_rw_lock_writer_lock(&thd_caplock);
				runerr = 0;
				run    = 1;
				g_rw_lock_writer_unlock(&thd_caplock);
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbr_expnum), 0.0);
				GError* thd_err = NULL;
				thd_capture = g_thread_try_new("Capture", thd_capture_run, NULL, &thd_err);
				if (thd_capture == NULL)
				{
					GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,  GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error starting capture thread:\n%s", thd_err->message);
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy(dialog);			
					g_error_free(thd_err);
					thd_err = NULL;
					run = 0;
					error = 1;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
					gtk_button_set_label(GTK_BUTTON(widget), "Start");
					gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Read from camera thread failed to start"));
				}
				else
				{
					//gtk_widget_set_sensitive(box_timelapse, 0);
					gtk_widget_set_sensitive(spn_shots, 0);
					gtk_widget_set_sensitive(cmd_load, 0);
					gtk_widget_set_sensitive(cmd_hold, 1);
					gtk_button_set_label(GTK_BUTTON(widget), "Stop");
					if (tlenable == 1)
					{
						// Start recurring timer to refresh the message
						// Timer will kill itself if !tlenable || !run
						if (tmrtlrefresh != -1)
						{
							g_source_remove(tmrtlrefresh);
						}
						tmrtlrefresh = g_timeout_add_seconds(5, (GSourceFunc) tmr_tlrefresh, NULL);			
					}
					if (capture)
					{
						gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Capture start"));
					}
					else
					{
						gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Focus start"));
					}
				}
			}
			else
			{
				if (breadout)
				{
					// Long exposure running; abort
					if (imgcam_get_shpar()->time > 1000)
					{
						if (kill == 1)
						{
							if (imgcam_abort())
							{
								kill = 0;
								g_rw_lock_writer_lock(&thd_caplock);
								run  = 0;
								g_rw_lock_writer_unlock(&thd_caplock);
								// Reset style
								gtk_button_set_label(GTK_BUTTON(widget), C_("main","Start"));
								gtk_widget_modify_bkg(widget, GTK_STATE_ACTIVE, &clrSelected);
								//gtk_widget_set_sensitive(box_timelapse, 1);
								gtk_widget_set_sensitive(spn_shots, 1);
								gtk_widget_set_sensitive(cmd_load, 1);
								gtk_widget_set_sensitive(cmd_hold, 0);
								gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Running exposure aborted"));
							}
							else
							{
								// Must remain active
								error = 1;
								gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
								gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Abort capture failed or not supported, please wait end of current exposure"));
							}
						}
						else
						{
							// Reactivate with noop
							kill  = 1;
							g_rw_lock_writer_lock(&thd_caplock);
							run   = 2;
							g_rw_lock_writer_unlock(&thd_caplock);
							error = 1;
							// Set style
							/// Start / Stop / Kill button state message
							gtk_button_set_label(GTK_BUTTON(widget), C_("main","Kill"));
							gtk_widget_modify_bkg(widget, GTK_STATE_ACTIVE, &clrKill);
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
							gtk_widget_set_sensitive(cmd_hold, 0);
						}
					}
					else
					{
						g_rw_lock_writer_lock(&thd_caplock);
						run  = 2;
						g_rw_lock_writer_unlock(&thd_caplock);
						error = 1;
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
						gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Wait last capture end"));		
					}
				}
				else
				{
					g_rw_lock_writer_lock(&thd_caplock);
					run  = 0;
					g_rw_lock_writer_unlock(&thd_caplock);
					if (kill == 1)
					{
						kill = 0;
						// Reset style
						gtk_button_set_label(GTK_BUTTON(widget), C_("main","Start"));
						gtk_widget_modify_bkg(widget, GTK_STATE_ACTIVE, &clrSelected);
						gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Running exposure aborted"));
					}
					if (hold == 1)
					{
						// Reset pause state (and flag)
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_hold), FALSE);
					}
					// Reset style (just in case)
					gtk_button_set_label(GTK_BUTTON(widget), C_("main","Start"));
					gtk_widget_modify_bkg(cmd_settings, GTK_STATE_ACTIVE, &clrSelected);
					//gtk_widget_set_sensitive(box_timelapse, 1);
					gtk_widget_set_sensitive(spn_shots, 1);
					gtk_widget_set_sensitive(cmd_load, 1);
					gtk_widget_set_sensitive(cmd_hold, 0);
				}
			}
		}
	}
	else
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
		{
			if (kill == 1)
			{
				gtk_button_set_label(GTK_BUTTON(widget), C_("main","Kill"));
			}
			else
			{
				gtk_button_set_label(GTK_BUTTON(widget), C_("main","Stop"));
			}
		}
		else
		{
			gtk_button_set_label(GTK_BUTTON(widget), C_("main","Start"));
		}
		error = 0;
	}
}   

void cmd_hold_click(GtkWidget *widget, gpointer data)
{	
	g_rw_lock_writer_lock(&thd_caplock);
	hold = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	g_rw_lock_writer_unlock(&thd_caplock);
	if (hold)
	{
		/// Hold / Resume status message
		gtk_button_set_label(GTK_BUTTON(widget), C_("main","Resume"));
		if (capture)
		{
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Capture hold"));
		}
		else
		{
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Focus hold"));
		}
	}
	else
	{
		/// Hold resume status message
		gtk_button_set_label(GTK_BUTTON(widget), C_("main","Hold"));
		if (capture)
		{
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Capture resume"));
		}
		else
		{
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Focus resume"));
		}
	}
}   	

void cmd_fit_click(GtkWidget *widget, gpointer data)
{	
	if (fit == 0)
	{
		/// 1:1 / Fit status message
		gtk_button_set_label(GTK_BUTTON(widget), C_("main","Fit"));
		set_img_fit();
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Fit to screen"));
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(widget), C_("main","1:1"));
		set_img_full();
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Original size"));
	}
}   

void cmd_histogram_click(GtkWidget *widget, gpointer data)
{	
	hst = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (hst)
	{
		load_histogram_from_data();
		gtk_button_set_label(GTK_BUTTON(cmd_histogram), C_("main","Hide graph"));
		gtk_widget_set_sensitive(spn_histogram, 1);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Show graph image"));
	}
	else
	{
		load_histogram_from_null();
		gtk_button_set_label(GTK_BUTTON(cmd_histogram), C_("main","Show graph"));
		gtk_widget_set_sensitive(spn_histogram, 0);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Show preview thumbnail"));
	}
}   	

//gboolean spn_histogram_changed (GtkSpinButton *spinbutton, GtkScrollType scroll, gdouble value, gpointer user_data)
gboolean spn_histogram_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	if (tmrhstrefresh != -1)
	{
		g_source_remove(tmrhstrefresh);
	}
	tmrhstrefresh = g_timeout_add(500, (GSourceFunc) tmr_hst_refresh, NULL);
	return FALSE;
}

gboolean hsc_maxadu_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	scrmaxadu = gtk_range_get_value(range);
	if (tmraducheck != -1)
	{
		g_source_remove(tmraducheck);
	}
	tmraducheck   = g_timeout_add(5  , (GSourceFunc) tmr_adu_check, range);
	return FALSE;
}

gboolean hsc_minadu_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	scrminadu = gtk_range_get_value(range);	
	if (tmraducheck != -1)
	{
		g_source_remove(tmraducheck);
	}
	tmraducheck   = g_timeout_add(5  , (GSourceFunc) tmr_adu_check, range);
	return FALSE;
}

gboolean frm_histogram_allocate(GtkWidget *widget, GdkRectangle *alloc, gpointer data)
{
	static int ww = 0, wh = 0;
	int nww, nwh;	
	
	if (ww == 0)
	{
		ww = alloc->width;
		wh = alloc->height;
	}
	nww = alloc->width;
	nwh = alloc->height;
	if ((nww != ww) || (nwh != wh))
	{
		// frame size has changed
		ww = nww;
		wh = nwh;
		if (hst)
		{
			load_histogram_from_data();
		}
		else
		{
			load_histogram_from_null();
		}
	}
	return FALSE;
}

gboolean swindow_allocate(GtkWidget *widget, GdkRectangle *alloc, gpointer data)
{
	static int ww = 0, wh = 0;
	int nww, nwh;	
	
	if (ww == 0)
	{
		ww = alloc->width;
		wh = alloc->height;
	}
	nww = alloc->width;
	nwh = alloc->height;
	if ((nww != ww) || (nwh != wh))
	{
		// window size has changed
		ww = nww;
		wh = nwh;
		if (fit == 1)
		{
			set_img_fit();
		}
	}
	return FALSE;
}

gboolean fwhmroi_scroll (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	int roisize = fwhms / imgratio;
	int roix = ((fwhmx - (fwhms / 2)) / imgratio), roiy = ((fwhmy - (fwhms / 2)) / imgratio);

	if ((event->type == GDK_SCROLL) && (fwhmv == 1))
	{
		// It's a scroll event and the fwhm is visible 
		if (((event->x > roix) && (event->x < (roix + roisize))) && ((event->y > roiy) && (event->y < (roiy + roisize))))
		{
			// It's in the ROI
			#if GTK_MAJOR_VERSION == 3
				#if GTK_MINOR_VERSION >= 4
					GdkScrollDirection direction;
					if (event->delta_y != 0) 
					{
						direction = (event->delta_y != 0) ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
					}
					else
					{
						direction = event->direction;
					}
				#else
					GdkScrollDirection direction = event->direction;
				#endif
			#else
				GdkScrollDirection direction = event->direction;
			#endif
		
			if (direction == GDK_SCROLL_UP)
			{
				fwhmp *= (fwhms < 64) ? 4 : 1;
				fwhms *= (fwhms < 64) ? 2 : 1;
			}
			else if (direction == GDK_SCROLL_DOWN)
			{
				fwhmp /= (fwhms > 8) ? 4 : 1;
				fwhms /= (fwhms > 8) ? 2 : 1;
			}
		
			// Draw roi
			fwhm_show();
			// Calc
			fwhm_calc();
			// Draw roi after possible calc move
			fwhm_show();
			return TRUE;
		}
	}
	return FALSE;
}

gboolean image_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	int roisize = fwhms / imgratio;
	int roix, roiy;
	
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1))
	{
		roix = ((fwhmx - (fwhms / 2)) / imgratio);
		roiy = ((fwhmy - (fwhms / 2)) / imgratio);

		if (((event->x > roix) && (event->x < (roix + roisize))) && ((event->y > roiy) && (event->y < (roiy + roisize))))
		{
			fwhm_hide();
		}
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
	{
		g_rw_lock_reader_lock(&thd_caplock);
		int width = (imgpix_get_width() / imgratio), height = (imgpix_get_height() / imgratio);

		// Center on image data regardless of "fit to screen"
		fwhmx = event->x * imgratio;
		fwhmy = event->y * imgratio;

		// Roi position depending on "fit to screen"
		roix = ((fwhmx - (fwhms / 2)) / imgratio);
		roiy = ((fwhmy - (fwhms / 2)) / imgratio);

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
		
		// Center on image data regardless of "fit to screen"
		fwhmx = (roix * imgratio) + (fwhms / 2);
		fwhmy = (roiy * imgratio) + (fwhms / 2);

		// Draw roi
		fwhm_show();
		// Calc
		fwhm_calc();
		// Draw roi after possible calc move
		fwhm_show();
		g_rw_lock_reader_unlock(&thd_caplock);
	}
	return FALSE;
}

void mainw_destroy( GtkWidget *widget, gpointer   data )
{
	gtk_main_quit ();
}

gboolean mainw_delete_event( GtkWidget *widget, GdkEvent *event, gpointer data)
{
	int retval = TRUE;
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, C_("quit-message","Confirm exit"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog), C_("quit-message","Do you really want to quit?"));
	gint result =  gtk_dialog_run (GTK_DIALOG (dialog));
	
	switch (result)
	{
		case GTK_RESPONSE_OK:
			if (imgcam_connected())
			{
				//Press on disconnect
				gtk_widget_activate(cmd_camera);
			}
			retval = FALSE;
			break;
		default:
			retval = TRUE;
			break;
	}
	gtk_widget_destroy(dialog);
	/* Change TRUE to FALSE and the main window will be destroyed with
	* a "delete-event". */
	return retval;
}

void cmb_debayer_changed (GtkComboBox *widget, gpointer user_data)
{	
	load_image_from_data();
	if (!hst)
	{
		load_histogram_from_null();
	}
	sprintf(imgmsg, C_("main","Debayer strategy set to: %s"), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

void cmb_exptime_changed (GtkComboBox *widget, gpointer user_data)
{
	//printf("%s\n", gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
	float tmp;
	
	g_rw_lock_writer_lock(&thd_caplock);
	sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%f", &tmp);
	if (tmp > 0) 
	{
		imgcam_get_expar()->time = (int) (tmp * 1000);
	}
	else
	{
		imgcam_get_expar()->time = 1;
	}
	imgcam_get_expar()->edit = 1;
	g_rw_lock_writer_unlock(&thd_caplock);
	sprintf(imgmsg, C_("main","Exposure time set to: %s"), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

gboolean numbers_input_keypress (GtkWidget *widget, GdkEventKey *event, int maxchars)
{
	char *kname = gdk_keyval_name(event->keyval);
	
	//printf("Keypress: %s %d\n", kname, event->keyval);

	if ((strcmp(kname, "BackSpace") == 0) || (strcmp(kname, "Home") == 0) || (strcmp(kname, "End") == 0) || (strcmp(kname, "Up") == 0) || (strcmp(kname, "Down") == 0) || (strcmp(kname, "Left") == 0) || (strcmp(kname, "Right") == 0) || (strcmp(kname, "Page_Up") == 0) || (strcmp(kname, "Page_Down") == 0))
		// Movement keys are accepted no matter what
		return FALSE;	

	if (strlen(gtk_entry_get_text(GTK_ENTRY(widget))) >= maxchars)
		// In order to accept key, string must be shorter than max
		return TRUE;

	if ((strcmp(kname, "plus") == 0) && (strlen(gtk_entry_get_text(GTK_ENTRY(widget))) == 0))
		// + is accepted as first char only
		return FALSE;	
		
	if ((strcmp(kname, "minus") == 0) && (strlen(gtk_entry_get_text(GTK_ENTRY(widget))) == 0))
		// - is accepted as first char only
		return FALSE;	
		
	if ((event->keyval >= 48) && (event->keyval <= 57))
		// Numbers are accepted
		return FALSE;	
	if ((sysloc->decimal_point[0] == event->keyval) && (strchr(gtk_entry_get_text(GTK_ENTRY(widget)), sysloc->decimal_point[0]) == NULL))
		// Only one decimal separator is allowed
		return FALSE;

	return TRUE;
}

void cmb_camera_changed (GtkComboBox *widget, gpointer user_data)
{
	if (gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)) != NULL)
	{
		// Valid camera selected, even None.
		gtk_widget_set_sensitive(cmd_camera, (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) > 0));
		gtk_widget_set_sensitive(cmd_resetcam, (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) > 0));
		imgcam_set_model(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
		sprintf(imgmsg, C_("main","Camera %s selected"), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
}

void cmd_camera_click(GtkWidget *widget, gpointer data)
{	
	static int error = 0;
	
	if (error == 0)
	{
		if (imgcam_connected())
		{
			//Disconnect
			if ((imgcam_get_tecp()->istec != 0))
			{
				// Terminates the tec thread and disable choice
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), FALSE);
				gtk_widget_set_sensitive(cmd_tecenable, 0);
			}
			if (strlen(imgcam_get_camui()->whlstr) > 0)
			{
				// Delete In-camera Wheel choice is there's one
				if (imgcfw_get_mode() == 99)
				{
					// Set connection to none (and reset cfwmode)
					int pre = gtk_combo_box_get_active(GTK_COMBO_BOX(cmb_cfw));
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfw), 0);
					// Delete the 99-In Camera mode row
					gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(cmb_cfw), pre);
				}
			}
			if (imgcam_disconnect() == 1)
			{
				// Disable camera model/type related UI 
				combo_setlist(cmb_bin, "");
				combo_setlist(cmb_csize, "");
				combo_setlist(cmb_dspeed, "");
				combo_setlist(cmb_mode, "");
				gtk_label_set_text(GTK_LABEL(lbl_mode), "");				
				combo_setlist(cmb_amp, "");
				combo_setlist(cmb_denoise, "");
				combo_setlist(cmb_depth, "");
				gtk_widget_set_sensitive(cmb_debayer, 1);
				gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_debayer), 0);
				//Enable choice list
				gtk_widget_set_sensitive(cmb_camera, 1);
				gtk_widget_set_sensitive(cmd_setcamlst, 1);
				gtk_widget_set_sensitive(cmd_updcamlst, 1);
				gtk_widget_set_sensitive(cmd_resetcam, 1);
				gtk_widget_set_sensitive(cmd_run, 0);
				gtk_button_set_label(GTK_BUTTON(widget), C_("settings","Connect"));
				sprintf(imgmsg, C_("main","Camera %s disconnected"), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cmb_camera)));
				gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
			}
			else
			{
				//Engage
				GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,  GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", imgcam_get_msg());
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy(dialog);
				error = 1;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			}
		}
		else
		{
			//Connect
			if (imgcam_connect() == 1)
			{
				// Update camera model/type related UI 
				// some choices can only be made after connection
				int tmp = 0;
				combo_setlist(cmb_bin, imgcam_get_camui()->binstr);
				combo_setlist(cmb_csize, imgcam_get_camui()->roistr);
				combo_setlist(cmb_dspeed, imgcam_get_camui()->spdstr);
				combo_setlist(cmb_mode, imgcam_get_camui()->modstr);
				gtk_label_set_text(GTK_LABEL(lbl_mode), imgcam_get_camui()->moddsc);				
				combo_setlist(cmb_amp, imgcam_get_camui()->ampstr);
				combo_setlist(cmb_denoise, imgcam_get_camui()->snrstr);
				combo_setlist(cmb_depth, imgcam_get_camui()->bppstr);
				sscanf(imgcam_get_camui()->byrstr, "%d", &tmp);
				if (gtk_combo_box_get_active(GTK_COMBO_BOX(cmb_camera)) > 0)
				{
					if (tmp > -1 && tmp < 5)
					{
						gtk_widget_set_sensitive(cmb_debayer, 1);
						gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_debayer), tmp);
					}
					else
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_debayer), 0);
						gtk_widget_set_sensitive(cmb_debayer, 0);
					}
				}
				else
				{
					gtk_widget_set_sensitive(cmb_debayer, 1);
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_debayer), 0);
				}
				//Disable choice list
				gtk_widget_set_sensitive(cmb_camera, 0);
				gtk_widget_set_sensitive(cmd_setcamlst, 0);
				gtk_widget_set_sensitive(cmd_updcamlst, 0);
				gtk_widget_set_sensitive(cmd_resetcam, 0);
				gtk_widget_set_sensitive(cmd_run, 1);
				gtk_button_set_label(GTK_BUTTON(widget), C_("settings","Disconnect"));
				// Connection message
				sprintf(imgmsg, C_("main","Camera %s connected"), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cmb_camera)));
				gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
				// Tec?
				if ((imgcam_get_tecp()->istec != 0))
				{
					gtk_widget_set_sensitive(cmd_tecenable, 1);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), TRUE);
				}
				// In-camera Wheel?
				if (strlen(imgcam_get_camui()->whlstr) > 0)
				{
					int pre = gtk_combo_box_get_active(GTK_COMBO_BOX(cmb_cfw));
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_cfw), C_("cfw","99-In camera"));
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfw), pre);
				}
			}
			else
			{
				//Disengage
				sprintf(imgmsg, "%s", imgcam_get_msg());
				gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
				error = 1;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
			}
		}
	}
	else
	{
		error = 0;
	}
}   

void cmd_setcamlst_click(GtkWidget *widget, gpointer data)
{	
	fullcamlist = (fullcamlist == 0) ? 1 : 0;
	combo_setlist(cmb_camera, imgcam_init_list(fullcamlist));
	if (fullcamlist)
	{
		sprintf(imgmsg, C_("main","Full camera list set"));
	}
	else
	{
		sprintf(imgmsg, C_("main","Active camera list set"));
	}
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}   

void cmd_updcamlst_click(GtkWidget *widget, gpointer data)
{
	combo_setlist(cmb_camera, imgcam_init_list(fullcamlist));
}

void cmd_resetcam_click(GtkWidget *widget, gpointer data)
{
	imgcam_reset();
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgcam_get_msg());
}

gboolean hsc_gain_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_caplock);
	imgcam_get_expar()->gain = gtk_range_get_value(range);
	imgcam_get_expar()->edit = 1;
	sprintf(imgmsg, C_("main","Gain set to: %d"), imgcam_get_expar()->gain);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	g_rw_lock_writer_unlock(&thd_caplock);
	return FALSE;
}

gboolean hsc_offset_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_caplock);
	imgcam_get_expar()->offset = gtk_range_get_value(range);
	imgcam_get_expar()->edit = 1;
	sprintf(imgmsg, C_("main","Offset set to: %d"), imgcam_get_expar()->offset);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	g_rw_lock_writer_unlock(&thd_caplock);
	return FALSE;
}

void cmb_bin_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%dx", &tmp);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->bin = tmp;
			imgcam_get_expar()->edit = 1;
			sprintf(imgmsg, C_("main","Binning mode set to: %dx%d"), imgcam_get_expar()->bin, imgcam_get_expar()->bin);
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		}
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_csize_changed (GtkComboBox *widget, gpointer user_data)
{
	int w, h;
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%dx%d", &w, &h);
		if (w > 0 && h > 0) 
		{
			imgcam_get_expar()->width  = w;
			imgcam_get_expar()->height = h;
		}
		else
		{
			imgcam_get_expar()->width  = 0;
			imgcam_get_expar()->height = 0;
		}
		sprintf(imgmsg, C_("main","Capture size set to: %dx%d"), imgcam_get_expar()->width, imgcam_get_expar()->height);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_dspeed_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	char str[32];
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->speed = tmp;
		}
		else
		{
			imgcam_get_expar()->speed = 0;
		}
		sprintf(imgmsg, C_("main","Download speed set to: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_mode_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	char str[32];
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		str[0] = '\0';
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->mode = tmp;
		}
		else
		{
			imgcam_get_expar()->mode = 1;
		}
		if (strlen(str) == 0)
		{
			sprintf(str, "%d", tmp);
		}
		/// This can be "Capture mode set to...  or Usb Speed set to...
		/// Content of the first part is translated separately
		sprintf(imgmsg, C_("main","%s set to: %s"), imgcam_get_camui()->moddsc, str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}


void cmb_amp_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	char str[32];
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->amp = tmp;
		}
		else
		{
			imgcam_get_expar()->amp = 0;
		}
		sprintf(imgmsg, C_("main","Amp control set to: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_denoise_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	char str[32];
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->denoise = tmp;
		}
		else
		{
			imgcam_get_expar()->denoise = 0;
		}
		sprintf(imgmsg, C_("main","Noise reduction set to: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_depth_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	char str[32];
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->bytepix = tmp;
		}
		else
		{
			imgcam_get_expar()->bytepix = 0;
		}
		sprintf(imgmsg, C_("main","Image depth set to: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmd_saveas_click(GtkWidget *widget, gpointer data)
{
	char *filename = NULL;
	char  folder[1024];
	char  file[1024];
	char *pch      = NULL;
	
	get_filename(&filename, 1, "*.fit");

	if (filename != NULL)
	{
		strcpy(folder, strcat(g_path_get_dirname(filename), "/"));
		strcpy(file, g_path_get_basename(filename));
		if ((pch = strrchr(file, '.')) != NULL)
		{
			if (strcmp(pch, ".fit") == 0)
			{
				// Get rid of the .fit
				file[pch - file] = '\0';
			}
		}
		gtk_entry_set_text(GTK_ENTRY(txt_fitfolder), folder);
		gtk_entry_set_text(GTK_ENTRY(txt_fitbase), file);
		sprintf(imgmsg, C_("main","Base folder/file set: %s%s"), folder, file);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
	g_free(filename);
}

void cmd_audela_click(GtkWidget *widget, gpointer data)
{	
	audelanaming = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (audelanaming)
	{
		// First set default base folder, the ""date part will be done later
		strcpy(fitfolder, g_get_home_dir());
		gtk_entry_set_text(GTK_ENTRY(txt_fitfolder), fitfolder);
		// User should not fiddle with this
		gtk_widget_set_sensitive(txt_fitfolder, 0);
		// User must be able to set a "base" name
		gtk_widget_set_sensitive(txt_fitbase, 1);
		if (strlen(fitbase) == 0)
		{
			// Just in case it's empty
			strcpy(fitbase, "Image");
			gtk_entry_set_text(GTK_ENTRY(txt_fitbase), fitbase);			
		}
		if (fitdateadd == 1)
		{
			// Just in case it was active
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_dateadd), FALSE);	
		}
		gtk_widget_set_sensitive(cmd_dateadd, 0);
		if (fittimeadd == 1)
		{
			// Just in case it was active
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_timeadd), FALSE);	
		}
		gtk_widget_set_sensitive(cmd_timeadd, 0);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Audela naming convention selected"));
	}
	else
	{
		gtk_widget_set_sensitive(txt_fitfolder, 1);
		gtk_widget_set_sensitive(cmd_dateadd, 1);
		gtk_widget_set_sensitive(cmd_timeadd, 1);
		gtk_widget_set_sensitive(txt_fitbase, 1);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Audela naming convention removed"));
	}
}   	

void cmd_iris_click(GtkWidget *widget, gpointer data)
{	
	irisnaming = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (irisnaming)
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Iris naming convention selected"));
	}
	else
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Iris naming convention removed"));
	}
}   	

void cmd_zerofc_click(GtkWidget *widget, gpointer data)
{	
	zerofc = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (zerofc)
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Frame counter will be reset each new start"));
	}
	else
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Frame counter absolute"));
	}
}   	

void cmd_tlenable_click(GtkWidget *widget, gpointer data)
{	
	tlenable = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (tlenable)
	{
		gtk_widget_set_sensitive(box_timelapse, 1);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Time lapse mode enabled"));
	}
	else
	{
		gtk_widget_set_sensitive(box_timelapse, 0);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Time lapse mode disabled"));
	}
}   	

void rbt_tlstart_click(GtkWidget *widget, gpointer data)
{	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
	{
		tlpick = 0;
	}
}   	

void rbt_tlend_click(GtkWidget *widget, gpointer data)
{	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
	{
		tlpick = 1;
	}
}   	

gboolean spn_tlhstart_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	char strdate[32];

	g_rw_lock_writer_lock(&thd_caplock);
	tlstart.tm_hour = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlstart);
	sprintf(imgmsg, C_("main","Time lapse date start set to: %s"), strdate);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tlmstart_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	char strdate[32];

	g_rw_lock_writer_lock(&thd_caplock);
	tlstart.tm_min = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlstart);
	sprintf(imgmsg, C_("main","Time lapse date start set to: %s"), strdate);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tlsstart_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	char strdate[32];

	g_rw_lock_writer_lock(&thd_caplock);
	tlstart.tm_sec = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlstart);
	sprintf(imgmsg, C_("main","Time lapse date start set to: %s"), strdate);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tlhend_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	char strdate[32];

	g_rw_lock_writer_lock(&thd_caplock);
	tlend.tm_hour = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlend);
	sprintf(imgmsg, C_("main","Time lapse date end set to: %s"), strdate);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tlmend_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	char strdate[32];

	g_rw_lock_writer_lock(&thd_caplock);
	tlend.tm_min = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlend);
	sprintf(imgmsg, C_("main","Time lapse date end set to: %s"), strdate);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tlsend_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	char strdate[32];

	g_rw_lock_writer_lock(&thd_caplock);
	tlend.tm_sec = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlend);
	sprintf(imgmsg, C_("main","Time lapse date end set to: %s"), strdate);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

void cmd_tlcalendar_click(GtkWidget *widget, gpointer data)
{	
	g_rw_lock_writer_lock(&thd_caplock);
	tlcalendar = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	g_rw_lock_writer_unlock(&thd_caplock);

	if (tlcalendar == 0)
	{
		gtk_button_set_label(GTK_BUTTON(widget), "Simple mode");
		gtk_widget_set_sensitive(rbt_tlstart, 0);
		gtk_widget_set_sensitive(lbl_tlstart, 0);
		gtk_widget_set_sensitive(spn_tlhstart, 0);
		gtk_widget_set_sensitive(spn_tlmstart, 0);
		gtk_widget_set_sensitive(spn_tlsstart, 0);
		gtk_widget_set_sensitive(rbt_tlend, 0);
		gtk_widget_set_sensitive(lbl_tlend, 0);
		gtk_widget_set_sensitive(spn_tlhend, 0);
		gtk_widget_set_sensitive(spn_tlmend, 0);
		gtk_widget_set_sensitive(spn_tlsend, 0);
		gtk_widget_set_sensitive(cal_tldpick, 0);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","TimeLapse set to use calendar driven start and stop plus interval"));
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(widget), "Full mode");
		gtk_widget_set_sensitive(rbt_tlstart, 1);
		gtk_widget_set_sensitive(lbl_tlstart, 1);
		gtk_widget_set_sensitive(spn_tlhstart, 1);
		gtk_widget_set_sensitive(spn_tlmstart, 1);
		gtk_widget_set_sensitive(spn_tlsstart, 1);
		gtk_widget_set_sensitive(rbt_tlend, 1);
		gtk_widget_set_sensitive(lbl_tlend, 1);
		gtk_widget_set_sensitive(spn_tlhend, 1);
		gtk_widget_set_sensitive(spn_tlmend, 1);
		gtk_widget_set_sensitive(spn_tlsend, 1);
		gtk_widget_set_sensitive(cal_tldpick, 1);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","TimeLapse set to use frame count driven start and stop plus interval"));
	}
}   

void cal_tldpick_dblclick (GtkCalendar *calendar, gpointer user_data)
{
	//int day;
	//g_object_get(G_OBJECT(calendar), "day", &day, NULL);
	//gtk_calendar_mark_day(calendar, day);
	time_t localt;
	char strdate[32];
	localt = time(NULL);
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (tlpick == 0)
	{
		tlstart = *(localtime(&localt));
		gtk_calendar_get_date(calendar, (guint *) &tlstart.tm_year, (guint *) &tlstart.tm_mon, (guint *) &tlstart.tm_mday);
		tlstart.tm_year -= 1900;
		strftime(strdate, 32, "%Y/%m/%d" ,&tlstart);
		gtk_label_set_text(GTK_LABEL(lbl_tlstart), (gchar *) strdate);
		strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlstart);
		sprintf(imgmsg, C_("main","Time lapse date start set to: %s"), strdate);
	}
	else if (tlpick == 1)
	{
		tlend = *(localtime(&localt));
		gtk_calendar_get_date(calendar, (guint *) &tlend.tm_year, (guint *) &tlend.tm_mon, (guint *) &tlend.tm_mday);
		tlend.tm_year -= 1900;
		strftime(strdate, 32, "%Y/%m/%d" ,&tlend);
		gtk_label_set_text(GTK_LABEL(lbl_tlend), (gchar *) strdate);
		strftime(strdate, 32, "%Y/%m/%d %H:%M:%S" ,&tlend);
		sprintf(imgmsg, C_("main","Time lapse date end set to: %s"), strdate);
	}
	g_rw_lock_writer_unlock(&thd_caplock);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

gboolean hsc_tlperiod_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_caplock);
	tlperiod = gtk_range_get_value(range);
	g_rw_lock_writer_unlock(&thd_caplock);

	sprintf(imgmsg, C_("main","Time Lapse period set to: %d"), tlperiod);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlperiod), tlperiod);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tlperiod_changed (GtkSpinButton *spinbutton, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_caplock);
	tlperiod = gtk_spin_button_get_value(spinbutton);
	g_rw_lock_writer_unlock(&thd_caplock);

	gtk_range_set_value(GTK_RANGE(hsc_tlperiod), tlperiod);
	return FALSE;
}

void txt_fitfolder_changed(GtkEditable *editable, gpointer user_data)
{
	strcpy(fitfolder, gtk_editable_get_chars(editable, 0, -1));
	sprintf(imgmsg, C_("main","Base folder/file set: %s%s"), fitfolder, fitbase);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

void txt_fitbase_changed(GtkEditable *editable, gpointer user_data)
{
	strcpy(fitbase, gtk_editable_get_chars(editable, 0, -1));
	sprintf(imgmsg, C_("main","Base folder/file set: %s%s"), fitfolder, fitbase);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

void cmd_dateadd_click(GtkWidget *widget, gpointer data)
{	
	fitdateadd = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (fitdateadd)
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Add date to naming convention selected"));
	}
	else
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Add date to naming convention removed"));
	}
}   	

void cmd_timeadd_click(GtkWidget *widget, gpointer data)
{	
	fittimeadd = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (fittimeadd)
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Add time to naming convention selected"));
	}
	else
	{
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","Add time to naming convention removed"));
	}
}   	

void cmd_fltadd_click(GtkWidget *widget, gpointer data)
{	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
	{
		gtk_widget_set_sensitive(cmb_flt, 1);
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(cmb_flt)) != -1)
		{
			strcpy(fitflt, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cmb_flt)));
		}
		else
		{
			fitflt[0] = '\0';
		}
		sprintf(imgmsg, C_("main","Add filter to naming convention selected"));
	}
	else
	{
		gtk_widget_set_sensitive(cmb_flt, 0);
		fitflt[0] = '\0';
		sprintf(imgmsg, C_("main","Add filter to naming convention removed"));
	}
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}   	

void cmb_flt_changed (GtkComboBox *widget, gpointer user_data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_fltadd)) == TRUE)
	{
		strcpy(fitflt, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
		if (strlen(fitflt) > 0)
		{
			sprintf(imgmsg, C_("main","Filter name: %s add to naming convention"), fitflt);
		}
		else
		{
			fitflt[0] = '\0';	
			sprintf(imgmsg, C_("main","Filter name removed from naming convention"));
		}
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
}

void cmb_fmt_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp;
	char str[32];
	
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if ((tmp >= 1) && (tmp <= 3))
		{
			savefmt = tmp;
		}
		else
		{
			savefmt = 1;
		}
		sprintf(imgmsg, C_("main","Save format set to: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
}

void cmd_tecenable_click(GtkWidget *widget, gpointer data)
{	
	static int error = 0, status = 0;
	
	status = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (error == 0)
	{
		if (imgcam_connected())
		{
			if (imgcam_get_tecp()->istec == 1)
			{
				if (status == 1)
				{
					if (tecrun == 0)
					{
						if (thd_tec != NULL)
						{
							g_thread_unref(thd_tec);
							thd_tec = NULL;
						}
						GError* thd_err = NULL;
						thd_tec = g_thread_try_new("Tec", thd_temp_run, NULL, &thd_err);
						tecrun = 1;
						if (thd_tec == NULL)
						{
							// Ui Message
							sprintf(imgmsg, C_("main","Tec controlling thread failed to start (%s)"), thd_err->message);
							g_error_free(thd_err);
							thd_err = NULL;
							gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
							// Disengage
							tecrun = 0;
							error = 1;
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
						}
						else
						{
							tec_init_graph();
							gtk_widget_set_sensitive(vsc_tecpwr, 1);
							gtk_widget_set_sensitive(cmd_tecauto, 1);
							gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Reading tec"));
						}
					}
				}
				else
				{
					if (tecrun == 1)
					{
						// Stop tec
						g_rw_lock_writer_lock(&thd_teclock);
						tecrun = 0;
						g_rw_lock_writer_unlock(&thd_teclock);
						// Wait for thread to end worst case (must improve this)
						usleep(500000);
						tec_init_graph();
						gtk_widget_set_sensitive(vsc_tecpwr, 0);
						gtk_widget_set_sensitive(cmd_tecauto, 0);
						gtk_widget_set_sensitive(spn_tectgt, 0);
						gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Enable tec read"));
						// Main image update
						tecfbk[0] = '\0';
						gtk_label_set_text(GTK_LABEL(lbl_fbktec), (gchar *) tecfbk);	
					}
				}
			}
			else if (imgcam_get_tecp()->istec == 2)
			{
				// Temp read only, no thread read allowed, in between frame capture
				tec_init_graph();
				gtk_widget_set_sensitive(vsc_tecpwr, 0);
				gtk_widget_set_sensitive(cmd_tecauto, 0);
				if (status == 1)
				{
					tecrun = 1;
					imgcam_get_tecp()->tecerr = 0;
					gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Reading tec"));
				}
				else
				{
					tecrun = 0;
					imgcam_get_tecp()->tecerr = 0;
					// Main image update
					tecfbk[0] = '\0';
					gtk_label_set_text(GTK_LABEL(lbl_fbktec), (gchar *) tecfbk);	
					gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Enable tec read"));
				}
			}
			else
			{
				if (status)
				{
					//Disengage (should not happen, but...)
					sprintf(imgmsg, C_("main","Camera %s does not have a controllable tec"), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cmb_camera)));
					gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
					error = 1;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
					gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Enable tec read"));
				}
			}
		}
	}
	else
	{
		error = 0;
	}
}   

void cmd_tecauto_click(GtkWidget *widget, gpointer data)
{
	static int status = 0;
	
	status = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (status)
	{
		gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Auto mode"));
		gtk_widget_set_sensitive(spn_tectgt, 1);
		g_rw_lock_writer_lock(&thd_teclock);
		imgcam_get_tecp()->tecauto = status;
		imgcam_get_tecp()->settemp = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spn_tectgt));
		sprintf(imgmsg, C_("main","Tec set auto to: %+06.2FC"), imgcam_get_tecp()->settemp);
		g_rw_lock_writer_unlock(&thd_teclock);		
		gtk_widget_set_sensitive(spn_tectgt, 1);
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(widget), C_("cooling","Manual mode"));
		gtk_widget_set_sensitive(spn_tectgt, 0);
		g_rw_lock_writer_lock(&thd_teclock);
		imgcam_get_tecp()->tecauto = status;
		g_rw_lock_writer_unlock(&thd_teclock);
		sprintf(imgmsg, C_("main","Tec set manual"));
		gtk_widget_set_sensitive(spn_tectgt, 0);
	}
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

gboolean spn_tectgt_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_teclock);
	imgcam_get_tecp()->settemp = gtk_spin_button_get_value(spinbutton);
	sprintf(imgmsg, C_("main","Tec set auto to: %+06.2FC"), imgcam_get_tecp()->settemp);
	g_rw_lock_writer_unlock(&thd_teclock);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean vsc_tecpwr_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	g_rw_lock_writer_lock(&thd_teclock);
	imgcam_get_tecp()->tecpwr = 255. * (gtk_range_get_value(range) / 100.);
	g_rw_lock_writer_unlock(&thd_teclock);

	if (tmrtecpwr != -1)
	{
		g_source_remove(tmrtecpwr);
	}
	tmrhstrefresh = g_timeout_add(500, (GSourceFunc) tmr_tecpwr, NULL);
	return FALSE;
}

void cmb_cfw_changed (GtkComboBox *widget, gpointer user_data)
{
	char str[32];
	int  tmp = 0;
	
	sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
	if (imgcfw_set_mode(tmp) == 1)
	{
		switch (tmp)
		{
			case 0:
				// GFW not in use, reset default filter list
				combo_setlist(cmb_flt, fltstr);
				break;
				
			case 1:
				// QHY Serial
				gtk_widget_set_sensitive(cmb_cfwtty, 1);
				combo_ttylist(cmb_cfwtty);
				gtk_widget_set_sensitive(cmd_cfwtty, 1);
				gtk_widget_set_sensitive(cmd_cfw, 1);
				gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				// This will read the configuration from the wheel itself
				break;
			
			case 99:
				// This is manufacturer specific so we load the list of choices 
				// from the camera UI.
				combo_setlist(cmb_cfwcfg, imgcam_get_camui()->whlstr);
				// Then only connect button and list of models are active
				gtk_widget_set_sensitive(cmd_cfw, 1);
				gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				break;
		
			default:
				// Deactivate all
				gtk_widget_set_sensitive(cmb_cfwcfg, 0);
				gtk_widget_set_sensitive(cmb_cfwtty, 0);
				gtk_widget_set_sensitive(cmd_cfwtty, 0);
				gtk_widget_set_sensitive(cmd_cfw, 0);
				break;				
		}
		sprintf(imgmsg, C_("cfw","Filter wheel mode set: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}	
	else
	{
		sprintf(imgmsg, C_("cfw","Could not set cfw mode"));
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		gtk_combo_box_set_active(widget, 0);
	}	
}

void cmb_cfwtty_changed (GtkComboBox *widget, gpointer user_data)
{
	if (gtk_widget_get_sensitive(GTK_WIDGET(widget)))
	{
		if (gtk_combo_box_get_active(widget) != -1)
		{
			imgcfw_set_tty(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
		}
		else
		{
			imgcfw_set_tty("");
		}	
		sprintf(imgmsg, C_("cfw","Filter wheel serial port: %s"), imgcfw_get_tty());
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
}

void cmd_cfwtty_click(GtkWidget *widget, gpointer data)
{
	combo_ttylist(cmb_cfwtty);
	sprintf(imgmsg, C_("cfw","Serial port list reloaded"));
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);	
}

void cmd_cfw_click(GtkWidget *widget, gpointer data)
{
	static int error = 0;
	
	if (error == 0)
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
		{
			// Not connected
			if (imgcfw_connect())
			{
				gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				gtk_widget_set_sensitive(cmd_cfwrst, (imgcfw_get_mode() == 1));
				combo_setlist(cmb_cfwcfg, imgcfw_get_models());
				gtk_button_set_label(GTK_BUTTON(widget), C_("cfw","Disconnect"));
				sprintf(imgmsg, C_("cfw","Filter wheel connected to %s"), imgcfw_get_tty());
			}
			else
			{
				error = 1;
				sprintf(imgmsg, "%s", imgcfw_get_msg());
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
			}
		}
		else
		{
			// Connected
			if (imgcfw_disconnect())
			{
				gtk_widget_set_sensitive(cmb_cfwcfg, 0);
				gtk_widget_set_sensitive(cmd_cfwrst, 0);
				gtk_button_set_label(GTK_BUTTON(widget), C_("cfw","Connect"));
				sprintf(imgmsg, C_("cfw","Filter wheel disconnected"));
			}
			else
			{
				error = 1;
				sprintf(imgmsg, "%s", imgcfw_get_msg());
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
			}
		}
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
	else
	{
		error = 0;
	}
}

void cmb_cfwcfg_changed (GtkComboBox *widget, gpointer user_data)
{
	int i = 0;
	
	if ((gtk_combo_box_get_active(widget) != -1) && (gtk_widget_get_sensitive(GTK_WIDGET(widget))))
	{
		imgcfw_set_model(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
		for (i = 0; i < CFW_SLOTS; i++)
		{
			gtk_widget_set_sensitive(cmb_cfwwhl[i], (i < imgcfw_get_slotcount()));
			gtk_widget_set_sensitive(cmd_cfwwhl[i], (i < imgcfw_get_slotcount()));
		}
		sprintf(imgmsg, C_("cfw","Filter wheel configuration: %d slots, %s model"), imgcfw_get_slotcount(), imgcfw_get_model());
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
}

void cmd_cfwrst_click(GtkWidget *widget, gpointer data)
{
	if (imgcfw_reset())
	{
		sprintf(imgmsg, C_("cfw","Filter wheel controller factory reset done"));
	}
	else
	{
		sprintf(imgmsg, "%s", imgcfw_get_msg());
	}
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);	
}

void cmb_cfwwhl_changed (GtkComboBox *widget, GtkWidget **awidget)
{
	char cfwfltstr[256];
	int  i;

	if ((gtk_combo_box_get_active(widget) != -1) && (gtk_widget_get_sensitive(GTK_WIDGET(widget))))
	{
		// If current combo has a meaningful value, recalc cmb_flt content
		cfwfltstr[0] = '\0';
		for (i = 0; i < imgcfw_get_slotcount(); i++)
		{
			if ((gtk_combo_box_get_active(GTK_COMBO_BOX(awidget[i])) != -1) && (gtk_widget_get_sensitive(GTK_WIDGET(awidget[i]))))
			{
				strcat(cfwfltstr, "|");
				strcat(cfwfltstr, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(awidget[i])));
			}
		}
		combo_setlist(cmb_flt, cfwfltstr);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_flt), imgcfw_get_slot());
	}
}

void cmd_cfwwhl_click (GtkComboBox *widget, gpointer user_data)
{
	//printf("Got value: %d\n", (int)user_data);
	//imgcfw_set_slot((int)user_data, NULL);
	if (imgcfw_set_slot((int)user_data, (gpointer) cfwmsgdestroy))
	{	
		// Show the change slot message
		cfwmsg = gtk_message_dialog_new ((GtkWindow *) window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, C_("cfw","Please wait for the filter to reach position..."));	
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(cfwmsg), C_("cfw","Dialog will disappear when done"));
		//gtk_window_set_decorated(GTK_WINDOW(cfwmsg), FALSE);
		gtk_window_set_deletable(GTK_WINDOW(cfwmsg), FALSE);
		gtk_window_set_keep_above(GTK_WINDOW(cfwmsg), TRUE);
		gtk_widget_show_all(cfwmsg);
	}
	sprintf(imgmsg, "%s", imgcfw_get_msg());
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

