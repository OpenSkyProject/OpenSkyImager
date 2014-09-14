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
	const gchar* wdgname;
	
	wdgname = gtk_widget_get_name(widget);
	gtk_statusbar_remove_all(GTK_STATUSBAR(widget), 0);
	
	if (strcmp(wdgname, "imgstatus") == 0)
	{
		tmrstatusbar = -1;
	}
	else if (strcmp(wdgname, "imgstatec") == 0)
	{
		tmrstatustec = -1;
	}
	else if (strcmp(wdgname, "imgstafit") == 0)
	{
		tmrstatusfit = -1;
	}
	
	//free(wdgname);
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_imgstatus_pixmsg (GtkWidget *widget)
{
	
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgpix_get_msg());
	tmrfrmrefresh = -1;
	
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

gboolean tmr_capture_progress_refresh (int *readoutok)
{
	double tmpshots, tmpfract, tmperr, tmprun, tmpfps;

	g_rw_lock_reader_lock(&thd_caplock);
	tmpshots = shots;
	tmpfract = shotfract;
	tmperr = runerr;
	tmprun = run;
	tmpfps = fps;
	g_rw_lock_reader_unlock(&thd_caplock);

	//printf("Readout: %d\n", *readoutok);

	if (*readoutok == 1)
	{	
		if (tmpfps < 10)
		{
			sprintf(fpsfbk, "Fps:%05.1F", (1./ tmpfps));
		}
		else if (tmpfps >= 10)
		{
			sprintf(fpsfbk, "Fpm:%05.1F", (60. / tmpfps));
		}
		else
		{
			fpsfbk[0] = '\0';
		}
	}
	else
	{	
		gtk_statusbar_write(GTK_STATUSBAR(imgstafit), 0, C_("camio","Bad data received, discarded"));
		strcpy(fpsfbk, "--");	
	}

	if (tmprun == 1)
	{

		if ((fwhmv == 1) && (*readoutok == 1))
		{
			fwhm_show();
		}
	
		if (capture)
		{
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_shots), (int)tmpshots);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbr_expnum), tmpfract);
			if (*readoutok == 1)
			{
				if (savefmt == 1)
				{
					/// Message on statusbar about last saved frame (%s)
					sprintf(imgmsg, C_("main","Image: %s saved"), g_path_get_basename(imgfit_get_name()));
				}
				else if ((savefmt == 2) || (savefmt == 4))
				{
					/// Message on statusbar about last frame add to avi (%s)
					sprintf(imgmsg, C_("main","Frame: add to %s"), g_path_get_basename(imgavi_get_name()));
				}
				else if (savefmt == 3)
				{
					/// Message on statusbar about last saved frame (%s) + last frame add to avi (%s)
					sprintf(imgmsg, C_("main","Image: %s saved, Frame: add to %s"), g_path_get_basename(imgfit_get_name()), g_path_get_basename(imgavi_get_name()));
				}
				gtk_statusbar_write(GTK_STATUSBAR(imgstafit), 0, imgmsg);
			}
			// Main image update
			sprintf(imgfbk, "#%04d", (int)tmpshots);
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), (gchar *) imgfbk);	
			gtk_label_set_text(GTK_LABEL(lbl_fbkfps), (gchar *) fpsfbk);	
		}
		else
		{
			if (*readoutok == 1)
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
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
	else
	{
		// Run end with no error
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_shots), (int)tmpshots);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbr_expnum), tmpfract);
		if (capture)
		{
			if (*readoutok == 1)
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
			}
			sprintf(imgmsg, C_("main","capture end"));
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
			// Main image update
			sprintf(imgfbk, "#%04d", (int)tmpshots);
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), (gchar *) imgfbk);	
		}
		else
		{
			/// Message to confirm end of capture thread in focus mode
			strcpy(imgmsg, C_("main","Focus end"));
			gtk_label_set_text(GTK_LABEL(lbl_fbkimg), "");	
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		}
		gtk_label_set_text(GTK_LABEL(lbl_fbkfps), (gchar *) fpsfbk);	
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_run)) == TRUE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_run), FALSE);
		}
	}
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
	g_rw_lock_writer_lock(&thd_teclock);
	imgcam_get_tecp()->tecedit = 1;
	g_rw_lock_writer_unlock(&thd_teclock);
	tmrtecpwr = -1;
	
	// Change to TRUE for a recurring timer
	return FALSE;
}

gboolean tmr_tecstatus_write (GtkWidget *widget)
{
	static double oldT;
	int setwait = 0, suspect = 0;
	int pct = 0;
	
	if (tecrun)
	{
		if (imgcam_get_tecp()->istec == 1)
		{	
			// This is to prevent interrupt read during image readout
			// The capture thread will lock for write
			if (g_rw_lock_writer_trylock(&thd_teclock) == TRUE)
			{	
				//Set the loop reference
				oldT = imgcam_get_tecp()->tectemp;

				if (imgcam_gettec(&imgcam_get_tecp()->tectemp, NULL, NULL, NULL))
				{
					//printf("Temp: %f\n", imgcam_get_tecp()->tectemp);
					if (imgcam_get_tecp()->tecauto)
					{
						if (setwait == 0)
						{
							if (fabs(oldT - imgcam_get_tecp()->tectemp) == 0 && suspect < 3)
							{
								//This is suspect... noop
								suspect++;
							}
							else if (fabs(imgcam_get_tecp()->tectemp - imgcam_get_tecp()->settemp) < 2.)
							{
								suspect = 0;
								// Temp is almost stabilized near to target, we do tiny corrections
								if ((oldT - imgcam_get_tecp()->tectemp) < 0.03 && imgcam_get_tecp()->tectemp > imgcam_get_tecp()->settemp)
								{
									// If temp is not moving or not the right direction
									if (imgcam_get_tecp()->tectemp > (imgcam_get_tecp()->settemp + 0.7) ) 
									{
										imgcam_get_tecp()->tecpwr += 2;
										imgcam_get_tecp()->tecpwr = MIN(imgcam_get_tecp()->tecpwr, imgcam_get_tecp()->tecmax);
										if (imgcam_get_tecp()->tectemp > oldT)
										{
											//Still going wrong direction
											setwait = 3;
										}
										else
										{
											setwait = 6;
										}
									}
									else if (imgcam_get_tecp()->tectemp > (imgcam_get_tecp()->settemp + 0.2)) 
									{
										imgcam_get_tecp()->tecpwr += 1;
										imgcam_get_tecp()->tecpwr = MIN(imgcam_get_tecp()->tecpwr, imgcam_get_tecp()->tecmax);
										if (imgcam_get_tecp()->tectemp > oldT)
										{
											//Still going wrong direction
											setwait = 1;
										}
										else
										{
											setwait = 3;
										}
									}
								}
								else if ((imgcam_get_tecp()->tectemp - oldT) < 0.03 && imgcam_get_tecp()->tectemp < imgcam_get_tecp()->settemp)
								{
									// If temp is not moving or not the right direction
									if (imgcam_get_tecp()->tectemp < (imgcam_get_tecp()->settemp - 0.7) ) 
									{
										imgcam_get_tecp()->tecpwr -= 2;
										imgcam_get_tecp()->tecpwr = MAX(imgcam_get_tecp()->tecpwr, 0);
										if (imgcam_get_tecp()->tectemp < oldT)
										{
											//Still going wrong direction
											setwait = 3;
										}
										else
										{
											setwait = 6;
										}
									}
									else if (imgcam_get_tecp()->tectemp < (imgcam_get_tecp()->settemp - 0.2)) 
									{
										imgcam_get_tecp()->tecpwr -= 1;
										imgcam_get_tecp()->tecpwr = MAX(imgcam_get_tecp()->tecpwr, 0);
										if (imgcam_get_tecp()->tectemp < oldT)
										{
											//Still going wrong direction
											setwait = 1;
										}
										else
										{
											setwait = 3;
										}
									}
								}
							}
							else if (imgcam_get_tecp()->settemp < imgcam_get_tecp()->tectemp) 
							{
								suspect = 0;
								if ((oldT - imgcam_get_tecp()->tectemp) < (0.05 * (tecspeed +1)))
								{
									//setTemp is still far. We gently pull tec up or down
									imgcam_get_tecp()->tecpwr += 6;
									imgcam_get_tecp()->tecpwr = MIN(imgcam_get_tecp()->tecpwr, imgcam_get_tecp()->tecmax);
									if (imgcam_get_tecp()->tectemp > oldT)
									{
										//Still going wrong direction
										setwait = 1;
									}
									else
									{
										setwait = 3;
									}
								}
							}
							else if (imgcam_get_tecp()->settemp > imgcam_get_tecp()->tectemp) 
							{
								suspect = 0;
								if ((imgcam_get_tecp()->tectemp - oldT) < (0.05 * (tecspeed +1)))
								{
									imgcam_get_tecp()->tecpwr -= 6;
									imgcam_get_tecp()->tecpwr = MAX(imgcam_get_tecp()->tecpwr, 0);
									if (imgcam_get_tecp()->tectemp < oldT)
									{
										//Still going wrong direction
										setwait = 1;
									}
									else
									{
										setwait = 3;
									}
								}
							}
							if (setwait)
							{
								imgcam_settec(imgcam_get_tecp()->tecpwr, -1);
							}
						}
						else
						{
							setwait--;
						}
					}
					else
					{
						imgcam_settec(imgcam_get_tecp()->tecpwr, -1);
					}
					imgcam_get_tecp()->tecedit = 0;
				}
				g_rw_lock_writer_unlock(&thd_teclock);
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
					// Header update
					fithdr[HDR_CCDTEMP].dvalue = round(imgcam_get_tecp()->tectemp * 100) / 100;
					fithdr[HDR_SETTEMP].dvalue = round(imgcam_get_tecp()->settemp * 100) / 100;
				}
				else
				{
					imgcam_get_tecp()->tecerr = 0;
					sprintf(imgmsg, C_("main","Error communicating with tec"));
				}
				gtk_statusbar_write(GTK_STATUSBAR(imgstatec), 0, imgmsg);
			}
			else
			{
				// 50ms retry
				if (tmrtecrefresh != -1)
				{
					g_source_remove(tmrtecrefresh);
				}
				tmrtecrefresh = g_timeout_add(50, (GSourceFunc) tmr_tecstatus_write, NULL);	
				return FALSE;
			}
			// Virtual recurring
			tmrtecrefresh = g_timeout_add_seconds(5, (GSourceFunc) tmr_tecstatus_write, NULL);
			return FALSE;
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
			gtk_statusbar_write(GTK_STATUSBAR(imgstatec), 0, imgmsg);
			// Header update
			fithdr[HDR_CCDTEMP].dvalue = round(imgcam_get_tecp()->tectemp * 100) / 100;
			tmrtecrefresh = -1;
		}
		else if (imgcam_get_tecp()->istec == 3)
		{
			// SBIG mode
			int enabled = 0;
			// This is to prevent interrupt read during image readout
			// The capture thread will lock for write
			if (g_rw_lock_writer_trylock(&thd_teclock) == TRUE)
			{	
				// See if we have to set
				if (imgcam_get_tecp()->tecedit)
				{
					if (imgcam_get_tecp()->tecauto)
					{
						// Auto
						imgcam_settec(imgcam_get_tecp()->settemp, 1);							
					}
					else
					{
						// Manual
						imgcam_settec(imgcam_get_tecp()->tecpwr, 2);							
					}
					g_rw_lock_writer_unlock(&thd_teclock);
					pct = (int)(((double)imgcam_get_tecp()->tecpwr / (double)imgcam_get_tecp()->tecmax) * 100.);
				}
				else
				{
					// Try read
					imgcam_gettec(&imgcam_get_tecp()->tectemp, &imgcam_get_tecp()->settemp, &imgcam_get_tecp()->tecpwr, &enabled);
					g_rw_lock_writer_unlock(&thd_teclock);

					if (imgcam_get_tecp()->tecerr == 0)
					{
						// Set the gui according to status returned from camera
						if (imgcam_get_tecp()->tecauto)
						{
							if (gtk_spin_button_get_value(GTK_SPIN_BUTTON(spn_tectgt)) != imgcam_get_tecp()->settemp)
							{
								// Set tec target
								gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tectgt), imgcam_get_tecp()->settemp);
							}
						}
						pct = (int)(((double)imgcam_get_tecp()->tecpwr / (double)imgcam_get_tecp()->tecmax) * 100.);
					}
				}
				if (imgcam_get_tecp()->tecauto)
				{
					// Statusbar feedback message about cooling status in automatic mode
					sprintf(imgmsg, C_("main","Tec: %+06.2FC, Target: %+06.2FC, Power: %d%%"), imgcam_get_tecp()->tectemp, imgcam_get_tecp()->settemp, pct);
				}
				else
				{
					// Satusbar feedback message about cooling in manual mode
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
				imgcam_get_tecp()->tecedit = 0;
				// Header update
				fithdr[HDR_CCDTEMP].dvalue = round(imgcam_get_tecp()->tectemp * 100) / 100;
				fithdr[HDR_SETTEMP].dvalue = round(imgcam_get_tecp()->settemp * 100) / 100;
				gtk_statusbar_write(GTK_STATUSBAR(imgstatec), 0, imgmsg);
			}
			else
			{
				// 50ms retry
				if (tmrtecrefresh != -1)
				{
					g_source_remove(tmrtecrefresh);
				}
				tmrtecrefresh = g_timeout_add(50, (GSourceFunc) tmr_tecstatus_write, NULL);	
				return FALSE;
			}
			// Virtual recurring
			tmrtecrefresh = g_timeout_add_seconds(5, (GSourceFunc) tmr_tecstatus_write, NULL);
			return FALSE;
		}
	}
	else
	{
		//printf("Stop\n");
		tmrtecrefresh = -1;
	}
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
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	
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
	tmrstatusbar = g_timeout_add_seconds(3, (GSourceFunc) tmr_imgstatus_wipe, GTK_WIDGET(statusbar));
}

void imgstafit_push (GtkStatusbar *statusbar, guint context_id, gchar *text, gpointer user_data)
{
	if (tmrstatusfit != -1)
	{
		g_source_remove(tmrstatusfit);
	}
	tmrstatusfit = g_timeout_add_seconds(3, (GSourceFunc) tmr_imgstatus_wipe, GTK_WIDGET(statusbar));
}

void imgstatec_push (GtkStatusbar *statusbar, guint context_id, gchar *text, gpointer user_data)
{
	if (tmrstatustec != -1)
	{
		g_source_remove(tmrstatustec);
	}
	tmrstatustec = g_timeout_add_seconds(3, (GSourceFunc) tmr_imgstatus_wipe, GTK_WIDGET(statusbar));
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
	gchar* authors[]     = { "Giampiero Spezzano ", "Clive Rogers ", "Dan Holler ", "Fabrice Phung", "Andrew Stepanenko", "Max Chen","Contributors are always welcome ", NULL };
	gchar* artists[]     = { "Wanted, a fancy icon and logo would be great! ", NULL };
	const gchar* translators   = "Giampiero Spezzano (IT), Fabrice Phung (FR-DE), Max Chen (CN)";
	gchar* documenters[] = { "Clive Rogers", NULL };	
	const gchar* comments      = C_("about","OpenSkyImager is a capture program written for Astronomy camera operation");
	const gchar* copyright     = C_("about","Copyright (c) 2013 JP & C AstroSoftware\n\nLicensed under GNU GPL 3.0\n\nThis program is free software: you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of the License, or\nany later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.");
	gchar* name          = APPNAM;
	gchar* version       = APPVER;
	gchar* website       = "https://github.com/OpenSkyProject/OpenSkyImager";
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

			// when in focus mode we may need to set something to get more speed
			imgcam_get_expar()->preview = (capture == 0)? 1: 0;
			imgcam_get_expar()->edit = 1;
			// 

			g_rw_lock_writer_unlock(&thd_caplock);

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
				// when in capture mode we restore previous mode (for ccd only, regardless of speed)
				if ((imgcam_get_tecp()->istec == 1) && (tecprerun == 1))
				{
					tecprerun = 0;
					gtk_widget_set_sensitive(cmd_tecenable, 1);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), TRUE);
				}
				gtk_widget_set_sensitive(box_filename, 0);
				gtk_widget_set_sensitive(box_cfw, 0);
				fwhm_hide();
				// Redraw icon
				if (hst == 0)
				{
					load_histogram_from_null();
				}	
				//
				gtk_widget_set_sensitive(cmd_focus, 1);
				gtk_widget_set_sensitive(cmd_capture, 0);
				error = 1;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_focus), FALSE);
			}
			else
			{
				// when in focus mode and fast speed we need to stop temp read (for ccd only)
				if ((imgcam_get_expar()->speed > 0) && (imgcam_get_tecp()->istec == 1) && (tecrun == 1))
				{
					tecprerun = 1;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecdisable), TRUE);
					gtk_widget_set_sensitive(cmd_tecenable, 0);
					gtk_widget_set_sensitive(cmd_tecdisable, 0);
				}
				gtk_widget_set_sensitive(box_filename, 1);
				gtk_widget_set_sensitive(box_cfw, 1);
				//
				gtk_widget_set_sensitive(cmd_focus, 0);
				gtk_widget_set_sensitive(cmd_capture, 1);
				error = 1;
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_capture), FALSE);
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
	
	if (data == NULL)
	{
		get_filename(&filename, 0, "*.fit");
	}
	else if (isfile(data))
	{
		filename = (char*)g_malloc(strlen(data));
		strcpy(filename, data);
	}

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
	int brun = 0, bexpose = 0, breadout = 0;
	
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
		printf("tsize   : %d\n", imgcam_get_shpar()->tsize);
		printf("preview : %d\n", imgcam_get_shpar()->preview);*/
		if (imgcam_connected())
		{
			g_rw_lock_reader_lock(&thd_caplock);
			brun = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)? 1: 0;
			bexpose = expose;
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
				if ((breadout) || (bexpose))
				{
					//printf("Expose: %d, Readout: %d\n", bexpose, breadout);
					// Long exposure running; abort
					if ((imgcam_get_shpar()->time > 1000) && (breadout == 0))
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
					run  = 2;
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
		else
		{
			kill = 0;
			gtk_button_set_label(GTK_BUTTON(widget), C_("main","Start"));
			gtk_widget_modify_bkg(widget, GTK_STATE_ACTIVE, &clrSelected);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_hold), FALSE);
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
			#if GTK_CHECK_VERSION(3,4,2)
				GdkScrollDirection direction;
				if (event->delta_y != 0) 
				{
					direction = (event->delta_y >= 0) ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
				}
				else
				{
					direction = event->direction;
				}
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
		
			g_rw_lock_reader_lock(&thd_caplock);
			// Draw roi
			fwhm_show();
			// Calc
			fwhm_calc();
			// Draw roi after possible calc move
			fwhm_show();
			// Redraw icon
			if (hst == 0)
			{
				load_histogram_from_null();
			}	
			g_rw_lock_reader_unlock(&thd_caplock);
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
		// Left click
		roix = ((fwhmx - (fwhms / 2)) / imgratio);
		roiy = ((fwhmy - (fwhms / 2)) / imgratio);

		if (((event->x > roix) && (event->x < (roix + roisize))) && ((event->y > roiy) && (event->y < (roiy + roisize))) && (fwhmv == 1))
		{
			fwhm_hide();
			// Redraw icon
			if (hst == 0)
			{
				load_histogram_from_null();
			}	
		}
		else
		{
			g_rw_lock_reader_lock(&thd_caplock);
			// Center on image data regardless of "fit to screen"
			fwhm_center(event->x, event->y, 0);
			// Draw roi
			fwhm_show();
			// Calc
			fwhm_calc();
			// Draw roi after possible calc move
			fwhm_show();
			// Redraw icon
			if (hst == 0)
			{
				load_histogram_from_null();
			}	
			g_rw_lock_reader_unlock(&thd_caplock);
		}
	}
	/*else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
	{
		// Right click
	}*/
	return FALSE;
}

void mainw_destroy( GtkWidget *widget, gpointer   data )
{
	gtk_main_quit ();
}

gboolean mainw_delete_event( GtkWidget *widget, GdkEvent *event, gpointer data)
{
	int retval = TRUE;

	if (run)
	{
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, C_("main","Capture thread running!"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog), C_("main","Confirm only if you know exaclty what you're doing"));
		gint result =  gtk_dialog_run (GTK_DIALOG (dialog));

		switch (result)
		{
			case GTK_RESPONSE_OK:
				// Brute force thread end
				g_rw_lock_writer_lock(&thd_caplock);
				run  = 0;
				runerr = 1;
				if ((readout) || (expose))
				{
					imgcam_abort();
				}
				readout = 0;
				expose = 0;
				g_rw_lock_writer_unlock(&thd_caplock);
				imgcam_end();
				break;
			default:
				break;
		}
		gtk_widget_destroy(dialog);
	}
	if (run == 0)
	{
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
				imgcam_end();
				retval = FALSE;
				break;
			default:
				retval = TRUE;
				break;
		}
		gtk_widget_destroy(dialog);
	}
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
	float tmp = 0;
	
	g_rw_lock_writer_lock(&thd_caplock);
	sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%f", &tmp);
	if (tmp > 0) 
	{
		imgcam_get_expar()->time = (int) (tmp * 1000);
		fithdr[HDR_EXPTIME].dvalue = (double)(imgcam_get_expar()->time/1000.);
		fithdr[HDR_EXPOSURE].dvalue = (double)(imgcam_get_expar()->time/1000.);
	}
	else
	{
		imgcam_get_expar()->time = 1;
		fithdr[HDR_EXPTIME].dvalue = 0.001;
		fithdr[HDR_EXPOSURE].dvalue = 0.001;
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
			if (run)
			{
				GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, C_("main","Capture thread running!"));
				gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog), C_("main","Confirm only if you know exaclty what you're doing"));
				gint result =  gtk_dialog_run (GTK_DIALOG (dialog));
	
				switch (result)
				{
					case GTK_RESPONSE_OK:
						// Brute force thread end
						g_rw_lock_writer_lock(&thd_caplock);
						run  = 0;
						runerr = 1;
						if ((readout) || (expose))
						{
							imgcam_abort();
						}
						readout = 0;
						expose = 0;
						g_rw_lock_writer_unlock(&thd_caplock);
						break;
					default:
						break;
				}
				gtk_widget_destroy(dialog);
			}
			if (run == 0)
			{
				if (imgcam_get_tecp()->istec != 0)
				{
					// Signal term to tec timer and disable choice
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_tecauto)))
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecmanual), TRUE);
					}
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecdisable), TRUE);
					gtk_widget_set_sensitive(cmd_tecenable, 0);
					gtk_widget_set_sensitive(cmd_tecdisable, 0);
				}
				if (strlen(imgcam_get_camui()->whlstr) > 0)
				{
					// Delete In-camera Wheel choice is there's one
					if (imgcfw_get_mode() == 99)
					{
						if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)))
						{
							// Disconnect to get the gui consistent
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_cfw), FALSE);
						}
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
				//Engage
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
				strcpy(fithdr[HDR_INSTRUME].svalue, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cmb_camera)));
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
				// Header values
				if (imgcam_get_camui()->pszx > 0)
				{
					fithdr[HDR_PSZX].dtype  = 'F';
					fithdr[HDR_PSZX].dvalue = imgcam_get_camui()->pszx;
					fithdr[HDR_PSZY].dtype  = 'F';
					fithdr[HDR_PSZY].dvalue = imgcam_get_camui()->pszy;
				}
				else
				{
					fithdr[HDR_PSZX].dtype  = '\0';
					fithdr[HDR_PSZY].dtype  = '\0';
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
				if (imgcam_get_tecp()->istec != 0)
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
	fithdr[HDR_GAIN].ivalue = imgcam_get_expar()->gain;
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
	fithdr[HDR_OFFSET].ivalue = imgcam_get_expar()->offset;
	imgcam_get_expar()->edit = 1;
	sprintf(imgmsg, C_("main","Offset set to: %d"), imgcam_get_expar()->offset);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	g_rw_lock_writer_unlock(&thd_caplock);
	return FALSE;
}

void cmb_bin_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp = 0;
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%dx", &tmp);
		if (tmp >= 0) 
		{
			imgcam_get_expar()->bin = tmp;
			fithdr[HDR_XBINNING].ivalue = tmp;
			fithdr[HDR_YBINNING].ivalue = tmp;
			fithdr[HDR_PSZX].dvalue = round(imgcam_get_camui()->pszx * tmp * 100) / 100;
			fithdr[HDR_PSZY].dvalue = round(imgcam_get_camui()->pszy * tmp * 100) / 100;
			imgcam_get_expar()->edit = 1;
			sprintf(imgmsg, C_("main","Binning mode set to: %dx%d"), imgcam_get_expar()->bin, imgcam_get_expar()->bin);
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		}
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_csize_changed (GtkComboBox *widget, gpointer user_data)
{
	int w = 0, h = 0;
	
	g_rw_lock_writer_lock(&thd_caplock);
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%dx%d", &w, &h);
		if (w > 0 && h > 0) 
		{
			imgcam_get_expar()->width  = w;
			imgcam_get_expar()->height = h;
			sprintf(imgmsg, C_("main","Capture size set to: %dx%d"), imgcam_get_expar()->width, imgcam_get_expar()->height);
		}
		else
		{
			imgcam_get_expar()->width  = 0;
			imgcam_get_expar()->height = 0;
			sprintf(imgmsg, C_("main","Capture size set to: Full frame"));
		}
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_dspeed_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
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
		if ((capture == 0) && (imgcam_get_expar()->speed > 0) && (imgcam_get_tecp()->istec == 1) && (tecrun == 1))
		{
			// when in focus mode and fast speed we need to stop temp read (for ccd only)
			tecprerun = 1;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecdisable), TRUE);
			gtk_widget_set_sensitive(cmd_tecenable, 0);
		}
		else if ((imgcam_get_tecp()->istec == 1) && (tecprerun == 1))
		{
			// when in capture mode or slow speed we restore previous mode (for ccd only, regardless of speed)
			tecprerun = 0;
			gtk_widget_set_sensitive(cmd_tecenable, 1);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), TRUE);
		}
		sprintf(imgmsg, C_("main","Download speed set to: %s"), str);
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		imgcam_get_expar()->edit = 1;
	}
	g_rw_lock_writer_unlock(&thd_caplock);
}

void cmb_mode_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
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
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
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
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
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
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
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
		// Start recurring timer to refresh the message
		// Timer will kill itself if !tlenable || !run
		if (tmrtlrefresh != -1)
		{
			g_source_remove(tmrtlrefresh);
		}
		tmrtlrefresh = g_timeout_add_seconds(5, (GSourceFunc) tmr_tlrefresh, NULL);			
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
		gtk_button_set_label(GTK_BUTTON(widget), "Full mode");
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
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","TimeLapse set to use frame count driven start and stop plus interval"));
	}
	else
	{
		gtk_button_set_label(GTK_BUTTON(widget), "Simple mode");
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
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, C_("main","TimeLapse set to use calendar driven start and stop plus interval"));
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
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
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
			strcpy(fithdr[HDR_FILTER].svalue,fitflt);
			gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
		}
	}
}

void cmb_fmt_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != -1)
	{
		sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
		if ((tmp >= 1) && (tmp <= 4))
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
	
	if (error == 0)
	{
		if (imgcam_connected())
		{
			if ((imgcam_get_tecp()->istec == 1) || (imgcam_get_tecp()->istec == 3))
			{
				status = (status == 0) ? 1 : 0;
				if (status == 1)
				{
					// Activate ccdtemp header entry
					fithdr[HDR_CCDTEMP].dtype = 'F';
					fithdr[HDR_SETTEMP].dtype = 'F';
					if (tmrtecrefresh == -1)
					{
						// If there's no one running, run it
						tmrtecrefresh = g_timeout_add_seconds(5, (GSourceFunc) tmr_tecstatus_write, NULL);	
					}
					if (tmrtecrefresh != -1)
					{
						// If it's running
						tecrun = 1;	
						tec_init_graph();
						if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_tecauto)))
						{
							gtk_widget_set_sensitive(cmd_tecmanual, 1);
							gtk_widget_set_sensitive(spn_tectgt, 1);
							if (imgcam_get_tecp()->istec == 1)
							{
								gtk_widget_set_sensitive(spn_tecspd, 1);
								gtk_widget_set_sensitive(vsc_tecpwr, 1);
							}
							else
							{
								gtk_widget_set_sensitive(spn_tecspd, 0);
								gtk_widget_set_sensitive(vsc_tecpwr, 0);		
							}
						}
						else
						{
							gtk_widget_set_sensitive(vsc_tecpwr, 1);
							gtk_widget_set_sensitive(cmd_tecauto, 1);
						}
						// Toggle
						gtk_widget_set_sensitive(cmd_tecenable, 0);						
						gtk_widget_set_sensitive(cmd_tecdisable, 1);						
						error = 1;
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecdisable), FALSE);
					}
					else
					{
						// Ui Message
						sprintf(imgmsg, C_("main","Tec controlling thread failed to start (%s)"), "");
						gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
						// Disengage
						tecrun = 0;
						error = 1;
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
					}
				}
				else
				{
					// De-activate ccdtemp header entry
					fithdr[HDR_CCDTEMP].dtype = '\0';
					fithdr[HDR_SETTEMP].dtype = '\0';
					if (tmrtecrefresh != -1)
					{
						// Stop tec
						//g_source_remove(tmrtecrefresh);
						//tmrtecrefresh = -1;
						tecrun = 0;	
						// Ui update
						tec_init_graph();
						gtk_widget_set_sensitive(vsc_tecpwr, 0);
						gtk_widget_set_sensitive(cmd_tecauto, 0);
						gtk_widget_set_sensitive(cmd_tecmanual, 0);
						gtk_widget_set_sensitive(spn_tectgt, 0);
						gtk_widget_set_sensitive(spn_tecspd, 0);
						// Main image update
						tecfbk[0] = '\0';
						gtk_label_set_text(GTK_LABEL(lbl_fbktec), (gchar *) tecfbk);	
						// Toggle
						gtk_widget_set_sensitive(cmd_tecenable, 1);						
						gtk_widget_set_sensitive(cmd_tecdisable, 0);						
						error = 1;
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), FALSE);
					}
				}
			}
			else if (imgcam_get_tecp()->istec == 2)
			{
				status = (status == 0) ? 1 : 0;
				// Temp read only, no thread read allowed, in between frame capture
				tec_init_graph();
				gtk_widget_set_sensitive(vsc_tecpwr, 0);
				gtk_widget_set_sensitive(cmd_tecauto, 0);
				if (status == 1)
				{
					tecrun = 1;
					imgcam_get_tecp()->tecerr = 0;
					// Activate ccdtemp header entry
					fithdr[HDR_CCDTEMP].dtype = 'F';
					fithdr[HDR_SETTEMP].dtype = '\0';
					// Toggle
					gtk_widget_set_sensitive(cmd_tecenable, 0);						
					gtk_widget_set_sensitive(cmd_tecdisable, 1);						
					error = 1;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecdisable), FALSE);
				}
				else
				{
					tecrun = 0;
					imgcam_get_tecp()->tecerr = 0;
					// Main image update
					tecfbk[0] = '\0';
					gtk_label_set_text(GTK_LABEL(lbl_fbktec), (gchar *) tecfbk);	
					// De-activate ccdtemp header entry
					fithdr[HDR_CCDTEMP].dtype = '\0';
					fithdr[HDR_SETTEMP].dtype = '\0';
					// Toggle
					gtk_widget_set_sensitive(cmd_tecenable, 1);						
					gtk_widget_set_sensitive(cmd_tecdisable, 0);						
					error = 1;
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), FALSE);
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
	static int error = 0, status = 0;
	
	//status = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE);
	if (error == 0)
	{
		status = (status == 0) ? 1 : 0;
		if (status)
		{
			gtk_widget_set_sensitive(spn_tectgt, 1);
			if (imgcam_get_tecp()->istec == 1)
			{
				gtk_widget_set_sensitive(spn_tecspd, 1);
				gtk_widget_set_sensitive(vsc_tecpwr, 1);
			}
			else
			{
				gtk_widget_set_sensitive(spn_tecspd, 0);
				gtk_widget_set_sensitive(vsc_tecpwr, 0);		
			}
			g_rw_lock_reader_lock(&thd_teclock);
			imgcam_get_tecp()->tecauto = status;
			imgcam_get_tecp()->settemp = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spn_tectgt));
			imgcam_get_tecp()->tecedit = 1;
			sprintf(imgmsg, C_("main","Tec set auto to: %+06.2FC"), imgcam_get_tecp()->settemp);
			g_rw_lock_reader_unlock(&thd_teclock);		
			gtk_widget_set_sensitive(spn_tectgt, 1);
			// Toggle
			gtk_widget_set_sensitive(cmd_tecauto, 0);						
			gtk_widget_set_sensitive(cmd_tecmanual, 1);						
			error = 1;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecmanual), FALSE);
		}
		else
		{
			gtk_widget_set_sensitive(spn_tectgt, 0);
			gtk_widget_set_sensitive(vsc_tecpwr, 1);
			g_rw_lock_reader_lock(&thd_teclock);
			imgcam_get_tecp()->tecauto = status;
			imgcam_get_tecp()->tecedit = 1;
			g_rw_lock_reader_unlock(&thd_teclock);
			sprintf(imgmsg, C_("main","Tec set manual"));
			gtk_widget_set_sensitive(spn_tectgt, 0);
			// Toggle
			gtk_widget_set_sensitive(cmd_tecauto, 1);						
			gtk_widget_set_sensitive(cmd_tecmanual, 0);						
			error = 1;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecauto), FALSE);
		}
		gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	}
	else
	{
		error = 0;
	}
}

gboolean spn_tectgt_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	g_rw_lock_reader_lock(&thd_teclock);
	imgcam_get_tecp()->settemp = gtk_spin_button_get_value(spinbutton);
	sprintf(imgmsg, C_("main","Tec set auto to: %+06.2FC"), imgcam_get_tecp()->settemp);
	imgcam_get_tecp()->tecedit = 1;
	g_rw_lock_reader_unlock(&thd_teclock);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean spn_tecspd_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	g_rw_lock_reader_lock(&thd_teclock);
	tecspeed = gtk_spin_button_get_value(spinbutton);
	sprintf(imgmsg, C_("main","Tec speed set to: %d"), tecspeed);
	g_rw_lock_reader_unlock(&thd_teclock);
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
	return FALSE;
}

gboolean vsc_tecpwr_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
	g_rw_lock_reader_lock(&thd_teclock);
	imgcam_get_tecp()->tecpwr = 255. * (gtk_range_get_value(range) / 100.);
	g_rw_lock_reader_unlock(&thd_teclock);

	if (tmrtecpwr != -1)
	{
		g_source_remove(tmrtecpwr);
	}
	tmrhstrefresh = g_timeout_add(500, (GSourceFunc) tmr_tecpwr, NULL);
	return FALSE;
}

void cmb_cfw_changed (GtkComboBox *widget, gpointer user_data)
{
	int tmp = 0;
	char str[32];
	
	str[0] = '\0';
	sscanf(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)), "%d-%[^\n]", &tmp, str);
	if (imgcfw_set_mode(tmp) == 1)
	{
		switch (tmp)
		{
			case 0:
				// CFW not in use, reset default filter list
				combo_setlist(cmb_flt, fltstr);
				break;
				
			case 1:
				// QHY Serial
				gtk_widget_set_sensitive(cmb_cfwtty, 1);
				combo_ttylist(cmb_cfwtty);
				gtk_widget_set_sensitive(cmd_cfwtty, 1);
				gtk_widget_set_sensitive(cmd_cfw, 1);
				//gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				// This will read the configuration from the wheel itself
				break;
			
			case 2:
				// QHY Serial (2)
				gtk_widget_set_sensitive(cmb_cfwtty, 1);
				combo_ttylist(cmb_cfwtty);
				gtk_widget_set_sensitive(cmd_cfwtty, 1);
				gtk_widget_set_sensitive(cmd_cfw, 1);
				gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				break;
			
			case 99:
				// This is manufacturer specific so we load the list of choices 
				// from the camera UI.
				//combo_setlist(cmb_cfwcfg, imgcam_get_camui()->whlstr);
				// Then only connect button and list of models are active
				gtk_widget_set_sensitive(cmd_cfw, 1);
				//gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				gtk_widget_set_sensitive(cmb_cfwtty, 0);
				gtk_widget_set_sensitive(cmd_cfwtty, 0);
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
	int i = 0;

	if (error == 0)
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
		{
			// Not connected
			if (imgcfw_connect())
			{
				// Set UI
				gtk_widget_set_sensitive(cmb_cfw, 0);
				gtk_widget_set_sensitive(cmb_cfwcfg, 1);
				gtk_widget_set_sensitive(cmd_cfwrst, (imgcfw_is_reset() == 1));
				
				combo_setlist(cmb_cfwcfg, imgcfw_get_models());
				gtk_button_set_label(GTK_BUTTON(widget), C_("cfw","Disconnect"));

				gtk_widget_set_sensitive(cmd_cfwrst, imgcfw_is_reset());
				for (i = 0; i < CFW_SLOTS; i++)
				{
					gtk_widget_set_sensitive(cmb_cfwwhl[i], (i < imgcfw_get_slotcount()));
					gtk_widget_set_sensitive(cmd_cfwwhl[i], (i < imgcfw_get_slotcount()));
				}
				// CFW in use, ser filter list if all configured
				cmb_cfwwhl_changed (GTK_COMBO_BOX(cmb_cfwwhl[0]), cmb_cfwwhl);

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
				// CFW not in use, reset default filter list
				combo_setlist(cmb_flt, fltstr);
				// Set UI
				gtk_widget_set_sensitive(cmb_cfw, 1);
				gtk_widget_set_sensitive(cmb_cfwcfg, 0);
				gtk_widget_set_sensitive(cmd_cfwrst, 0);
				gtk_button_set_label(GTK_BUTTON(widget), C_("cfw","Connect"));

				gtk_widget_set_sensitive(cmd_cfwrst, FALSE);
				for (i = 0; i < CFW_SLOTS; i++)
				{
					gtk_widget_set_sensitive(cmb_cfwwhl[i], FALSE);
					gtk_widget_set_sensitive(cmd_cfwwhl[i], FALSE);
				}

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
	if ((gtk_combo_box_get_active(widget) != -1) && (gtk_widget_get_sensitive(GTK_WIDGET(widget))))
	{
		int i;
		
		imgcfw_set_model(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));
		sprintf(imgmsg, C_("cfw","Filter wheel configuration: %d slots, %s model"), imgcfw_get_slotcount(), imgcfw_get_model());
		// Set UI
		for (i = 0; i < CFW_SLOTS; i++)
		{
			gtk_widget_set_sensitive(cmb_cfwwhl[i], (i < imgcfw_get_slotcount()));
			gtk_widget_set_sensitive(cmd_cfwwhl[i], (i < imgcfw_get_slotcount()));
		}
		// CFW in use, ser filter list if all configured
		cmb_cfwwhl_changed (GTK_COMBO_BOX(cmb_cfwwhl[0]), cmb_cfwwhl);
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
	int  i, doit = 0;

	if ((gtk_combo_box_get_active(widget) != -1) && (gtk_widget_get_sensitive(GTK_WIDGET(widget))))
	{
		// If current combo has a meaningful value, try recalc cmb_flt content
		for (i = 0; i < imgcfw_get_slotcount(); i++)
		{
			if ((gtk_combo_box_get_active(GTK_COMBO_BOX(awidget[i])) > 0) && (gtk_widget_get_sensitive(GTK_WIDGET(awidget[i]))))
			{
				doit++;
			}
		}
		if (doit == imgcfw_get_slotcount())
		{
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
			if ((i = imgcfw_get_slot()) > -1)
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_flt), i);
			}
		}
		else
		{
			// Configuration not complete, reset default
			combo_setlist(cmb_flt, fltstr);
		}
	}
}

void cmd_cfwwhl_click (GtkComboBox *widget, gpointer user_data)
{
	int i;
	
	//printf("Got value: %d\n", (int)user_data);
	//imgcfw_set_slot((int)user_data, NULL);
	if (imgcfw_get_mode() == 99)
	{
		// We also need to pause the tec thread when "in camera"
		g_rw_lock_reader_lock(&thd_teclock);
	}
	if (imgcfw_set_slot(GPOINTER_TO_INT(user_data), (gpointer) cfwmsgdestroy))
	{
		// Disable all slot buttons
		for (i = 0; i < CFW_SLOTS; i++)
		{
			gtk_widget_set_sensitive(cmb_cfwwhl[i], 0);
			gtk_widget_set_sensitive(cmd_cfwwhl[i], 0);
		}
		// Show the change slot message
		cfwmsg = gtk_message_dialog_new ((GtkWindow *) window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, C_("cfw","Please wait for the filter to reach position..."));	
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(cfwmsg), C_("cfw","Dialog will disappear when done"));
		//gtk_window_set_decorated(GTK_WINDOW(cfwmsg), FALSE);
		gtk_window_set_deletable(GTK_WINDOW(cfwmsg), FALSE);
		gtk_window_set_keep_above(GTK_WINDOW(cfwmsg), TRUE);
		gtk_widget_show_all(cfwmsg);
	}
	else 
	{
		
		if (imgcfw_get_mode() == 99)
		{
			// Unlock here only in event of failure
			// Otherwise unlock is done in "cfwmsgdestroy" (see imgWfuncs.c)
			// The tec thread must stay paused while program checks and waits for the cfw to idle
			g_rw_lock_reader_unlock(&thd_teclock);
		}
	}
	sprintf(imgmsg, "%s", imgcfw_get_msg());
	gtk_statusbar_write(GTK_STATUSBAR(imgstatus), 0, imgmsg);
}

gboolean fiforeadcb (GIOChannel *gch, GIOCondition condition, gpointer data)
{
	GIOStatus retval;
	GError *err = NULL;
	gchar  *msg;
	char  cmd[33];
	static char  arg[225];
	float fval;
	int   ival, ival2;
	gsize len;

	if (condition & G_IO_HUP)
	{
		printf("Fifo: Read end died!\n");
		// We remove the event as it's now invalid
		return FALSE;
	}
	else
	{
		retval = g_io_channel_read_line (gch, &msg, &len, NULL, &err);
		if (retval == G_IO_STATUS_ERROR)
		{
			printf("Fifo: Error reading; %s\n", err->message);
		}
		else
		{
			// Msg format: "<command>:<arg>\n"
			// Msg max len 255
			sscanf(msg, "%[^:]:%[^\n]", cmd, arg);
			if (strcmp(cmd, "EXPTIME") == 0)
			{
				// Exposure time in ms
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				fval = (float)(ival / 1000.);
				sprintf(arg, "%05.3f", fval);
				GtkWidget *text = gtk_bin_get_child(GTK_BIN(cmb_exptime));
				gtk_entry_set_text(GTK_ENTRY(text), arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}			
			else if (strcmp(cmd, "TOTSHOTS") == 0)
			{
				// Shots to do in a run
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if (ival > 0)
				{
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_expnum), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=must be > 0\n");
				}
			}
			else if (strcmp(cmd, "SAVSHOTS") == 0)
			{	
				// Shots saved already (useful to number future shots)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if (ival > 0)
				{
					gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_shots), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=must be > 0\n");
				}
			}
			else if (strcmp(cmd, "MAXADU") == 0)
			{
				// Preview max adu (preview image will be saved accordingly)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if (((ival <= 65535) && (ival >= scrminadu) && (uibytepix == 2)) || ((ival <= 255) && (ival >= scrminadu) && (uibytepix == 1)))
				{
					gtk_range_set_value(GTK_RANGE(hsc_maxadu), (double)ival);
					hsc_maxadu_changed(GTK_RANGE(hsc_maxadu), GTK_SCROLL_NONE, (double)ival, NULL);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=MaxAdu out of range (%d-%d)\n", scrminadu, (int)(pow(256, uibytepix) -1));
				}
			}
			else if (strcmp(cmd, "READMAXADU") == 0)
			{
				sprintf(arg, "%d", scrmaxadu);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "MINADU") == 0)
			{
				// Preview min adu (preview image will be saved accordingly)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival <= scrmaxadu) && (ival >= 0))
				{
					gtk_range_set_value(GTK_RANGE(hsc_minadu), (double)ival);
					hsc_maxadu_changed(GTK_RANGE(hsc_minadu), GTK_SCROLL_NONE, (double)ival, NULL);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=MinAdu out of range (%d-%d)\n", 0, scrmaxadu);
				}
			}
			else if (strcmp(cmd, "READMINADU") == 0)
			{
				sprintf(arg, "%d", scrminadu);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "TECREAD") == 0)
			{
				// Enable / disable tec read feature
				if (imgcam_get_tecp()->istec > 0)
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if (gtk_widget_get_sensitive(cmd_tecenable) || gtk_widget_get_sensitive(cmd_tecdisable))
					{
						if (ival > 0)
						{
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), TRUE);
						}
						else
						{
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecdisable), TRUE);
						}
						
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=TEC unavailable in fast preview mode\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=No TEC to read\n");
				}
			}
			else if (strcmp(cmd, "TECAUTO") == 0)
			{
				// Enable disable tec feedback mode (using current target temp)
				if ((imgcam_get_tecp()->istec == 1) || (imgcam_get_tecp()->istec == 3))
				{
					if (gtk_widget_get_sensitive(cmd_tecenable) || gtk_widget_get_sensitive(cmd_tecdisable))
					{
						sscanf(arg, "%d", &ival);
						sprintf(arg, "%d", ival);
						if ((ival > 0) && (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_tecenable)) == FALSE))
						{
							// Activate tec read mode too if it is not already
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), TRUE);
						}
						if (ival)
						{
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecauto), TRUE);
						}
						else
						{
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecmanual), TRUE);
						}
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=TEC unavailable in fast preview mode\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=No TEC to set\n");
				}
			}
			else if (strcmp(cmd, "TECSPEED") == 0)
			{
				// Enable disable tec feedback mode (using current target temp)
				if ((imgcam_get_tecp()->istec == 1))
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if ((ival > 0) && (ival < 11))
					{
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tecspd), (gdouble)ival);
						printf("Fifo: %s=%s\n", cmd, arg);					
					}
					else
					{
						printf("Fifo: ERROR=TecSpeed %d out of range\n", ival);
					}
				}
				else
				{
					printf("Fifo: ERROR=No TEC to set or tec does not support speed setting\n");
				}
			}
			else if (strcmp(cmd, "SETTEMP") == 0)
			{
				// Set target temperature (if tecread & tecauto are not set already, it will do)
				if ((imgcam_get_tecp()->istec == 1) || (imgcam_get_tecp()->istec == 3))
				{
					if (gtk_widget_get_sensitive(cmd_tecenable) || gtk_widget_get_sensitive(cmd_tecdisable))
					{
						sscanf(arg, "%f", &fval);
						sprintf(arg, "%06.2f", fval);
						if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_tecenable)) == FALSE)
						{
							// Activate tec read mode too if it is not already
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecenable), TRUE);
						}
						if (imgcam_get_tecp()->tecauto == 0)
						{
							// Set tec to auto
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_tecauto), TRUE);
						}
						gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tectgt), fval);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=TEC unavailable in fast preview mode\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=No TEC to set\n");
				}
			}
			else if (strcmp(cmd, "GETTEMP") == 0)
			{
				// Print current CCD temp on the command line
				printf("Fifo: %s=%+06.2f\n", cmd, imgcam_get_tecp()->tectemp);
			}
			else if (strcmp(cmd, "CAPMODE") == 0)
			{
				// Set capture mode (next run will save file(s) following current naming conventin)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if (ival > 0)
				{
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_capture), TRUE);
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_capture)))
					{
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=file name/folder parameters missing\n");
					}
				}
				else
				{
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_focus), TRUE);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
			}
			else if (strcmp(cmd, "RUN") == 0)
			{
				// This will start camera capture in current CAPMODE
				// This command has no arg
				if (imgcam_connected())
				{
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_run)) == FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_run), TRUE);
						printf("Fifo: %s=ACK\n", cmd);				
					}
					else
					{
						printf("Fifo: ERROR=Capture already running\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=Camera is not connected\n");
				}
			}
			else if (strcmp(cmd, "STOP") == 0)
			{
				// This will request stop current camera capture run
				// To kill last capture (if > 1000ms) just issue a second STOP request
				// This command has no arg
				if (imgcam_connected())
				{
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_run)))
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_run), FALSE);
						printf("Fifo: %s=ACK\n", cmd);				
					}
					else
					{
						printf("Fifo: ERROR=No capture running\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=Camera is not connected\n");
				}
			}
			else if (strcmp(cmd, "HOLD") == 0)
			{
				// Enable / disable a pause in the current camera capture run
				if (imgcam_connected())
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_run)))
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_hold), (ival > 0));
						printf("Fifo: %s=ACK\n", cmd);				
					}
					else
					{
						printf("Fifo: ERROR=No capture running\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=Camera is not connected\n");
				}
			}
			else if (strcmp(cmd, "AUDELA") == 0)
			{
				// Sets "audela" naming convention (save folder = ~/<current date>, base name="image")
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_audela), (ival > 0));
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "IRIS") == 0)
			{
				// Add a variant to file numbering to be iris compatible
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_iris), (ival > 0));
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "DATEADD") == 0)
			{
				// Add current date to the file naming convention
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_dateadd), (ival > 0));
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "TIMEADD") == 0)
			{
				// Add current time to the naming convention
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_timeadd), (ival > 0));
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "FLTADD") == 0)
			{
				// Add current filter name to the naming convention
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_fltadd), (ival > 0));
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "FLTLIST") == 0)
			{
				// Prints the filter (pipe separated) list to the command line
				// FLTSET must use ordinal position (0 based) from this list
				// That is FLTSET:1 will set the second element
				combo_getlist(cmb_flt, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "FLTSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_flt)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_flt), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Filter index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_flt)-1);
				}
			}
			else if (strcmp(cmd, "ZEROCNT") == 0)
			{
				// Set a flag so that each camera capture run will restart file numbering from 0
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_zerofc), (ival > 0));
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "BASEFOLDER") == 0)
			{
				// Sets a custom folder to save captured frames
				gtk_entry_set_text(GTK_ENTRY(txt_fitfolder), arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "BASENAME") == 0)
			{
				// Sets a custom base name for captured frames
				gtk_entry_set_text(GTK_ENTRY(txt_fitbase), arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "OUTMODE") == 0)
			{
				// Sets the output mode (values are those in program combo 1:Fit, 2:Avi 3:Avi+Fit)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival > 0) && (ival < 4))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_fmt), (ival -1));
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Output mode out of range (1-3)\n");
				}
			}
			else if (strcmp(cmd, "CAMLIST") == 0)
			{
				// Prints the camera (pipe separated) list to the command line
				// CAMSET must use ordinal position (0 based) from this list
				// That is CAMSET:1 will set the second element
				combo_getlist(cmb_camera, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				if (!imgcam_connected())
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_camera)))
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_camera), ival);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=Camera index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_camera)-1);
					}
				}
				else
				{
					printf("Fifo: ERROR=Camera is connected already\n");
				}
			}
			else if (strcmp(cmd, "CAMCONNECT") == 0)
			{	
				// Connect / disconnect selected camera
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if (ival == 1)
				{
					if (!imgcam_connected())
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_camera), TRUE);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=Camera is connected already\n");
					}
				}
				else if (ival == 0)
				{
					if (imgcam_connected())
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_camera), FALSE);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=Camera is not connected\n");
					}				
				}
				else
				{
					printf("Fifo: ERROR=Invalid value (0-1)\n");
				}
			}
			else if (strcmp(cmd, "CAMREFRESH") == 0)
			{
				// Refresh camera list
				if (!imgcam_connected())
				{
					gtk_button_clicked(GTK_BUTTON(cmd_updcamlst));
					printf("Fifo: %s=ACK\n", cmd);
				}
				else
				{
					printf("Fifo: ERROR=Camera is connected already\n");
				}				
			}
			else if (strcmp(cmd, "CAMRESET") == 0)
			{
				// Reset currently selected camera
				if (!imgcam_connected())
				{
					if (gtk_combo_box_get_active(GTK_COMBO_BOX(cmb_camera)) > 0)
					{
						gtk_button_clicked(GTK_BUTTON(cmd_resetcam));
						printf("Fifo: %s=ACK\n", cmd);
					}
					else
					{
						printf("Fifo: ERROR=Cannot reset none camera\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=Camera is connected already\n");
				}				
			}
			else if (strcmp(cmd, "CAMOFFSET") == 0)
			{
				// Camera offset (0-255)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival <= 255) && (ival >= 0)) 
				{
					gtk_range_set_value(GTK_RANGE(hsc_offset), (double)ival);
					hsc_offset_changed(GTK_RANGE(hsc_offset), GTK_SCROLL_NONE, (double)ival, NULL);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Offset out of range (0-255)\n");
				}
			}
			else if (strcmp(cmd, "CAMGAIN") == 0)
			{
				// Camera offset (0-100)
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival <= 100) && (ival >= 0)) 
				{
					gtk_range_set_value(GTK_RANGE(hsc_gain), (double)ival);
					hsc_gain_changed(GTK_RANGE(hsc_gain), GTK_SCROLL_NONE, (double)ival, NULL);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Gain out of range (0-100)\n");
				}
			}
			else if (strcmp(cmd, "CAMBINLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMBINSET must use ordinal position (0 based) from this list
				// That is CAMBINSET:1 will set the second element
				combo_getlist(cmb_bin, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMBINSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_bin)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_bin), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Bin index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_bin)-1);
				}
			}
			else if (strcmp(cmd, "CAMSIZELIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMSIZESET must use ordinal position (0 based) from this list
				// That is CAMSIZESET:1 will set the second element
				combo_getlist(cmb_csize, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMSIZESET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_csize)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_csize), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Capture size index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_csize)-1);
				}
			}
			else if (strcmp(cmd, "CAMDSPDLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMDSPDSET must use ordinal position (0 based) from this list
				// That is CAMDSPDSET:1 will set the second element
				combo_getlist(cmb_dspeed, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMDSPDSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_dspeed)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_dspeed), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Download speed index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_dspeed)-1);
				}
			}
			else if (strcmp(cmd, "CAMXMODLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMXMODSET must use ordinal position (0 based) from this list
				// That is CAMDXMODSET:1 will set the second element
				combo_getlist(cmb_mode, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMXMODSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_mode)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_mode), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=%s index out of range (0-%d)\n", gtk_label_get_text(GTK_LABEL(lbl_mode)), gtk_combo_box_element_count(cmb_mode)-1);
				}
			}
			else if (strcmp(cmd, "CAMAMPLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMAMPSET must use ordinal position (0 based) from this list
				// That is CAMAMPSET:1 will set the second element
				combo_getlist(cmb_amp, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMAMPSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_amp)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_amp), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Amp mode index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_amp)-1);
				}
			}
			else if (strcmp(cmd, "CAMNRLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMNRSET must use ordinal position (0 based) from this list
				// That is CAMNRSET:1 will set the second element
				combo_getlist(cmb_denoise, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMNRSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_denoise)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_denoise), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Noise reduction mode index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_denoise)-1);
				}
			}
			else if (strcmp(cmd, "CAMDEPTHLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMDEPTHSET must use ordinal position (0 based) from this list
				// That is CAMDEPTHSET:1 will set the second element
				combo_getlist(cmb_depth, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMDEPTHSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_depth)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_depth), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Image depth index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_depth)-1);
				}
			}
			else if (strcmp(cmd, "CAMBAYERLIST") == 0)
			{
				// Prints the camera bin (pipe separated) list to the command line
				// CAMBAYERSET must use ordinal position (0 based) from this list
				// That is CAMBAYERSET:1 will set the second element
				combo_getlist(cmb_debayer, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CAMBAYERSET") == 0)
			{	
				// Set active the nth element in the filters combobox
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_debayer)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_debayer), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=Bayer matrix index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_debayer)-1);
				}
			}
			// CFW
			else if (strcmp(cmd, "CFWMODELIST") == 0)
			{
				// Prints the cfw (pipe separated) modes list to the command line
				// CFWMODESET must use ordinal position (0 based) from this list
				// That is CFWMODESET:1 will set the second element
				combo_getlist(cmb_cfw, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CFWMODESET") == 0)
			{	
				// Set active the nth element in the cfw mode combobox
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == FALSE)
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_cfw)))
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfw), ival);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=CFW index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_cfw)-1);
					}
				}
				else
				{
					printf("Fifo: ERROR=CFW is connected already\n");
				}
			}
			else if (strcmp(cmd, "CFWTTYLIST") == 0)
			{
				// Prints the cfw (pipe separated) modes list to the command line
				// CFWTTYSET must use ordinal position (0 based) from this list
				// That is CFWTTYSET:1 will set the second element
				combo_getlist(cmb_cfwtty, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CFWTTYSET") == 0)
			{	
				// Set active the nth element in the cfw tty combobox
				if (imgcfw_get_mode() == 1)
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_cfwtty)))
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfwtty), ival);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=CFW tty index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_cfwtty)-1);
					}
				}
				else
				{
					printf("Fifo: ERROR=CFW is not in tty mode\n");
				}
			}
			else if (strcmp(cmd, "CFWTTYREFRESH") == 0)
			{
				// Refresh cfw tty list
				if (imgcfw_get_mode() == 1)
				{
					gtk_button_clicked(GTK_BUTTON(cmd_cfwtty));
					printf("Fifo: %s=ACK\n", cmd);
				}
				else
				{
					printf("Fifo: ERROR=CFW is not in tty mode\n");
				}				
			}
			else if (strcmp(cmd, "CFWCONNECT") == 0)
			{	
				// Connect / disconnect selected CFW
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if (ival == 1)
				{
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_cfw), TRUE);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=CFW is connected already\n");
					}
				}
				else if (ival == 0)
				{
					if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == TRUE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cmd_camera), FALSE);
						printf("Fifo: %s=%s\n", cmd, arg);
					}
					else
					{
						printf("Fifo: ERROR=CFW is not connected\n");
					}				
				}
				else
				{
					printf("Fifo: ERROR=Invalid value (0-1)\n");
				}
			}
			else if (strcmp(cmd, "CFWCFGLIST") == 0)
			{
				// Prints the cfw (pipe separated) modes list to the command line
				// CFWCFGSET must use ordinal position (0 based) from this list
				// That is CFWCFGSET:1 will set the second element
				combo_getlist(cmb_cfwcfg, arg);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "CFWCFGSET") == 0)
			{	
				// Set the CFW geometry
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				if ((ival >= 0) && (ival < gtk_combo_box_element_count(cmb_cfwcfg)))
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfwcfg), ival);
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=CFW cfg index out of range (0-%d)\n", gtk_combo_box_element_count(cmb_cfwcfg)-1);
				}
			}
			else if (strcmp(cmd, "CFWRESET") == 0)
			{
				// Reset currently selected cfw
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == TRUE)
				{
					gtk_button_clicked(GTK_BUTTON(cmd_cfwrst));
					printf("Fifo: %s=ACK\n", cmd);
				}
				else
				{
					printf("Fifo: ERROR=CFW is not connected\n");
				}				
			}
			else if (strcmp(cmd, "CFWSETFILTERS") == 0)
			{
				// Sets a custom config for filters
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == TRUE)
				{
					char flt[225];	
					char *pch;
					char *saveptr;				
					int  n = 0, fpos = 0;
					
					strcpy(flt, arg);
					pch = strtok_r(flt,"|", &saveptr);
					while ((pch != NULL) && (n < CFW_SLOTS))
					{
						if ((fpos = gtk_combo_box_seek(cmb_cfwwhl[n], pch)) != -1)
						{
							gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfwwhl[n]), fpos);
						}
						else
						{
							printf("Fifo: ERROR=Unknown filter %s\n", pch);
						}
						n++;
						pch = strtok_r(NULL,"|", &saveptr);
					}
					printf("Fifo: %s=%s\n", cmd, arg);
				}
				else
				{
					printf("Fifo: ERROR=CFW is not connected\n");
				}				
			}
			else if (strcmp(cmd, "CFWGOTO") == 0)
			{
				// Goto position the currently selected cfw
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == TRUE)
				{
					sscanf(arg, "%d", &ival);
					sprintf(arg, "%d", ival);
					if ((ival >= 0) && (ival < imgcfw_get_slotcount()))
					{
						if (gtk_widget_get_sensitive(cmd_cfwwhl[ival]) == 1)
						{
							gtk_button_clicked(GTK_BUTTON(cmd_cfwwhl[ival]));
							printf("Fifo: %s=ACK\n", cmd);
						}
						else if (imgcfw_is_idle())
						{
							printf("Fifo: ERROR=CFW position %d is not valid\n", ival);
						}
						else
						{
							printf("Fifo: ERROR=Wait for last goto to end\n");
						}
					}
					else
					{
						printf("Fifo: ERROR=CFW position %d is not valid\n", ival);
					}
				}
				else
				{
					printf("Fifo: ERROR=CFW is not connected\n");
				}				
			}
			else if (strcmp(cmd, "CFWISIDLE") == 0)
			{
				// Return the current Idle status for CFW
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == TRUE)
				{
					printf("Fifo: %s=%d\n", cmd, imgcfw_is_idle());
				}
				else
				{
					printf("Fifo: ERROR=CFW is not connected\n");
				}				
			}
			else if (strcmp(cmd, "CFWGETPOS") == 0)
			{
				// Get current position of the currently selected cfw
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cmd_cfw)) == TRUE)
				{
					if (imgcfw_is_idle())
					{
						printf("Fifo: %s=%d\n", cmd, imgcfw_get_slot());
					}
					else
					{
						printf("Fifo: ERROR=CFW is not idle\n");
					}									
				}
				else
				{
					printf("Fifo: ERROR=CFW is not connected\n");
				}				
			}
			//
			else if (strcmp(cmd, "GETPRVWIDTH") == 0)
			{
				g_rw_lock_reader_lock(&pixbuf_lock);
				sprintf(arg, "%d", imgpix_get_width());
				g_rw_lock_reader_unlock(&pixbuf_lock);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "GETPRVHEIGHT") == 0)
			{
				g_rw_lock_reader_lock(&pixbuf_lock);
				sprintf(arg, "%d", imgpix_get_height());
				g_rw_lock_reader_unlock(&pixbuf_lock);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "GETDATADEPTH") == 0)
			{
				sprintf(arg, "%d", uibytepix);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "SETSAVEJPG") == 0)
			{
				sscanf(arg, "%d", &ival);
				sprintf(arg, "%d", ival);
				g_rw_lock_writer_lock(&thd_caplock);
				savejpg = (ival) ? 1 : 0;
				g_rw_lock_writer_unlock(&thd_caplock);
				printf("Fifo: %s=%s\n", cmd, arg);
			}
			else if (strcmp(cmd, "GETPREVIEW") == 0)
			{
				// Will save a preview image (as can be seen in program window)
				// File name will be <fifopath>.jpg
				// This command has no arg
				// Must use lock to avoid hitting the capture thread
				// Other features use native callbacks that are thread aware already
				char prwfile[256];
				char prwhst[256];
				
				sprintf(prwfile, "%s.jpg", fifopath);
				sprintf(prwhst , "%s.txt", fifopath);
				g_rw_lock_reader_lock(&pixbuf_lock);
				//if (gdk_pixbuf_save(imgpix_get_data(), prwfile, "png", NULL, "compression", "0", NULL))
				if ((imgpix_save_data(prwfile)) && (imgpix_save_histogram_data(prwhst)))
				{
					printf("Fifo: %s=ACK\n", cmd);
				}
				else
				{
					printf("Fifo: ERROR=Save preview failed\n");
				}
				g_rw_lock_reader_unlock(&pixbuf_lock);
			}
			else if (strcmp(cmd, "SETROIPOS") == 0)
			{
				ival = 0; ival2 = 0;
				sscanf(arg, "%d %d", &ival, &ival2);
				if (imgpix_loaded())
				{
					g_rw_lock_reader_lock(&thd_caplock);
					// Center on image data absolute
					fwhm_center(ival, ival2, 1);
					// Draw roi
					fwhm_show();
					// Calc
					fwhm_calc();
					// Draw roi after possible calc move
					fwhm_show();
					// Redraw icon
					if (hst == 0)
					{
						load_histogram_from_null();
					}	
					g_rw_lock_reader_unlock(&thd_caplock);
					// Please note "center" can modify requested x,y to let roi
					// be fully inside image, also roi will settle on the
					// brightest spot...
					printf("Fifo: %s=%d %d\n", cmd, fwhmx, fwhmy);

					FILE *outf;
					outf = fopen("/tmp/roipos.txt", "w");
					if (outf != NULL)
					{
					//	fprintf(outf, "Fifo: %s=%d %d\n", cmd, fwhmx, fwhmy);
						fprintf(outf,"%d\n", fwhmx);
						fprintf(outf,"%d\n", fwhmy);
					}
					fclose(outf);
				}
				else
				{
					printf("Fifo: ERROR=Set ROI position failed\n");
				}
			}
			else if (strcmp(cmd, "GETROIPOS") == 0)
			{
				if (fwhmv == 1)
				{
					g_rw_lock_reader_lock(&thd_caplock);
					printf("Fifo: %s=%d %d\n", cmd, fwhmx, fwhmy);
					g_rw_lock_reader_unlock(&thd_caplock);

					FILE *outf;
					outf = fopen("/tmp/roipos.txt", "w");
					if (outf != NULL)
					{
					//	fprintf(outf, "Fifo: %s=%d %d\n", cmd, fwhmx, fwhmy);
						fprintf(outf,"%d\n", fwhmx);
						fprintf(outf,"%d\n", fwhmy);
					}
					fclose(outf);
				}
				else
				{
					printf("Fifo: ERROR=ROI not visible\n");
				}
			}
			else if (strcmp(cmd, "SETROISIZE") == 0)
			{
				sscanf(arg, "%d", &ival);
				if ((ival == 8) || (ival == 16) || (ival == 32) || (ival == 64))
				{
					fwhmp = ival * ival;
					fwhms = ival;		
					if (fwhmv == 1)
					{
						g_rw_lock_reader_lock(&thd_caplock);
						// Draw roi
						fwhm_show();
						// Calc
						fwhm_calc();
						// Draw roi after possible calc move
						fwhm_show();
						// Redraw icon
						if (hst == 0)
						{
							load_histogram_from_null();
						}	
						g_rw_lock_reader_unlock(&thd_caplock);
					}
					printf("Fifo: %s=ACK\n", cmd);
				}
				else
				{
					printf("Fifo: ERROR=Must be 8, 16, 32 or 64\n");
				}				
			}
			else if (strcmp(cmd, "HIDEROI") == 0)
			{
				fwhm_hide();
				// Redraw icon
				if (hst == 0)
				{
					load_histogram_from_null();
				}	
				printf("Fifo: %s=ACK\n", cmd);
			}
			else if (strcmp(cmd, "GETFWHM") == 0)
			{
				if (fwhmv == 1)
				{
					g_rw_lock_reader_lock(&thd_caplock);
					printf("Fifo: %s=%05.2F %d\n", cmd, afwhm, pfwhm);
					g_rw_lock_reader_unlock(&thd_caplock);

					FILE *outf;
					outf = fopen("/tmp/fwhm.txt", "w");
					if (outf != NULL)
					{
					//	fprintf(outf, "Fifo: %s=%05.2F %d\n", cmd, afwhm, pfwhm);
						fprintf(outf, "%05.2F\n", afwhm);
						fprintf(outf, "%d\n", pfwhm);
					}
					fclose(outf);
				}
				else
				{
					printf("Fifo: ERROR=ROI not visible\n");
				}
			}
			else if (strcmp(cmd, "LOADFILE") == 0)
			{
				g_rw_lock_reader_lock(&thd_caplock);
				ival = run;
				g_rw_lock_reader_unlock(&thd_caplock);
				if (ival == 0)
				{
					if (isfile(arg))
					{
						cmd_load_click(NULL, arg);
						if (imgfit_loaded())
						{
							if (imgpix_loaded())
							{
								printf("Fifo: %s=ACK\n", cmd);
							}
							else
							{
								printf("Fifo: ERROR=could not load pixel buffer\n");
							}
						}
						else
						{
							printf("Fifo: ERROR=could not load fit file\n");
						}
					}
					else
					{
						printf("Fifo: ERROR=file not found\n");
					}
				}
				else
				{
					printf("Fifo: ERROR=capture thread is running\n");
				}
			}
			else
			{
				printf("Fifo: Unknown command; Read %" G_GSIZE_FORMAT " bytes; %s\n", len, msg);
			}
		}
		g_free(msg);
	}
	return TRUE;
}

