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
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include "tools.h"



// Hints got from: http://stackoverflow.com/questions/9629850/how-to-get-cpu-info-in-c-on-linux-such-as-number-of-cores
int get_cpu_cores(void)
{
	FILE *cmdline = fopen("/proc/cpuinfo", "rb");
	char *arg = 0;
	size_t size = 0;
	int curid = -1, newid = 0, val = 0, arm = 0, prc = 0, cpu = 0, retval = 0;
	
	while(getdelim(&arg, &size, '\n', cmdline) != -1)
	{
		retval = 1;
		// For arm we have to count processors, 
		// for others we need to count physical cores
		if (strstr(arg, "Processor") != NULL)
		{
			if (strstr(arg, "ARM") != NULL)
			{
				arm = 1;
			}
		}
		else if (strstr(arg, "processor") != NULL)
		{
				prc++;
		}
		else if (strstr(arg, "model name") != NULL)
		{
			if (strstr(arg, "ARM") != NULL)
			{
				arm = 1;
			}
		}
		else if (strstr(arg, "physical id") != NULL)
		{
			sscanf(arg, "physical id	:%d", &newid);
		}
		else if(strstr(arg, "cpu cores") != NULL)
		{
			arm = 0;
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
	return (retval == 1)? ((arm == 1) ? ((prc > 0) ? prc : 1) : ((cpu > 0) ? cpu : 1)) : 0;
}
//

char *rtrim(char *instr) 
{
	if (instr != NULL)
	{
		if (strlen(instr) > 0) 
		{
			while ((instr[strlen(instr)-1] == '\t') || (instr[strlen(instr)-1] == '\n') || (instr[strlen(instr)-1] == ' ')) 
			{
				instr[strlen(instr)-1] = '\0';
			}
		}
	}
	return instr; 
}

char *strreplace(char *instr, char chold, char chnew) 
{
	int i;
	
	for (i = 0; i < strlen(instr); i++)
	{
		if (instr[i] == chold)
		{
			instr[i] = chnew;
		}
	}
	return instr; 
}

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

char *getusername()
{
	struct passwd *pwd;
	char *loginname = getenv("USER");
	char *retval = NULL;

	if (loginname != NULL)
	{
		pwd = getpwnam(loginname);
		if (pwd != NULL)
		{
			#ifndef HAVE_NO_PASSWD_PW_GECOS
				// Some implentations don't feature the pw_gecos field.
				// If such and not yet fixed in lib usiong the above macro
				// just add it here.
				retval = (strlen(pwd->pw_gecos) > 0) ? pwd->pw_gecos : loginname;
				if (strchr(retval, ',') != NULL)
				{
					retval[strcspn(retval, ",")] = '\0';
				}
			#else
				retval = loginname;
			#endif
		}
		else
		{
			retval = loginname;
		}
	}
	return retval;
}

char *getloginname()
{
	return getenv("USER");
}

char *gettimestamp(char* buffer)
{
	struct timeval tv;
	struct tm* tm_info;
	char buf2[25];

	buffer = (char*) realloc(buffer, 25);
	gettimeofday(&tv,NULL);
	//time_t      tv.tv_sec  // seconds
	//suseconds_t tv.tv_usec // microseconds

	tm_info = localtime(&tv.tv_sec);

	strftime(buffer, 25, "%Y%m%d-%H:%M:%S", tm_info);
	sprintf(buf2, ".%03d", (int)tv.tv_usec / 1000);

	strcat(buffer, buf2); 
	//printf("%s\n", buffer);
	
	return(buffer);
}

