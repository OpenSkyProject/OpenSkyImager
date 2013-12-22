/*
 * tools.c
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "tools.h"

// Hints got from: http://stackoverflow.com/questions/9629850/how-to-get-cpu-info-in-c-on-linux-such-as-number-of-cores
int get_cpu_cores(void)
{
	FILE *cmdline = fopen("/proc/cpuinfo", "rb");
	char *arg = 0;
	size_t size = 0;
	int curid = -1, newid = 0, val = 0, cpu = 0, retval = 0;
	
	while(getdelim(&arg, &size, '\n', cmdline) != -1)
	{
		retval = 1;
		if (strstr(arg, "physical id") != NULL)
		{
			sscanf(arg, "physical id	:%d", &newid);
		}
		else if(strstr(arg, "cpu cores") != NULL)
		{
			if (curid != newid)
			{
				curid = newid;
				sscanf(arg, "cpu cores	:%d", &val);
				cpu += val;
			}
		}
	}
	free(arg);
	fclose(cmdline);
	return (retval == 1) ? ((cpu > 0) ? cpu : 1) : 0;
}
//

int isdir(char* path)
{
	int retval = 0;
	struct stat st;
	if(stat(path, &st) == 0)
	{
	   retval = (S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode));
	}
	return (retval);
}

int isfile(char* path)
{
	int retval = 0;
	struct stat st;
	if(stat(path, &st) == 0)
	{
	   retval = (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode));
	}
	return (retval);
}

int isfifo(char* path)
{
	int retval = 0;
	struct stat st;
	if(stat(path, &st) == 0)
	{
	   retval = (S_ISFIFO(st.st_mode) || S_ISLNK(st.st_mode));
	}
	return (retval);
}

/* 
 *  Original code from StackOverflow: http://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix 
 *  Licensed under cc-wiki with attibution required as per: http://blog.stackoverflow.com/2009/06/attribution-required/ 
 */
int mkpath(char* file_path, mode_t mode) 
{
	int retval = 1;
	
	if (mode == 0)
	{
		mode = 0755;
	}
	
	if (file_path != NULL)
	{
		if (strlen(file_path) > 0)
		{
			char* p;
			for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/')) 
			{
				*p='\0';
				if (mkdir(file_path, mode) == -1) 
				{
					if (errno != EEXIST) 
					{ 
						*p='/';
						retval = 0;
						break; 
					}
				}
				*p='/';
			}
		}
	}
	return (retval);
}

