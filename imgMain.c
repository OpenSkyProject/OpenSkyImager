/*
 * imgMain.c
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

#include "imgBase.h"
#include "imgWindow.h"

int main(int argc, char* argv[])
{
	setlocale(LC_ALL,"en_US");
	bindtextdomain(APPNAM,".");
	textdomain(APPNAM);
	#if GLIB_MINOR_VERSION < 32
	if(!g_thread_supported())
	{
	    g_thread_init( NULL );
	}
	#endif
	gtk_init(&argc, &argv);
	imgwin_build();
	gtk_main();
	
	return EXIT_SUCCESS;
}
