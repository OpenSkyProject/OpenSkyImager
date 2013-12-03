/*
 * imgWCallbacks.h
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

gboolean tmr_img_refresh (GtkWidget *widget);
gboolean tmr_adu_check (GtkWidget *widget);
gboolean tmr_frm_refresh (GtkWidget *widget);
gboolean tmr_hst_refresh (GtkWidget *widget);
gboolean tmr_capture_progress_refresh (GtkWidget *widget);
gboolean tmr_exp_progress_refresh (GtkWidget *widget);
gboolean tmr_tecpwr (GtkWidget *widget);
gboolean tmr_tecstatus_write (GtkWidget *widget);
gboolean tmr_tlrefresh (GtkWidget *widget);
void imgstatus_push (GtkStatusbar *statusbar, guint context_id, gchar *text, gpointer user_data);
void cmd_settings_click(GtkWidget *widget, gpointer data);
void cmd_about_click(GtkWidget *widget, gpointer data);
gboolean spn_expnum_changed(GtkSpinButton *spinbutton, gpointer user_data);
gboolean spn_shots_changed(GtkSpinButton *spinbutton, gpointer user_data);
void cmd_capture_click(GtkWidget *widget, gpointer data);
void cmd_load_click(GtkWidget *widget, gpointer data);
void cmd_run_click(GtkWidget *widget, gpointer data);
void cmd_hold_click(GtkWidget *widget, gpointer data);
void cmd_fit_click(GtkWidget *widget, gpointer data);
void cmd_histogram_click(GtkWidget *widget, gpointer data);
gboolean spn_histogram_changed (GtkSpinButton *spinbutton, gpointer user_data);
gboolean hsc_maxadu_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
gboolean hsc_minadu_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
gboolean frm_histogram_allocate(GtkWidget *widget, GdkRectangle *alloc, gpointer data);
gboolean swindow_allocate(GtkWidget *widget, GdkRectangle *alloc, gpointer data);
void mainw_destroy( GtkWidget *widget, gpointer   data );
gboolean mainw_delete_event( GtkWidget *widget, GdkEvent *event, gpointer data);
void cmb_debayer_changed (GtkComboBox *widget, gpointer user_data);
void cmb_exptime_changed (GtkComboBox *widget, gpointer user_data);
void cmb_camera_changed (GtkComboBox *widget, gpointer user_data);
void cmd_camera_click(GtkWidget *widget, gpointer data);
void cmd_setcamlst_click(GtkWidget *widget, gpointer data);
void cmd_updcamlst_click(GtkWidget *widget, gpointer data);
void cmd_resetcam_click(GtkWidget *widget, gpointer data);
gboolean hsc_gain_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
gboolean hsc_offset_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
void cmb_bin_changed (GtkComboBox *widget, gpointer user_data);
void cmb_csize_changed (GtkComboBox *widget, gpointer user_data);
void cmb_dspeed_changed (GtkComboBox *widget, gpointer user_data);
void cmb_mode_changed (GtkComboBox *widget, gpointer user_data);
void cmb_amp_changed (GtkComboBox *widget, gpointer user_data);
void cmb_denoise_changed (GtkComboBox *widget, gpointer user_data);
void cmb_depth_changed (GtkComboBox *widget, gpointer user_data);
void cmd_saveas_click(GtkWidget *widget, gpointer data);
void cmd_audela_click(GtkWidget *widget, gpointer data);
void cmd_iris_click(GtkWidget *widget, gpointer data);
void cmd_zerofc_click(GtkWidget *widget, gpointer data);
void cmd_tlenable_click(GtkWidget *widget, gpointer data);
void rbt_tlstart_click(GtkWidget *widget, gpointer data);
void rbt_tlend_click(GtkWidget *widget, gpointer data);
gboolean spn_tlhstart_changed (GtkSpinButton *spinbutton, gpointer user_data);
gboolean spn_tlmstart_changed (GtkSpinButton *spinbutton, gpointer user_data);
gboolean spn_tlsstart_changed (GtkSpinButton *spinbutton, gpointer user_data);
gboolean spn_tlhend_changed (GtkSpinButton *spinbutton, gpointer user_data);
gboolean spn_tlmend_changed (GtkSpinButton *spinbutton, gpointer user_data);
gboolean spn_tlsend_changed (GtkSpinButton *spinbutton, gpointer user_data);
void cmd_tlcalendar_click(GtkWidget *widget, gpointer data);
void cal_tldpick_dblclick (GtkCalendar *calendar, gpointer user_data);
gboolean hsc_tlperiod_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
gboolean spn_tlperiod_changed (GtkSpinButton *spinbutton, gpointer user_data);
void txt_fitfolder_changed(GtkEditable *editable, gpointer user_data);
void txt_fitbase_changed(GtkEditable *editable, gpointer user_data);
void cmd_dateadd_click(GtkWidget *widget, gpointer data);
void cmd_timeadd_click(GtkWidget *widget, gpointer data);
void cmd_fltadd_click(GtkWidget *widget, gpointer data);
void cmb_flt_changed (GtkComboBox *widget, gpointer user_data);
void cmd_tecenable_click(GtkWidget *widget, gpointer data);
void cmd_tecauto_click(GtkWidget *widget, gpointer data);
gboolean spn_tectgt_changed(GtkSpinButton *spinbutton, gpointer user_data);
gboolean vsc_tecpwr_changed (GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
void cmb_cfw_changed (GtkComboBox *widget, gpointer user_data);
void cmb_cfwtty_changed (GtkComboBox *widget, gpointer user_data);
void cmd_cfwtty_click(GtkWidget *widget, gpointer data);
void cmd_cfw_click(GtkWidget *widget, gpointer data);
void cmb_cfwcfg_changed (GtkComboBox *widget, gpointer user_data);
void cmd_cfwrst_click(GtkWidget *widget, gpointer data);
void cmb_cfwwhl_changed (GtkComboBox *widget, GtkWidget **awidget);
void cmd_cfwwhl_click (GtkComboBox *widget, gpointer user_data);
