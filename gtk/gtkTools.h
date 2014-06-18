/*
 * gtkTools.c
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

GtkWidget *gtk_label_new_with_align(const gchar *label, gfloat xalign, gfloat yalign, gint width, gint height);
int        gtk_statusbar_write (GtkStatusbar *statusbar, guint context_id, const gchar *text);
#if GTK_MAJOR_VERSION == 2
GtkWidget *gtk_toggle_button_new_with_label_color(const gchar *label, gint width, gint height, GdkColor *clrSel);
void       gtk_color_get_lighter (GdkColor *color);
void       gtk_color_get_alert (GdkColor *color);
void       gtk_widget_modify_bkg(GtkWidget *widget, GtkStateType state, GdkColor* color);
#else
GtkWidget *gtk_toggle_button_new_with_label_color(const gchar *label, gint width, gint height, GdkRGBA *clrSel);
void       gtk_combo_wakeup(GtkWidget *widget);
GtkWidget *gtk_paned_new_with_handle(GtkOrientation orientation);
void       gtk_color_get_lighter (GdkRGBA *color);
void       gtk_color_get_alert (GdkRGBA *color);
void       gtk_widget_modify_bkg(GtkWidget *widget, GtkStateType state, GdkRGBA* color);
#endif
int        gtk_combo_box_element_count(GtkWidget *cmb);
