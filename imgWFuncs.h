/*
 * imgWFuncs.h
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

void get_filename(char **filename, int mode, char* flt);
void set_img_fit();
void set_img_full();
void set_adu_limits(int bytepix);
void load_image_from_data();
void load_histogram_from_data();
void load_histogram_from_null();
void tec_init_graph();
void tec_show_graph();
void tec_print_graph();
void combo_setlist(GtkWidget *cmb, char *str);
void combo_ttylist(GtkWidget *cmb);
void cfwmsgdestroy(int response);
void filenaming(char *thdfit);
void shotsnaming(char *thdfit, int thdshots, int thdpreshots);
gpointer thd_capture_run(gpointer thd_data);
gpointer thd_pixbuf_run(gpointer thd_data);
gpointer thd_fitsav_run(gpointer thd_data);
gpointer thd_avisav_run(gpointer thd_data);
gpointer thd_temp_run(gpointer thd_data);

