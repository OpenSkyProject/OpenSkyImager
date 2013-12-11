/*
 * imgWindow.c
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

#define DECLARE_WINDOW

#include "imgWindow.h"
#include "imgWCallbacks.h"
#include "imgWFuncs.h"

void imgstatus_build()
{
	imgstatus = gtk_statusbar_new();
	g_signal_connect(G_OBJECT(imgstatus), "text-pushed", G_CALLBACK(imgstatus_push), NULL);
}

void cmd_settings_build()
{
	cmd_settings = gtk_toggle_button_new_with_label_color(C_("main","Show settings"), 140, 25, &clrSelected);
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(cmd_settings, TRUE);
	gtk_widget_set_vexpand(cmd_settings, TRUE);
	#endif	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_settings), "clicked", G_CALLBACK(cmd_settings_click), NULL);
}

void cmd_about_build()
{
	cmd_about = gtk_button_new_with_label("?");
	gtk_widget_set_size_request(cmd_about, 25, 25);

	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(cmd_about, FALSE);
	gtk_widget_set_vexpand(cmd_about, TRUE);
	#endif	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_about), "clicked", G_CALLBACK(cmd_about_click), NULL);
}

void cmd_capture_build()
{
	cmd_capture = gtk_toggle_button_new_with_label_color(C_("main","Focus mode"), 140, 25, &clrSelected);	
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(cmd_capture, TRUE);
	gtk_widget_set_vexpand(cmd_capture, TRUE);
	#endif	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_capture), "clicked", G_CALLBACK(cmd_capture_click), NULL);
}

void cmb_exptime_build()
{
	int crows = 0;
	char tmp[8];
	
	cmb_exptime = gtk_combo_box_text_new_with_entry();
	gtk_entry_set_width_chars(GTK_ENTRY(gtk_bin_get_child (GTK_BIN(cmb_exptime))), 6);
	gtk_widget_set_size_request(cmb_exptime, 40, 25);
	sprintf(tmp, "%4.4g", 1800.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 1200.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 900.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 600.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 480.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 240.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 120.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 60.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 30.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 20.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 10.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 5.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 2.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", 1.);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .5);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .2);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .1);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .05);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .02);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .01);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .005);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .002);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	sprintf(tmp, "%4.4g", .001);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_exptime), tmp);
	
	g_signal_connect(G_OBJECT(cmb_exptime), "changed", G_CALLBACK(cmb_exptime_changed),  NULL);
	g_signal_connect(G_OBJECT(gtk_bin_get_child (GTK_BIN(cmb_exptime))), "key-press-event", G_CALLBACK(numbers_input_keypress), (gpointer)5);	
	crows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(cmb_exptime))), NULL);
	if (crows > 0)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_exptime), (crows - 1));
	}
}

void spn_expnum_build()
{
	spn_expnum = gtk_spin_button_new_with_range (1.0, 900.0, 1.0);
	gtk_widget_set_size_request(spn_expnum, 40, 25);
	expnum = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spn_expnum));
	//Callbacks
	g_signal_connect(G_OBJECT(spn_expnum), "value-changed", G_CALLBACK(spn_expnum_changed),  NULL);
}

void spn_shots_build()
{
	spn_shots = gtk_spin_button_new_with_range (0.0, 900.0, 1.0);
	gtk_widget_set_size_request(spn_shots, 40, 25);
	shots = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spn_shots));
	
	g_signal_connect(G_OBJECT(spn_shots), "value-changed", G_CALLBACK(spn_shots_changed),  NULL);
}

void pbr_expnum_build()
{
	pbr_expnum = gtk_progress_bar_new();
	gtk_widget_set_size_request(pbr_expnum, 140, 10);
	#if GTK_MAJOR_VERSION == 3
	gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(pbr_expnum), FALSE);
	#else
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbr_expnum), GTK_PROGRESS_LEFT_TO_RIGHT);
	#endif

	//g_signal_connect(G_OBJECT(spn_shots), "value-changed", G_CALLBACK(spn_shots_changed),  NULL);
}

void cmd_run_build()
{
	cmd_run = gtk_toggle_button_new_with_label_color(C_("main","Start"), 140, 25, &clrSelected);
	gtk_widget_set_sensitive(cmd_run, 0);
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(cmd_run, TRUE);
	gtk_widget_set_vexpand(cmd_run, TRUE);
	#endif	
	
	// Initialize thread RW lock for the capture thread
	g_rw_lock_init(&thd_caplock);
	
	// Callback
	g_signal_connect(G_OBJECT(cmd_run), "clicked", G_CALLBACK(cmd_run_click), NULL);
}

void pbr_exptime_build()
{
	pbr_exptime = gtk_progress_bar_new();
	gtk_widget_set_size_request(pbr_exptime, 140, 10);
	#if GTK_MAJOR_VERSION == 3
	gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(pbr_exptime), TRUE);
	#else
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbr_exptime), GTK_PROGRESS_RIGHT_TO_LEFT);
	#endif
	
	//g_signal_connect(G_OBJECT(spn_shots), "value-changed", G_CALLBACK(spn_shots_changed),  NULL);
}

void cmd_hold_build()
{
	cmd_hold = gtk_toggle_button_new_with_label_color(C_("main","Hold"), 140, 25, &clrSelected);
	gtk_widget_set_sensitive(cmd_hold, 0);
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(cmd_hold, TRUE);
	gtk_widget_set_vexpand(cmd_hold, TRUE);
	#endif	

	g_signal_connect(G_OBJECT(cmd_hold), "clicked", G_CALLBACK(cmd_hold_click), NULL);
}

void cmd_load_build()
{
	cmd_load = gtk_button_new_with_label(C_("main","Load"));
	gtk_widget_set_size_request(cmd_load, 70, 25);

	// Callbacks
	g_signal_connect(G_OBJECT(cmd_load), "clicked", G_CALLBACK(cmd_load_click), NULL);
}

void cmd_fit_build()
{
	cmd_fit = gtk_toggle_button_new_with_label_color(C_("main","1:1"), 70, 25, &clrSelected);	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_fit), "clicked", G_CALLBACK(cmd_fit_click), NULL);
}

void lbl_fbkimg_build()
{
	PangoFontDescription *fd; 
	
	imgfbk[0] = '\0';
	lbl_fbkimg = gtk_label_new_with_align(imgfbk, 0.0, 0.5, 90, 60);
	fd = pango_font_description_from_string("Monospace 18"); 
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_override_color(lbl_fbkimg, GTK_STATE_NORMAL, &clrFbk);
	gtk_widget_override_font(lbl_fbkimg, fd);
	#else
	gtk_widget_modify_fg(lbl_fbkimg, GTK_STATE_NORMAL, &clrFbk);
	gtk_widget_modify_font(lbl_fbkimg, fd);
	#endif
	pango_font_description_free(fd);
}

void lbl_fbktec_build()
{
	PangoFontDescription *fd; 
	
	tecfbk[0] = '\0';
	lbl_fbktec = gtk_label_new_with_align(tecfbk, 0.0, 0.5, 120, 60);
	fd = pango_font_description_from_string("Monospace 18"); 
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_override_color(lbl_fbktec , GTK_STATE_NORMAL, &clrFbk);
	gtk_widget_override_font(lbl_fbktec, fd);
	#else
	gtk_widget_modify_fg(lbl_fbktec, GTK_STATE_NORMAL, &clrFbk);
	gtk_widget_modify_font(lbl_fbktec, fd);
	#endif
	pango_font_description_free(fd);
}

void lbl_fbkfps_build()
{
	PangoFontDescription *fd; 
	
	fpsfbk[0] = '\0';
	lbl_fbkfps = gtk_label_new_with_align(fpsfbk, 0.0, 0.5, 150, 60);
	fd = pango_font_description_from_string("Monospace 18"); 
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_override_color(lbl_fbkfps , GTK_STATE_NORMAL, &clrFbk);
	gtk_widget_override_font(lbl_fbkfps, fd);
	#else
	gtk_widget_modify_fg(lbl_fbkfps, GTK_STATE_NORMAL, &clrFbk);
	gtk_widget_modify_font(lbl_fbkfps, fd);
	#endif
	pango_font_description_free(fd);
}

void cmd_histogram_build()
{
	cmd_histogram = gtk_toggle_button_new_with_label_color(C_("main","Show graph"), 90, 25, &clrSelected);	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_histogram), "clicked", G_CALLBACK(cmd_histogram_click), NULL);
}

void spn_histogram_build()
{
	spn_histogram = gtk_spin_button_new_with_range (1.0, 100.0, 1.0);
	gtk_widget_set_size_request(spn_histogram, 30, 25);
	//gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spn_histogram), TRUE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_histogram), 5.0);
	gtk_widget_set_sensitive(spn_histogram, 0);

	//Callbacks
	g_signal_connect(G_OBJECT(spn_histogram), "value-changed", G_CALLBACK(spn_histogram_changed),  NULL);
}

void hsc_maxadu_build()
{
	hsc_maxadu = gtk_hscale_new_with_range(0.0, 255.0, 1.0);
	gtk_widget_set_name(GTK_WIDGET(hsc_maxadu), "hsc_maxadu");
	gtk_scale_set_digits(GTK_SCALE(hsc_maxadu), 0);
	gtk_scale_set_value_pos(GTK_SCALE(hsc_maxadu), GTK_POS_TOP);
	gtk_range_set_value(GTK_RANGE(hsc_maxadu), 255);
	gtk_widget_set_size_request(hsc_maxadu, 140, 35);
	
	g_signal_connect(G_OBJECT(hsc_maxadu), "change-value", G_CALLBACK(hsc_maxadu_changed),  NULL);
}

void hsc_minadu_build()
{
	hsc_minadu = gtk_hscale_new_with_range(0.0, 254.0, 1.0);
	gtk_widget_set_name(GTK_WIDGET(hsc_minadu), "hsc_minadu");
	gtk_scale_set_digits(GTK_SCALE(hsc_minadu), 0);
	gtk_scale_set_value_pos(GTK_SCALE(hsc_minadu), GTK_POS_BOTTOM);
	gtk_range_set_value(GTK_RANGE(hsc_minadu), 0);
	gtk_widget_set_size_request(hsc_minadu, 140, 35);
	
	g_signal_connect(G_OBJECT(hsc_minadu), "change-value", G_CALLBACK(hsc_minadu_changed),  NULL);
}

void frm_histogram_build()
{
	histogram = gtk_image_new();
	gtk_widget_set_size_request(histogram, 180, 70);
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(histogram, TRUE);
	gtk_widget_set_vexpand(histogram, TRUE);
	#endif

	#if GTK_MAJOR_VERSION == 3
	frm_histogram = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(frm_histogram, 180, 75);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(frm_histogram), GTK_WIDGET(histogram));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(frm_histogram), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(frm_histogram), GTK_SHADOW_ETCHED_IN);
	#else
	frm_histogram = gtk_frame_new(NULL);
	gtk_widget_set_size_request(frm_histogram, 180, 70);
	gtk_container_add(GTK_CONTAINER(frm_histogram), histogram);
	#endif
		
	// Callback
	g_signal_connect(G_OBJECT(frm_histogram), "size-allocate", G_CALLBACK(frm_histogram_allocate), NULL);	
}

void image_build()
{
	// Image
	image = gtk_image_new();	
	
	// Initialize pixbuf RW lock for the capture thread
	g_rw_lock_init(&pixbuf_lock);
}

void swindow_build()
{
	// Scrolled window for image
	swindow = gtk_scrolled_window_new(NULL, NULL);
	fixed   = gtk_fixed_new();
	
	lbl_fbkimg_build();
	lbl_fbktec_build();
	lbl_fbkfps_build();
	image_build();

	// Pack image into scrolled window
	gtk_fixed_put(GTK_FIXED(fixed), image, 0, 0);
	gtk_fixed_put(GTK_FIXED(fixed), lbl_fbkimg,  10, 0);
	gtk_fixed_put(GTK_FIXED(fixed), lbl_fbktec, 100, 0);
	gtk_fixed_put(GTK_FIXED(fixed), lbl_fbkfps, 230, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swindow), GTK_WIDGET(fixed));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swindow), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	
	// Callback
	g_signal_connect(G_OBJECT(swindow), "size-allocate", G_CALLBACK(swindow_allocate), NULL);	
}

void cmb_camera_build()
{
	cmb_camera = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_camera, 120, 30);	
	imgcam_init();
	combo_setlist(cmb_camera, imgcam_get_camui()->camstr);

	// Callbacks
	g_signal_connect(G_OBJECT(cmb_camera), "changed", G_CALLBACK(cmb_camera_changed),  NULL);
}

void cmd_camera_build()
{
	cmd_camera = gtk_toggle_button_new_with_label_color(C_("settings","Connect"), 90, 30, &clrSelected);
	gtk_widget_set_sensitive(cmd_camera, 0);

	// Callbacks
	g_signal_connect(G_OBJECT(cmd_camera), "clicked", G_CALLBACK(cmd_camera_click), NULL);
}

void cmd_setcamlst_build()
{
	cmd_setcamlst = gtk_toggle_button_new_with_label_color(C_("settings","Full list"), 70, 30, &clrSelected);	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_setcamlst), "clicked", G_CALLBACK(cmd_setcamlst_click), NULL);
}

void cmd_updcamlst_build()
{
	cmd_updcamlst = gtk_button_new_with_label(C_("main","Refresh"));
	gtk_widget_set_size_request(cmd_updcamlst, 70, 30);
	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_updcamlst), "clicked", G_CALLBACK(cmd_updcamlst_click), NULL);
}

void cmd_resetcam_build()
{
	cmd_resetcam = gtk_button_new_with_label(C_("main","Reset"));
	gtk_widget_set_size_request(cmd_resetcam, 70, 30);
	gtk_widget_set_sensitive(cmd_resetcam, 0);	
	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_resetcam), "clicked", G_CALLBACK(cmd_resetcam_click), NULL);
}

void cmb_bin_build()
{
	cmb_bin = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_bin, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_bin);
	#endif
	
	g_signal_connect(G_OBJECT(cmb_bin), "changed", G_CALLBACK(cmb_bin_changed),  NULL);
}

void hsc_offset_build()
{
	hsc_offset = gtk_hscale_new_with_range(0.0, 255.0, 1.0);
	gtk_scale_set_digits(GTK_SCALE(hsc_offset), 0);
	gtk_scale_set_value_pos(GTK_SCALE(hsc_offset), GTK_POS_LEFT);
	gtk_widget_set_size_request(hsc_offset, 50, 30);
	
	g_signal_connect(G_OBJECT(hsc_offset), "change-value", G_CALLBACK(hsc_offset_changed),  NULL);
	gtk_range_set_value(GTK_RANGE(hsc_offset), 120);
	imgcam_get_expar()->offset = 120;
}

void hsc_gain_build()
{
	hsc_gain = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
	gtk_scale_set_digits(GTK_SCALE(hsc_gain), 0);
	gtk_scale_set_value_pos(GTK_SCALE(hsc_gain), GTK_POS_LEFT);
	gtk_widget_set_size_request(hsc_gain, 50, 30);
		
	g_signal_connect(G_OBJECT(hsc_gain), "change-value", G_CALLBACK(hsc_gain_changed),  NULL);
	gtk_range_set_value(GTK_RANGE(hsc_gain), 20);
	imgcam_get_expar()->gain = 20;
}

void cmb_csize_build()
{
	cmb_csize = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_csize, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_csize);
	#endif
		
	g_signal_connect(G_OBJECT(cmb_csize), "changed", G_CALLBACK(cmb_csize_changed),  NULL);
}

void cmb_dspeed_build()
{
	cmb_dspeed = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_dspeed, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_dspeed);
	#endif


	g_signal_connect(G_OBJECT(cmb_dspeed), "changed", G_CALLBACK(cmb_dspeed_changed),  NULL);
}

void cmb_mode_build()
{
	cmb_mode = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_mode, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_mode);
	#endif
		
	g_signal_connect(G_OBJECT(cmb_mode), "changed", G_CALLBACK(cmb_mode_changed),  NULL);
}

void cmb_amp_build()
{
	cmb_amp = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_amp, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_amp);
	#endif
		
	g_signal_connect(G_OBJECT(cmb_amp), "changed", G_CALLBACK(cmb_amp_changed),  NULL);
}

void cmb_denoise_build()
{
	cmb_denoise = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_denoise, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_denoise);
	#endif
		
	g_signal_connect(G_OBJECT(cmb_denoise), "changed", G_CALLBACK(cmb_denoise_changed),  NULL);
}

void cmb_depth_build()
{
	cmb_depth = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_depth, 50, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_depth);
	#endif
	
	g_signal_connect(G_OBJECT(cmb_depth), "changed", G_CALLBACK(cmb_depth_changed),  NULL);
}

void cmb_debayer_build()
{
	cmb_debayer = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_debayer, 50, 30);
	/// This is the "no bayer mask" value in the debayer combo box, other values are just intl as they are
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_debayer), C_("settings","None"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_debayer), "GBRG");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_debayer), "RGGB");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_debayer), "GRBG");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_debayer), "BGGR");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_debayer), 0);
	
	g_signal_connect(G_OBJECT(cmb_debayer), "changed", G_CALLBACK(cmb_debayer_changed),  NULL);
}

void cmd_saveas_build()
{
	cmd_saveas = gtk_button_new_with_label(C_("filename","Save As"));
	gtk_widget_set_size_request(cmd_saveas, 80, 30);
	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_saveas), "clicked", G_CALLBACK(cmd_saveas_click), NULL);
}

void cmd_audela_build()
{
	cmd_audela = gtk_toggle_button_new_with_label_color(C_("filename","Audela naming"), 80, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_audela), "clicked", G_CALLBACK(cmd_audela_click), NULL);
}

void cmd_iris_build()
{
	cmd_iris = gtk_toggle_button_new_with_label_color(C_("filename","Iris naming"), 80, 30, &clrSelected);	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_iris), "clicked", G_CALLBACK(cmd_iris_click), NULL);
}

void cmd_zerofc_build()
{
	cmd_zerofc = gtk_toggle_button_new_with_label_color(C_("filename","Zero counter"), 80, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_zerofc), "clicked", G_CALLBACK(cmd_zerofc_click), NULL);
}

void cmd_tlenable_build()
{
	cmd_tlenable = gtk_toggle_button_new_with_label_color(C_("filename","Time Lapse"), 80, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_tlenable), "clicked", G_CALLBACK(cmd_tlenable_click), NULL);
}

void txt_fitfolder_build()
{
	txt_fitfolder = gtk_entry_new();
	gtk_widget_set_size_request(txt_fitfolder, 560, 30);
	//Callbacks
	g_signal_connect(G_OBJECT(txt_fitfolder), "changed", G_CALLBACK(txt_fitfolder_changed),  NULL);
}

void txt_fitbase_build()
{
	txt_fitbase = gtk_entry_new();
	gtk_widget_set_size_request(txt_fitbase, 560, 30);
	
	//Callbacks
	g_signal_connect(G_OBJECT(txt_fitbase), "changed", G_CALLBACK(txt_fitbase_changed),  NULL);
}

void cmd_dateadd_build()
{
	cmd_dateadd = gtk_toggle_button_new_with_label_color(C_("filename","Add date"), 80, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_dateadd), "clicked", G_CALLBACK(cmd_dateadd_click), NULL);
}

void cmd_timeadd_build()
{
	cmd_timeadd = gtk_toggle_button_new_with_label_color(C_("filename","Add time"), 80, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_timeadd), "clicked", G_CALLBACK(cmd_timeadd_click), NULL);
}

void cmd_fltadd_build()
{
	cmd_fltadd = gtk_toggle_button_new_with_label_color(C_("filename","Add filter"), 80, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_fltadd), "clicked", G_CALLBACK(cmd_fltadd_click), NULL);
}

void cmb_flt_build()
{
	cmb_flt = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_flt, 80, 30);
	gtk_widget_set_sensitive(cmb_flt, 0);
	
	g_signal_connect(G_OBJECT(cmb_flt), "changed", G_CALLBACK(cmb_flt_changed),  NULL);
	// Also set initial value for the char variable ;-)
	combo_setlist(cmb_flt, fltstr);
}

void cmd_tecenable_build()
{
	cmd_tecenable = gtk_toggle_button_new_with_label_color(C_("cooling","Enable tec read"), 140, 30, &clrSelected);
	gtk_widget_set_sensitive(cmd_tecenable, 0);

	// Callbacks
	g_signal_connect(G_OBJECT(cmd_tecenable), "clicked", G_CALLBACK(cmd_tecenable_click), NULL);
}

void cmd_tecauto_build()
{
	cmd_tecauto = gtk_toggle_button_new_with_label_color(C_("cooling","Manual mode"), 140, 30, &clrSelected);
	gtk_widget_set_sensitive(cmd_tecauto, 0);

	// Callbacks
	g_signal_connect(G_OBJECT(cmd_tecauto), "clicked", G_CALLBACK(cmd_tecauto_click), NULL);
}

void spn_tectgt_build()
{
	spn_tectgt = gtk_spin_button_new_with_range(-50.0, +30.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spn_tectgt), 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tectgt), +5.0);
	gtk_widget_set_size_request(spn_tectgt, 60, 25);	
	gtk_widget_set_sensitive(spn_tectgt, 0);
	
	//Callbacks
	g_signal_connect(G_OBJECT(spn_tectgt), "value-changed", G_CALLBACK(spn_tectgt_changed),  NULL);
	g_signal_connect(G_OBJECT(spn_tectgt), "key-press-event", G_CALLBACK(numbers_input_keypress), (gpointer)5);	
}

void vsc_tectemp_build()
{
	vsc_tectemp = gtk_vscale_new_with_range(-50.0, +30.0, 1.0);
	gtk_widget_set_name(GTK_WIDGET(vsc_tectemp), "tecpwr");
	gtk_scale_set_digits(GTK_SCALE(vsc_tectemp), 1);
	gtk_range_set_inverted(GTK_RANGE(vsc_tectemp), TRUE);
	gtk_scale_set_value_pos(GTK_SCALE(vsc_tectemp), GTK_POS_LEFT);
	gtk_widget_set_size_request(vsc_tectemp, 70, 120);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), -50.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), -40.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), -30.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), -20.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), -10.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp),   0.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), +10.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), +20.0, GTK_POS_RIGHT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tectemp), +30.0, GTK_POS_RIGHT, NULL);
	gtk_widget_set_sensitive(vsc_tectemp, 0);
	
	//g_signal_connect(G_OBJECT(vsc_tectemp), "change-value", G_CALLBACK(vsc_tectempr_changed),  NULL);
}

void frm_tecgraph_build()
{
	tecgraph = gtk_image_new();	
	gtk_widget_set_size_request(tecgraph, 360, 240);
	#if GTK_MAJOR_VERSION == 3
	gtk_widget_set_hexpand(tecgraph, FALSE);
	gtk_widget_set_vexpand(tecgraph, FALSE);
	#endif
	
	// Image init
	#if GTK_MAJOR_VERSION == 3
	frm_tecgraph = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(frm_tecgraph, 365, 245);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(frm_tecgraph), GTK_WIDGET(tecgraph));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(frm_tecgraph), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(frm_tecgraph), GTK_SHADOW_ETCHED_IN);
	#else
	frm_tecgraph = gtk_frame_new(NULL);
	gtk_widget_set_size_request(frm_tecgraph, 360, 240);
	gtk_container_add(GTK_CONTAINER(frm_tecgraph), tecgraph);
	#endif
	// Pixel buffer init
	tecpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 120, 80);
	tec_init_graph();
	
	// Callback
	//g_signal_connect(G_OBJECT(frm_tecgraph), "size-allocate", G_CALLBACK(frm_tecgraph_allocate), NULL);
}

void vsc_tecpwr_build()
{
	vsc_tecpwr = gtk_vscale_new_with_range(0.0, 100.0, 1.0);
	gtk_widget_set_name(GTK_WIDGET(vsc_tecpwr), "tecpwr");
	gtk_scale_set_digits(GTK_SCALE(vsc_tecpwr), 0);
	gtk_range_set_inverted(GTK_RANGE(vsc_tecpwr), TRUE);
	gtk_scale_set_value_pos(GTK_SCALE(vsc_tecpwr), GTK_POS_RIGHT);
	gtk_widget_set_size_request(vsc_tecpwr, 70, 120);
	gtk_scale_add_mark(GTK_SCALE(vsc_tecpwr),  0.0, GTK_POS_LEFT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tecpwr), 25.0, GTK_POS_LEFT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tecpwr), 50.0, GTK_POS_LEFT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tecpwr), 75.0, GTK_POS_LEFT, NULL);
	gtk_scale_add_mark(GTK_SCALE(vsc_tecpwr),100.0, GTK_POS_LEFT, NULL);
	gtk_widget_set_sensitive(vsc_tecpwr, 0);
	
	g_signal_connect(G_OBJECT(vsc_tecpwr), "change-value", G_CALLBACK(vsc_tecpwr_changed),  NULL);
}

void cmd_tlcalendar_build()
{
	cmd_tlcalendar = gtk_toggle_button_new_with_label_color(C_("timelapse","Simple mode"), 140, 30, &clrSelected);
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_tlcalendar), "clicked", G_CALLBACK(cmd_tlcalendar_click), NULL);
}

void cal_tldpick_build()
{
	cal_tldpick = gtk_calendar_new();
	gtk_calendar_set_display_options(GTK_CALENDAR(cal_tldpick), GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);
	gtk_widget_set_sensitive(cal_tldpick, 0);

	//Callbacks
	g_signal_connect(G_OBJECT(cal_tldpick), "day-selected-double-click", G_CALLBACK(cal_tldpick_dblclick),  NULL);
}

void hsc_tlperiod_build()
{
	hsc_tlperiod = gtk_hscale_new_with_range(0.0, 86400.0, 1.0);
	gtk_scale_set_digits(GTK_SCALE(hsc_tlperiod), 0);
	gtk_scale_set_draw_value(GTK_SCALE(hsc_tlperiod), FALSE);
	gtk_widget_set_size_request(hsc_tlperiod, 300, 30);
	
	// Callback
	g_signal_connect(G_OBJECT(hsc_tlperiod), "change-value", G_CALLBACK(hsc_tlperiod_changed),  NULL);
	gtk_range_set_value(GTK_RANGE(hsc_tlperiod), 1.0);
}

void cmb_cfw_build()
{
	cmb_cfw = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_cfw, 140, 30);
	/// Value "No CFW" of the cfw combo, please preserve the n-xxxxx format
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_cfw), C_("cfw","0-None"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cmb_cfw), C_("cfw","1-QHY Serial"));
	
	g_signal_connect(G_OBJECT(cmb_cfw), "changed", G_CALLBACK(cmb_cfw_changed),  NULL);	
	gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_cfw), 0);
}

void cmb_cfwtty_build()
{
	cmb_cfwtty = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_cfwtty, 140, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_cfwtty);
	#endif
	gtk_widget_set_sensitive(cmb_cfwtty, 0);
	
	g_signal_connect(G_OBJECT(cmb_cfwtty), "changed", G_CALLBACK(cmb_cfwtty_changed),  NULL);	
}

void cmd_cfwtty_build()
{
	cmd_cfwtty = gtk_button_new_with_label(C_("cfw","Refresh"));
	gtk_widget_set_size_request(cmd_cfwtty, 140, 30);
	gtk_widget_set_sensitive(cmd_cfwtty, 0);
	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_cfwtty), "clicked", G_CALLBACK(cmd_cfwtty_click), NULL);
}

void cmd_cfw_build()
{
	cmd_cfw = gtk_toggle_button_new_with_label_color(C_("cfw","Connect"), 140, 30, &clrSelected);
	gtk_widget_set_sensitive(cmd_cfw, 0);
	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_cfw), "clicked", G_CALLBACK(cmd_cfw_click), NULL);
}

void cmb_cfwcfg_build()
{
	cmb_cfwcfg = gtk_combo_box_text_new();
	gtk_widget_set_size_request(cmb_cfwcfg, 140, 30);
	#if GTK_MAJOR_VERSION == 3
	gtk_combo_wakeup(cmb_cfwcfg);
	#endif
	gtk_widget_set_sensitive(cmb_cfwcfg, 0);
	
	g_signal_connect(G_OBJECT(cmb_cfwcfg), "changed", G_CALLBACK(cmb_cfwcfg_changed),  NULL);	
}

void cmd_cfwrst_build()
{
	cmd_cfwrst = gtk_button_new_with_label(C_("cfw","Factory Reset"));
	gtk_widget_set_size_request(cmd_cfwrst, 140, 30);
	gtk_widget_set_sensitive(cmd_cfwrst, 0);
	
	// Callbacks
	g_signal_connect(G_OBJECT(cmd_cfwrst), "clicked", G_CALLBACK(cmd_cfwrst_click), NULL);
}

void cmb_cfwwhl_build()
{
	int i = 0;
	
	for (i = 0; i < CFW_SLOTS; i++)
	{
		cmb_cfwwhl[i] = gtk_combo_box_text_new();
		gtk_widget_set_size_request(cmb_cfwwhl[i], 140, 30);
		#if GTK_MAJOR_VERSION == 3
		gtk_combo_wakeup(cmb_cfwwhl[i]);
		#endif
		gtk_widget_set_sensitive(cmb_cfwwhl[i], 0);
		combo_setlist(cmb_cfwwhl[i], fltstr);		
	}
	for (i = 0; i < CFW_SLOTS; i++)
	{
		g_signal_connect(G_OBJECT(cmb_cfwwhl[i]), "changed", G_CALLBACK(cmb_cfwwhl_changed), cmb_cfwwhl);	
	}
}

void cmd_cfwwhl_build()
{
	int i = 0;
	char lbl[32];
	
	for (i = 0; i < CFW_SLOTS; i++)
	{
		/// Label for button to change wheel slot to %d
		sprintf(lbl, C_("cfw","Slot %d"), (i + 1));
		cmd_cfwwhl[i] = gtk_button_new_with_label(lbl);
		gtk_widget_set_size_request(cmd_cfwwhl[i], 140, 30);
		gtk_widget_set_sensitive(cmd_cfwwhl[i], 0);
		combo_setlist(cmb_cfwwhl[i], fltstr);		
	
		g_signal_connect(G_OBJECT(cmd_cfwwhl[i]), "clicked", G_CALLBACK(cmd_cfwwhl_click),  (gpointer)i);	
	}
}

void box_ccd_build()
{
	box_ccd = gtk_table_new(15, 13, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_ccd), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_ccd), 4);
	gtk_container_set_border_width(GTK_CONTAINER(box_ccd), 4);

	cmb_camera_build();
	cmd_camera_build();
	cmd_setcamlst_build();
	cmd_updcamlst_build();
	cmd_resetcam_build();
	cmb_bin_build();
	hsc_offset_build();
	hsc_gain_build();
	cmb_csize_build();
	cmb_dspeed_build();
	cmb_mode_build();
	cmb_amp_build();
	cmb_denoise_build();
	cmb_depth_build();
	cmb_debayer_build();
	
	// MP mode label
	lbl_mode = gtk_label_new_with_align("", 0.0, 0.5, 70, 30);
	
	// Buttons	
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Camera Controls"), 0.5, 0.5, 150, 30), 0, 6, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_vseparator_new(), 6, 7, 0, 15, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","User Profiles Settings"), 0.5, 0.5, 150, 30), 7, 13, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	
	// Basics
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Model"), 0.0, 0.5, 40, 30), 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_camera, 1, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmd_camera, 4, 6, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), cmd_setcamlst, 0, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmd_updcamlst, 2, 4, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmd_resetcam , 4, 6, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Offset"), 0.0, 0.5, 90, 30), 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), hsc_offset, 1, 6, 3, 4, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Gain"), 0.0, 0.5, 90, 30), 0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), hsc_gain, 1, 6, 4, 5, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Binning Mode"), 0.0, 0.5, 70, 30), 0, 3, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_bin, 3, 6, 5, 6, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Capture Size"), 0.0, 0.5, 70, 30), 0, 3, 6, 7, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_csize, 3, 6, 6, 7, GTK_FILL, GTK_FILL, 0, 0);
		
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Download Speed"), 0.0, 0.5, 70, 30), 0, 3, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_dspeed, 3, 6, 7, 8, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), lbl_mode, 0, 3, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_mode, 3, 6, 8, 9, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Amp Control"), 0.0, 0.5, 70, 30), 0, 3, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_amp, 3, 6, 9, 10, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Noise reduction"), 0.0, 0.5, 70, 30), 0, 3, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_denoise, 3, 6, 10, 11, GTK_FILL, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Image depth"), 0.0, 0.5, 70, 30), 0, 3, 11, 12, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_depth, 3, 6, 11, 12, GTK_FILL, GTK_FILL, 0, 0);

	// Color mode
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_hseparator_new(), 0, 6, 12, 13, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), gtk_label_new_with_align(C_("settings","Color mode"), 0.0, 0.5, 70, 30), 0, 3, 13, 14, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_ccd), cmb_debayer, 3, 6, 13, 14, GTK_FILL, GTK_FILL, 0, 0);
}

void box_cooling_build()
{
	box_cooling = gtk_table_new(15, 14, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_cooling), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_cooling), 4);
	gtk_container_set_border_width(GTK_CONTAINER(box_cooling), 4);

	cmd_tecenable_build();
	cmd_tecauto_build();
	spn_tectgt_build();
	vsc_tectemp_build();
	frm_tecgraph_build();
	vsc_tecpwr_build();

	gtk_table_attach(GTK_TABLE(box_cooling),       cmd_tecenable,  0,  6,  0,  1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),         cmd_tecauto,  6, 12,  0,  1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),          spn_tectgt, 12, 14,  0,  1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),gtk_hseparator_new(),  0, 14,  1,  2, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cooling),gtk_label_new_with_align(C_("cooling","Current °C"), 1, 0.5, 70, 30),  0,  2,  2,  3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),gtk_label_new_with_align(C_("cooling","Temperature / Time"), 0.5, 0.5, 140, 30),  2, 12,  2,  3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),gtk_label_new_with_align(C_("cooling","Power %"), 0.0, 0.5, 70, 30), 12, 14,  2,  3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),gtk_hseparator_new(),  0, 14,  3,  4, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cooling),         vsc_tectemp,  0,  2,  4,  7, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),        frm_tecgraph,  2, 12,  4,  7, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),          vsc_tecpwr, 12, 14,  4,  7, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cooling),gtk_hseparator_new(),  0, 14,  7,  8, GTK_FILL, GTK_FILL, 0, 0);	
}

void box_filename_build()
{
	box_filename = gtk_table_new(15, 12, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_filename), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_filename), 4);
	gtk_container_set_border_width(GTK_CONTAINER(box_filename), 4);
	
	cmd_saveas_build();
	cmd_dateadd_build();
	cmd_timeadd_build();
	cmd_fltadd_build();
	cmb_flt_build();
	txt_fitfolder_build();
	txt_fitbase_build();
	cmd_audela_build();
	cmd_iris_build();
	cmd_zerofc_build();
	cmd_tlenable_build();
	
	//gtk_table_attach(GTK_TABLE(box_filename), gtk_label_new_with_align("", 0.5, 0.5), 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),    cmd_saveas,  0,  2,  1,  2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),   cmd_dateadd,  2,  4,  1,  2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),   cmd_timeadd,  4,  6,  1,  2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),    cmd_fltadd,  6,  8,  1,  2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename), txt_fitfolder,  0,  8,  2,  3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),   txt_fitbase,  0,  8,  3,  4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),    cmd_audela,  0,  2,  4,  5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),      cmd_iris,  2,  4,  4,  5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),    cmd_zerofc,  4,  6,  4,  5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),       cmb_flt,  6,  8,  4,  5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_filename),  cmd_tlenable,  0,  2,  5,  6, GTK_FILL, GTK_FILL, 0, 0);
}

void box_timelapse_build()
{
	time_t localt;
	char strdate[16];
	
	localt = time(NULL);
	tlstart = *(localtime(&localt));
	tlend = *(localtime(&localt));

	box_timelapse = gtk_table_new(15, 12, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_timelapse), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_timelapse), 4);
	gtk_container_set_border_width(GTK_CONTAINER(box_timelapse), 4);
	gtk_widget_set_sensitive(box_timelapse, 0);	
	
	// Start	
	rbt_tlstart = gtk_radio_button_new_with_label(NULL, C_("timelapse","Start"));
	gtk_widget_set_size_request(rbt_tlstart, 140, 25);
	gtk_widget_set_sensitive(rbt_tlstart, 0);
	
	lbl_tlstart = gtk_label_new_with_align("", 0.0, 0.5, 100, 25);
	gtk_label_set_max_width_chars(GTK_LABEL(lbl_tlstart), 10);
	gtk_widget_set_sensitive(lbl_tlstart, 0);

	spn_tlhstart = gtk_spin_button_new_with_range (0.0, 23.0, 1.0);
	gtk_widget_set_size_request(spn_tlhstart, 70, 25);
	gtk_widget_set_sensitive(spn_tlhstart, 0);

	spn_tlmstart = gtk_spin_button_new_with_range (0.0, 59.0, 1.0);
	gtk_widget_set_size_request(spn_tlmstart, 70, 25);
	gtk_widget_set_sensitive(spn_tlmstart, 0);

	spn_tlsstart = gtk_spin_button_new_with_range (0.0, 59.0, 1.0);
	gtk_widget_set_size_request(spn_tlsstart, 70, 25);
	gtk_widget_set_sensitive(spn_tlsstart, 0);

	// End
	rbt_tlend = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rbt_tlstart), C_("timelapse","End "));
	gtk_widget_set_size_request(rbt_tlend, 140, 25);
	gtk_widget_set_sensitive(rbt_tlend, 0);

	lbl_tlend = gtk_label_new_with_align("", 0.0, 0.5, 100, 25);
	gtk_label_set_max_width_chars(GTK_LABEL(lbl_tlend), 10);
	gtk_widget_set_sensitive(lbl_tlend, 0);

	spn_tlhend = gtk_spin_button_new_with_range (0.0, 23.0, 1.0);
	gtk_widget_set_size_request(spn_tlhend, 70, 25);
	gtk_widget_set_sensitive(spn_tlhend, 0);

	spn_tlmend = gtk_spin_button_new_with_range (0.0, 59.0, 1.0);
	gtk_widget_set_size_request(spn_tlmend, 70, 25);
	gtk_widget_set_sensitive(spn_tlmend, 0);

	spn_tlsend = gtk_spin_button_new_with_range (0.0, 59.0, 1.0);
	gtk_widget_set_size_request(spn_tlsend, 70, 25);
	gtk_widget_set_sensitive(spn_tlsend, 0);
	
	strftime(strdate, 16, "%Y/%m/%d" ,&tlstart);
	gtk_label_set_text(GTK_LABEL(lbl_tlstart), (gchar *) strdate);
	strftime(strdate, 16, "%Y/%m/%d" ,&tlend);
	gtk_label_set_text(GTK_LABEL(lbl_tlend), (gchar *) strdate);

	// Init values for both
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlhstart), tlstart.tm_hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlmstart), tlstart.tm_min);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlsstart), tlstart.tm_sec);
	//
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlhend), tlend.tm_hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlmend), tlend.tm_min);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlsend), tlend.tm_sec);

	// Callbacks for both
	g_signal_connect(G_OBJECT(rbt_tlstart), "clicked", G_CALLBACK(rbt_tlstart_click), NULL);
	g_signal_connect(G_OBJECT(spn_tlhstart), "value-changed", G_CALLBACK(spn_tlhstart_changed),  NULL);
	g_signal_connect(G_OBJECT(spn_tlmstart), "value-changed", G_CALLBACK(spn_tlmstart_changed),  NULL);
	g_signal_connect(G_OBJECT(spn_tlsstart), "value-changed", G_CALLBACK(spn_tlsstart_changed),  NULL);
	//
	g_signal_connect(G_OBJECT(rbt_tlend), "clicked", G_CALLBACK(rbt_tlend_click), NULL);
	g_signal_connect(G_OBJECT(spn_tlhend), "value-changed", G_CALLBACK(spn_tlhend_changed),  NULL);
	g_signal_connect(G_OBJECT(spn_tlmend), "value-changed", G_CALLBACK(spn_tlmend_changed),  NULL);
	g_signal_connect(G_OBJECT(spn_tlsend), "value-changed", G_CALLBACK(spn_tlsend_changed),  NULL);
	//
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbt_tlstart), TRUE);
	
	cmd_tlcalendar_build();
	cal_tldpick_build();
	hsc_tlperiod_build();
	
	spn_tlperiod = gtk_spin_button_new_with_range (0.0, 86400.0, 1.0);
	gtk_widget_set_size_request(spn_tlperiod, 70, 25);
	g_signal_connect(G_OBJECT(spn_tlperiod), "value-changed", G_CALLBACK(spn_tlperiod_changed),  NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spn_tlperiod), 1.0);
	
	gtk_table_attach(GTK_TABLE(box_timelapse), cmd_tlcalendar,  0, 10,  0,  1, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse), gtk_hseparator_new(), 0,  10,  1,  2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_timelapse),    rbt_tlstart,  0,  2,  2,  3, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),    lbl_tlstart,  2,  4,  2,  3, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),   spn_tlhstart,  4,  6,  2,  3, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),   spn_tlmstart,  6,  8,  2,  3, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),   spn_tlsstart,  8, 10,  2,  3, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse), gtk_hseparator_new(), 0, 10,  3,  4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_timelapse),      rbt_tlend,  0,  2,  4,  5, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),      lbl_tlend,  2,  4,  4,  5, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),     spn_tlhend,  4,  6,  4,  5, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),     spn_tlmend,  6,  8,  4,  5, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),     spn_tlsend,  8, 10,  4,  5, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse), gtk_hseparator_new(), 0, 10,  5,  6, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_timelapse),    cal_tldpick,  0, 10,  6, 11, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse), gtk_hseparator_new(), 0, 10,  11,  12, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_timelapse),   hsc_tlperiod,  0, 8, 12, 13, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse),   spn_tlperiod,  8, 10, 12, 13, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_timelapse), gtk_hseparator_new(), 0, 10,  13,  14, GTK_FILL, GTK_FILL, 0, 0);
}

void box_cfw_build()
{
	int i = 0;
	
	box_cfw = gtk_table_new(20, 12, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_cfw), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_cfw), 8);
	gtk_container_set_border_width(GTK_CONTAINER(box_cfw), 4);
	
	cmb_cfw_build();
	cmb_cfwtty_build();
	cmd_cfwtty_build();
	cmd_cfw_build();
	cmb_cfwcfg_build();
	cmd_cfwrst_build();
	cmb_cfwwhl_build();
	cmd_cfwwhl_build();

	gtk_table_attach(GTK_TABLE(box_cfw), gtk_label_new_with_align(C_("cfw","Connection"), 0.0, 0.5, 140, 30), 0, 2, 0, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cfw), cmb_cfw   ,  2,  5,  0,  1, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cfw), cmb_cfwtty,  5,  8,  0,  1, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cfw), cmd_cfw   ,  2,  5,  1,  2, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cfw), cmd_cfwtty,  5,  8,  1,  2, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cfw), gtk_hseparator_new(),  0, 12,  2,  3, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cfw), gtk_label_new_with_align(C_("cfw","Configuration"), 0.0, 0.5, 140, 30), 0, 2, 3, 14, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_cfw), cmb_cfwcfg,  2,  5,  3,  4, GTK_FILL, GTK_FILL, 0, 0);	
	gtk_table_attach(GTK_TABLE(box_cfw), cmd_cfwrst,  5,  8,  3,  4, GTK_FILL, GTK_FILL, 0, 0);	
	for (i = 0; i < CFW_SLOTS; i++)
	{
		gtk_table_attach(GTK_TABLE(box_cfw), cmb_cfwwhl[i],  2,  5,  (4 + i),  (5 + i), GTK_FILL, GTK_FILL, 0, 0);	
	}
	for (i = 0; i < CFW_SLOTS; i++)
	{
		gtk_table_attach(GTK_TABLE(box_cfw), cmd_cfwwhl[i],  5,  8,  (4 + i),  (5 + i), GTK_FILL, GTK_FILL, 0, 0);	
	}
	gtk_table_attach(GTK_TABLE(box_cfw), gtk_hseparator_new(),  0, 12, 14, 15, GTK_FILL, GTK_FILL, 0, 0);	
}

void tab_settings_build()
{
	// Notebook (tab) for settings...
	tab_settings = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tab_settings), GTK_POS_TOP);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK(tab_settings), TRUE);

	box_ccd_build();
	box_cooling_build();
	box_filename_build();
	box_timelapse_build();
	box_scripting = gtk_vbox_new(FALSE, 4);
	box_header    = gtk_vbox_new(FALSE, 4);
	box_cfw_build();
	box_calc      = gtk_vbox_new(FALSE, 4);
	
	// Pack into tab_settings
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_ccd, gtk_label_new(C_("main","Camera")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_cooling, gtk_label_new(C_("main","Cooling")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_filename, gtk_label_new(C_("main","Filename")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_scripting, gtk_label_new(C_("main","Scripting")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_timelapse, gtk_label_new(C_("main","Timelapse")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_header, gtk_label_new(C_("main","Fits")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_cfw, gtk_label_new(C_("main","CFW")));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_settings), box_calc, gtk_label_new(C_("main","FOV Calc")));

	// Callback
	//g_signal_connect(G_OBJECT(tab_settings), "size-allocate", G_CALLBACK(tab_settings_allocate), NULL);	

}

void box_top_left_build()
{
	// Button box left
	box_top_left = gtk_table_new(10, 6, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_top_left), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_top_left), 4);
	// This is what sets the left part minimim hsize
	gtk_widget_set_size_request(box_top_left, 190, 300);
	gtk_container_set_border_width(GTK_CONTAINER(box_top_left), 4);

	cmd_settings_build();
	cmd_about_build();
	cmd_capture_build();
	cmb_exptime_build();
	spn_expnum_build();
	spn_shots_build();
	pbr_expnum_build();
	cmd_run_build();
	pbr_exptime_build();
	cmd_hold_build();
	cmd_load_build();
	cmd_fit_build();

	// Pack into box_top_left
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_about   , 0, 1, 0,  1, GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_settings, 1, 6, 0,  1, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_capture,  0, 6, 1,  2, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), gtk_label_new_with_align(C_("main","Exp. time (s)"), 0.0, 0.5, 90, 30), 0, 4, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), cmb_exptime,  4, 6, 2,  3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), gtk_label_new_with_align(C_("main","Total shots"), 0.0, 0.5, 90, 30), 0, 4, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), spn_expnum ,  4, 6, 3,  4, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), gtk_label_new_with_align(C_("main","Saved shots"), 0.0, 0.5, 90, 30), 0, 4, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), spn_shots ,   4, 6, 4,  5, GTK_EXPAND |GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), pbr_expnum,   0, 6, 5,  6, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 5);
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_run,      0, 6, 6,  7, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), pbr_exptime,  0, 6, 7,  8, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 5);
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_hold,     0, 6, 8,  9, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_load,     0, 3, 9, 10, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_top_left), cmd_fit,      3, 6, 9, 10, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

void box_bot_left_build()
{
	// Button box left
	box_bot_left = gtk_table_new(10, 6, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(box_bot_left), 4);
	gtk_table_set_col_spacings(GTK_TABLE(box_bot_left), 4);
	gtk_widget_set_size_request(box_bot_left, 190, -1);
	gtk_container_set_border_width(GTK_CONTAINER(box_bot_left), 4);

	cmd_histogram_build();
	spn_histogram_build();
	hsc_maxadu_build();
	frm_histogram_build();
	hsc_minadu_build();

	// Pack into btnbox
	gtk_table_attach(GTK_TABLE(box_bot_left), cmd_histogram, 0, 4, 0,  1, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_bot_left), spn_histogram, 4, 6, 0,  1, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_bot_left), hsc_maxadu,    0, 6, 1,  2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_bot_left), frm_histogram, 0, 6, 2,  9, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_bot_left), hsc_minadu,    0, 6, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
}

void pnd_left_build()
{
	pnd_left = gtk_vpaned_new();
	gtk_paned_set_position(GTK_PANED(pnd_left), 300);

	box_top_left_build();
	box_bot_left_build();

	// Pack boxes into pnd_left
	gtk_paned_pack1(GTK_PANED(pnd_left), box_top_left, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(pnd_left), box_bot_left, TRUE, FALSE);
}

void tab_right_build()
{
	// Notebook (tab) for image / tab_settings / header ...
	tab_right = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tab_right), GTK_POS_TOP);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(tab_right), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(tab_right), FALSE);

	swindow_build();
	tab_settings_build();

	// Pack into tab_right
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_right), swindow, gtk_label_new("Image"));
	gtk_notebook_append_page(GTK_NOTEBOOK(tab_right), tab_settings, gtk_label_new("Settings"));
}

void pnd_main_build()
{
	pnd_main = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(pnd_main), 210);

	pnd_left_build();
	tab_right_build();

	// Pack boxes into pnd_main
	gtk_paned_pack1(GTK_PANED(pnd_main), pnd_left, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(pnd_main), tab_right, TRUE, FALSE);
}

void box_main_build()
{
	box_main = gtk_table_new(10, 5, FALSE);
	
	imgstatus_build();
	pnd_main_build();
	
	gtk_table_attach(GTK_TABLE(box_main), pnd_main,  0, 5, 0,  9, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(box_main), imgstatus, 0, 5, 9, 10, GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
}

void window_build()
{
	GError *error = NULL;
	GdkGeometry hints;

	// Main window definitions
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	hints.min_width = 750;
	hints.max_width = gdk_screen_get_width(gtk_widget_get_screen(window));
	hints.min_height = 500;
	hints.max_height = gdk_screen_get_height(gtk_widget_get_screen(window));
	gtk_window_set_title(GTK_WINDOW(window), APPTIT);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 780, 510);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	gtk_window_set_geometry_hints(GTK_WINDOW(window), window, &hints, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
	icopixbuf = gdk_pixbuf_new_from_file(APPICO, &error);
	if(icopixbuf) 
	{
		gtk_window_set_icon(GTK_WINDOW(window), icopixbuf);
	}
	else
	{
		g_error_free(error);
	}

	// Look up the default selected_bg_color in the theme, use for "active" 
	// background on toggle_buttons
	#if GTK_MAJOR_VERSION == 3
	GtkStyleContext *style = gtk_widget_get_style_context(window);
	gtk_style_context_lookup_color(style, "selected_bg_color", &clrSelected);
	gtk_style_context_lookup_color(style, "selected_bg_color", &clrKill);
	#else
	GtkWidget *tmp = gtk_toggle_button_new_with_label("Test");
	GtkStyle *style = gtk_rc_get_style(tmp);
	gtk_style_lookup_color(style, "selected_bg_color", &clrSelected);
	gtk_style_lookup_color(style, "selected_bg_color", &clrKill);
	g_object_ref_sink(tmp);
	#endif
	gtk_color_get_lighter(&clrSelected);
	gtk_color_get_alert(&clrKill);

	box_main_build();

	// Pack the pnd_main in window
	gtk_container_add(GTK_CONTAINER(window), box_main);

	// Callbacks
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK (mainw_delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK (mainw_destroy), NULL);
}

void imgwin_build()
{
	// Init locale conventions
	sysloc = localeconv();
	
	// Init decorations
	watchCursor = gdk_cursor_new(GDK_WATCH);
	
	// Code Init
	imgfit_init();
	imgcfw_init();
	/// Value "No cam" of the models combo
	imgcam_set_model(C_("camio","None"));

	// Main window build
	window_build();	
	// Draw all
	gtk_widget_show_all(window);
}

