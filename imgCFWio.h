/*
 * imgCFWio.h
 *
 *  Created on: 20.11.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Device access code (QHY-USB) is based on original QHYCCD.inc code from 
 * https://github.com/qhyccd-lzr
 *
 * Device access code (QHY-TTY) is original work based on the protocol document 
 * as released by QHYCCD.inc:
 * http://qhyccd.com/en/left/qhy-colorwheel/
 * http://qhyccd.com/ccdbbs/index.php?topic=1083.0
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

void  imgcfw_init();
char  *imgcfw_get_msg();
int   imgcfw_set_mode(int mode);
int   imgcfw_get_mode();
int   imgcfw_set_tty(char *tty);
const char *imgcfw_get_tty();
int   imgcfw_connect();
int   imgcfw_disconnect();
int   imgcfw_read_all();
int   imgcfw_set_model(char *model);
char *imgcfw_get_model();
char *imgcfw_get_models();
int   imgcfw_get_slotcount();
int   imgcfw_get_slot();
int imgcfw_set_slot(int slot, gpointer (*postProcess)(int));
