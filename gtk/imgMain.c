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
#define DECLARE_MAIN
#include "imgBase.h"
#include "imgFifoio.h"
#include "imgWindow.h"

static gchar *FPath = NULL;
static GOptionEntry options[] =
{
	{ "fifo", 'f', 0, G_OPTION_ARG_NONE, &fifomode, "Create / Open '/tmp/<program_name>' named pipe for command input", NULL },
	{ "custom_fifo", 'F', 0, G_OPTION_ARG_STRING, &FPath, "custom fifo path/name", NULL },
	{ NULL }
};

int main(int argc, char* argv[])
{
	GError *error = NULL;

    	// Read cpuinfo
	cpucores = get_cpu_cores();
	printf("Detected %d cpu cores\n", cpucores);

	// Set app path
	readlink("/proc/self/exe", imgBasePath, PATH_MAX);
	strcpy(imgBasePath, g_path_get_dirname(imgBasePath));
	strcat(imgBasePath, "/");
	//printf("%s\n", imgBasePath);
	
	// Set Locale
	setlocale(LC_ALL,"en_US");
	bindtextdomain(APPNAM, imgBasePath);
	textdomain(APPNAM);
	
	// Set ico and handles
	strcpy(imgAppIco, imgBasePath);
	strcat(imgAppIco, APPICO);
	strcpy(imgOrzHnd, imgBasePath);
	strcat(imgOrzHnd, ORZHND);
	strcpy(imgVrtHnd, imgBasePath);
	strcat(imgVrtHnd, VRTHND);
	
	#if GLIB_MINOR_VERSION < 32
	if(!g_thread_supported())
	{
	    g_thread_init( NULL );
	}
	#endif

	// Init gtk env
	if (gtk_init_with_args(&argc, &argv, APPTIT, options, NULL, &error))
	{
		if (fifomode)
		{
			fifomode = FALSE;
			if (FPath != NULL)
			{
				sprintf(fifopath, "%s", FPath);
			}
			else
			{
				sprintf(fifopath, "%s%s", "/tmp/", g_get_prgname());
			}
			if (CreateFifo(fifopath))
			{
				fifofd = OpenFifo(fifopath);
				if (fifofd > -1)
				{
					fifoch = g_io_channel_unix_new(fifofd);
					fifotag = LinkFifo(fifofd, fifoch, (GIOFunc)fiforead(), NULL);
					if (fifotag > -1)
					{
						fifomode = TRUE;
						printf("Fifo: %s active\n", fifopath);
						// Set stdout as unbuffered in this case
						fflush(stdout);
						setvbuf(stdout,NULL,_IONBF,0);
					}
				}
			}
		}
		
		// Build window(s)
		imgwin_build();

		// Main loop start
		gtk_main();

		if (fifomode)
		{
			if (UnLinkFifo(fifotag, fifoch))
			{
				fifoch = NULL;
				if (CloseFifo(fifofd))
				{
					printf("Fifo: %s inactive\n", fifopath);
					if (DeleteFifo(fifopath))
					{
						printf("Fifo: %s deleted\n", fifopath);
					}
				}
			}
		}		
	}
	else
	{
		printf("GTK init failed: %s\n", error->message);	
		return EXIT_FAILURE;
	}
	// Return value	
	return EXIT_SUCCESS;
}
