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

#include <gtk/gtk.h>
#include "imgBase.h"
#include "gtkversions.h"
#include "gtkTools.h"

GtkWidget *gtk_label_new_with_align(const gchar *label, gfloat xalign, gfloat yalign, gint width, gint height)
{
	GtkWidget *lbl = gtk_label_new(label);
	gtk_widget_set_size_request(lbl, width, height);
	gtk_misc_set_alignment(GTK_MISC(lbl), xalign, yalign);
	if (strlen(label) > 0)
	{
		gtk_label_set_max_width_chars(GTK_LABEL(lbl), strlen(label));
	}
	else
	{
		gtk_label_set_max_width_chars(GTK_LABEL(lbl), 999);
	}
	return lbl;
}

int gtk_statusbar_write (GtkStatusbar *statusbar, guint context_id, const gchar *text)
{
	gtk_statusbar_remove_all(statusbar, context_id);
	return gtk_statusbar_push(statusbar, context_id, text);
}

#if GTK_MAJOR_VERSION == 2
GtkWidget *gtk_toggle_button_new_with_label_color(const gchar *label, gint width, gint height, GdkColor *clrSel)
{
	GtkWidget *cmd = gtk_toggle_button_new_with_label(label);
	gtk_widget_set_size_request(cmd, width, height);
	gtk_widget_modify_bg(cmd, GTK_STATE_ACTIVE, clrSel);
	return cmd;
}

void gtk_color_get_lighter (GdkColor *color)
{
	int reF = MAX(color->red, MAX(color->green,color->blue));
	
	// After determining the "dominant value", it halves the difference, thus creating a less "saturated" color 
	color->red   = MIN(65535, (color->red  + ((color->red < reF) ? ((reF - color->red) / 2.) : 0)));
	color->green = MIN(65535, (color->green + ((color->green < reF) ? ((reF - color->green) / 2.) : 0)));
	color->blue  = MIN(65535, (color->blue  + ((color->blue < reF) ? ((reF - color->blue) / 2.) : 0)));
}

void gtk_color_get_alert (GdkColor *color)
{
	// Gets the "red based" color with similar RGB mix
	// If input color is "red based" already, a blue based color is returned.
	int reF = MAX(color->red, MAX(color->green,color->blue));
	int ref = MIN(color->red, MIN(color->green,color->blue));

	if (color->red == reF)
	{
		// Color is "red based" -> "blue based" return
		color->red   = ref / 2;
		color->green = ref / 2;
		color->blue  = reF;
	}
	else 
	{ 
		color->red   = reF;
		color->green = ref / 2;
		color->blue  = ref / 2;
	}
}

void gtk_widget_modify_bkg(GtkWidget *widget, GtkStateType state, GdkColor* color)
{
	gtk_widget_modify_bg(widget, state, color);
}
#else
GtkWidget *gtk_toggle_button_new_with_label_color(const gchar *label, gint width, gint height, GdkRGBA *clrSel)
{
	GtkWidget *cmd = gtk_toggle_button_new_with_label(label);

	gtk_widget_set_size_request(cmd, width, height);
	gtk_widget_modify_bkg(cmd, GTK_STATE_ACTIVE, clrSel);
	return cmd;
}

void gtk_combo_wakeup(GtkWidget *widget)
{
	// Nth workaround on GT3 bug. 
	// Auto sensitivity needs to be triggered by one fake element add and delete
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "");
	gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget))));
	gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(widget), GTK_SENSITIVITY_AUTO);
}

GtkWidget *gtk_paned_new_with_handle(GtkOrientation orientation)
{
	GtkWidget *pnd = gtk_paned_new(orientation);
	GtkStyleContext *context = gtk_widget_get_style_context(pnd);
	GtkCssProvider  *cssprv =  gtk_css_provider_new();
	GdkRGBA clrSel = {0.,0.,0.,0.};
	char css[2048];
	
	gtk_style_context_lookup_color(context, "bg_color", &clrSel);
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		sprintf(css, "* {\n -GtkPaned-handle-size: 3; \n } \n .pane-separator.horizontal {\n background-image: -gtk-gradient (linear, left top, left bottom, from (%s), color-stop (0.5, shade (%s, 0.95)), to (%s)); \n -unico-centroid-texture: url(\"%s\");\n }\n", gdk_rgba_to_string(&clrSel), gdk_rgba_to_string(&clrSel), gdk_rgba_to_string(&clrSel), ORZHND);
	}
	else
	{
		sprintf(css, "* {\n -GtkPaned-handle-size: 3; \n } \n \n .pane-separator.vertical {\n background-image: -gtk-gradient (linear, left top, right top, from (%s), color-stop (0.5, shade (%s, 0.95)), to (%s)); \n -unico-centroid-texture: url(\"%s\");\n }\n", gdk_rgba_to_string(&clrSel), gdk_rgba_to_string(&clrSel), gdk_rgba_to_string(&clrSel), VRTHND);
	}
	gtk_style_context_remove_provider(context, GTK_STYLE_PROVIDER (cssprv));
	gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER (cssprv), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_css_provider_load_from_data(cssprv, css, -1, NULL);
	g_object_unref(cssprv);
	return pnd;
}

void gtk_color_get_lighter (GdkRGBA *color)
{
	double reF = MAX(color->red, MAX(color->green,color->blue));
	
	// After determining the "dominant value", it halves the difference, thus creating a less "saturated" color 
	color->red   = MIN(1., (color->red  + ((color->red < reF) ? ((reF - color->red) / 2.) : 0)));
	color->green = MIN(1., (color->green + ((color->green < reF) ? ((reF - color->green) / 2.) : 0)));
	color->blue  = MIN(1., (color->blue  + ((color->blue < reF) ? ((reF - color->blue) / 2.) : 0)));
}

void gtk_color_get_alert (GdkRGBA *color)
{
	// Gets the "red based" color with similar RGB mix
	// If input color is "red based" already, a blue based color is returned.
	double reF = MAX(color->red, MAX(color->green,color->blue));
	double ref = MIN(color->red, MIN(color->green,color->blue));
	
	if (color->red == reF)
	{
		// Color is "red based" -> "blue based" return
		color->red   = ref / 2.;
		color->green = ref / 2.;
		color->blue  = reF;
	}
	else 
	{ 
		color->red   = reF;
		color->green = ref / 2.;
		color->blue  = ref / 2.;
	}
}

void gtk_widget_modify_bkg(GtkWidget *widget, GtkStateType state, GdkRGBA* color)
{
	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	GtkCssProvider  *cssprv =  gtk_css_provider_new();
	char css[2048], strstate[256];
	
	strstate[0] = '\0';
	if (state == 0)
	{
		//GTK_STATE_FLAG_NORMAL
		strcat(strstate, "");
	}
	if ((state << 0) == 1)
	{
		//GTK_STATE_FLAG_ACTIVE
		strcat(strstate, ":active");
	}
	if ((state << 1) == 1)
	{
		//GTK_STATE_FLAG_PRELIGHT
		strcat(strstate, ":hoover");
	}
	if ((state << 2) == 1)
	{
		//GTK_STATE_FLAG_SELECTED
		strcat(strstate, ":focus");
	}
	if ((state << 3) == 1)
	{
		//GTK_STATE_FLAG_INSENSITIVE
		strcat(strstate, ":insensitive");
	}
	if ((state << 4) == 1)
	{
		//GTK_STATE_FLAG_INCONSISTENT
		strcat(strstate, "");
	}
	if ((state << 5) == 1)
	{
		//GTK_STATE_FLAG_FOCUSED
		strcat(strstate, "focus");
	}
	//sprintf(css, "%s%s {\n background-color: %s;\n background-image: none;\n }\n", G_OBJECT_TYPE_NAME(widget), strstate, gdk_rgba_to_string(color));
	sprintf(css, "%s%s {\n background-image: -gtk-gradient (linear, left top, left bottom, from (shade (%s, 1.1)), color-stop (0.5, shade (%s, 0.85)), to (shade (%s, 1.1)));\n }\n", G_OBJECT_TYPE_NAME(widget), strstate, gdk_rgba_to_string(color), gdk_rgba_to_string(color), gdk_rgba_to_string(color));
	gtk_style_context_remove_provider(context, GTK_STYLE_PROVIDER (cssprv));
	gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER (cssprv), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_css_provider_load_from_data(cssprv, css, -1, NULL);
	g_object_unref(cssprv);
}
#endif
