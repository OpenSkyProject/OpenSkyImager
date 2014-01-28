#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include "imgFifoio.h"
#define BUFFER_SIZE PIPE_BUF

GIOChannel *fch;

int CreateFifo(char *fifoname) 
{
	int retcode = 1;

	if (access(fifoname, F_OK) == -1) 
	{   
		/* check if fifo already exists*/
		/* if not then, create the fifo*/
		if (mkfifo(fifoname, S_IRWXU|S_IRWXG|S_IRWXO) == 0) 
		{
			chmod(fifoname, S_IRWXU|S_IRWXG|S_IRWXO);
		}
		else
		{
			printf( "Fifo: Could not create %s\n", fifoname);
			retcode = 0;
		}
	}        
	else
	{
		chmod(fifoname, S_IRWXU|S_IRWXG|S_IRWXO);
	}

	//else
	//{
	//	printf( "Fifo: %s, already exists!\n", FIFO_IN);
	//	retcode = 0;
	//}	
	return retcode;
}

int DeleteFifo(char *fifoname) 
{
	int retcode = 1;

	// Wipe fifos
	if (access(fifoname, F_OK) > -1) 
	{   
		if (unlink(fifoname) == -1) 
		{
			printf( "Fifo: Could not delete %s\n", fifoname);
			retcode = 0;
		}
	}
	return retcode;
}

int OpenFifo(char *fifoname)
{  
	int fd_fifo;
	
	printf("Fifo: Opening %s\n", fifoname);
	fd_fifo = open(fifoname, O_RDWR | O_NONBLOCK);
	return fd_fifo;
}

int CloseFifo(int fd_fifo)
{
	printf("Fifo: Closing %d\n", fd_fifo);
	return close(fd_fifo);
}

int LinkFifo(int fd_fifo, GIOChannel *gch, GIOFunc func, gpointer user_data)
{
	int retval  = 0;
	
	if (fd_fifo > -1)
	{
		// Valid file
		if(gch)
		{
			// Channel created
			retval = g_io_add_watch(gch, G_IO_IN, func, user_data);
			if(!retval)
			{
				printf("Fifo: Error adding watch\n");
			}
		}
		else
		{
			printf("Fifo: Error creating GIOChannel\n");
		}
	}
	return retval; 
}

int UnLinkFifo(int tag, GIOChannel *gch)
{
	int retval = 0;
	
	if (g_source_remove(tag))
	{
		// Watch removed
		if(g_io_channel_shutdown(gch, FALSE, NULL) == G_IO_STATUS_NORMAL)
		{
			// Io channel shutdown, unref variable
			g_io_channel_unref(gch);
			retval = 1;
		}
		else
		{
			printf("Fifo: Error shutdown channel\n");
		}
	}
	else
	{
		printf("Fifo: Error removing watch\n");
	}
	return (retval);
}

