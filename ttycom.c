/*
 * ttycom.c
 * This module borrows code from INDI LIB (indicom) from 
 * Jasem Mutlaq (mutlaqja@ikarustech.com).
 *
 * Original code was released under the terms of the GNU Lesser 
 * General Public License as published by the Free Software 
 * Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 * 
 * This code was adapted on: 20.11.2013 by:
 *      Giampiero Spezzano (gspezzano@gmail.com)
 * to be part of "OpenSkyImager".
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
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/param.h>
#include <termios.h>
#include "ttycom.h"
#define PARITY_NONE    0
#define PARITY_EVEN    1
#define PARITY_ODD     2
#define MAXRBUF        2048

int tty_timeout(int fd, int timeout)
{
	if (fd == -1)
	{
		return TTY_ERRNO;
	}

	struct timeval tv;
	fd_set readout;
	int retval;

	FD_ZERO(&readout);
	FD_SET(fd, &readout);

	/* wait for 'timeout' seconds */
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	/* Wait till we have a change in the fd status */
	retval = select (fd+1, &readout, NULL, NULL, &tv);

	if (retval > 0)
	{
		/* Return 0 on successful fd change */
		return TTY_OK;
	}
	else if (retval == -1)
	{
		/* Return -1 due to an error */
		return TTY_SELECT_ERROR;
	}
	else 
	{
		/* Return -2 if time expires before anything interesting happens */
		return TTY_TIME_OUT;
	}
}

int tty_write(int fd, const char * buf, int nbytes, int *nbytes_written)
{
	if (fd == -1)
	{
		return TTY_ERRNO;
	}

	int bytes_w = 0;   
	*nbytes_written = 0;

	while (nbytes > 0)
	{
		bytes_w = write(fd, buf, nbytes);

		if (bytes_w < 0)
		{
			return TTY_WRITE_ERROR;
		}

		*nbytes_written += bytes_w;
		buf += bytes_w;
		nbytes -= bytes_w;
	}

	return TTY_OK;
}

int tty_write_string(int fd, const char * buf, int *nbytes_written)
{
	if (fd == -1)
	{
		return TTY_ERRNO;
	}

	unsigned int nbytes;
	int bytes_w = 0;
	*nbytes_written = 0;

	nbytes = strlen(buf);

	while (nbytes > 0)
	{

		bytes_w = write(fd, buf, nbytes);

		if (bytes_w < 0)
		{
			return TTY_WRITE_ERROR;
		}

		*nbytes_written += bytes_w;
		buf += bytes_w;
		nbytes -= bytes_w;
	}

	return TTY_OK;
}

int tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read)
{
	if (fd == -1)
	{
		return TTY_ERRNO;
	}

	int bytesRead = 0;
	int err = 0;
	*nbytes_read =0;

	if (nbytes <=0)
	{
		return TTY_PARAM_ERROR;
	}

	while (nbytes > 0)
	{
		if ( (err = tty_timeout(fd, timeout)) )
		{
			return err;
		}

		bytesRead = read(fd, buf, ((unsigned) nbytes));

		if (bytesRead < 0 )
		{
			return TTY_READ_ERROR;
		}

		buf += bytesRead;
		*nbytes_read += bytesRead;
		nbytes -= bytesRead;
	}

	return TTY_OK;
}

int tty_read_section(int fd, char *buf, char stop_char, int timeout, int *nbytes_read)
{
	if (fd == -1)
	{
		return TTY_ERRNO;
	}

	int bytesRead = 0;
	int err = TTY_OK;
	*nbytes_read = 0;

	for (;;)
	{
		if ( (err = tty_timeout(fd, timeout)) )
		{
			return err;
		}

		bytesRead = read(fd, buf, 1);

		if (bytesRead < 0 )
		{
			return TTY_READ_ERROR;
		}

		if (bytesRead)
		{
			(*nbytes_read)++;
		}

		if (*buf == stop_char)
		{
			return TTY_OK;
		}

		buf += bytesRead;
	}

	return TTY_TIME_OUT;
}

#if defined(BSD) && !defined(__GNU__)
	// BSD - OSX version
	int tty_connect(const char *device, int bit_rate, int word_size, int parity, int stop_bits, int *fd)
	{
		int	t_fd = -1;
		int bps;
		char msg[80];
		int	handshake;
		struct termios	tty_setting;

		// Open the serial port read/write, with no controlling terminal, and don't wait for a connection.
		// The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
		// See open(2) ("man 2 open") for details.

		t_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
		if (t_fd == -1)
		{
			printf("Error opening serial port %s - %s(%d).\n", device, strerror(errno), errno);
			goto error;
		}

		// Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
		// unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
		// processes.
		// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

		if (ioctl(t_fd, TIOCEXCL) == -1)
		{
		    printf("Error setting TIOCEXCL on %s - %s(%d).\n", device, strerror(errno), errno);
		    goto error;
		}

		// Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
		// See fcntl(2) ("man 2 fcntl") for details.

		if (fcntl(t_fd, F_SETFL, 0) == -1)
		{
		    printf("Error clearing O_NONBLOCK %s - %s(%d).\n", device, strerror(errno), errno);
		    goto error;
		}

		// Get the current options and save them so we can restore the default settings later.
		if (tcgetattr(t_fd, &tty_setting) == -1)
		{
		    printf("Error getting tty attributes %s - %s(%d).\n", device, strerror(errno), errno);
		    goto error;
		}

		// Set raw input (non-canonical) mode, with reads blocking until either a single character
		// has been received or a one second timeout expires.
		// See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") for details.

		cfmakeraw(&tty_setting);
		tty_setting.c_cc[VMIN] = 1;
		tty_setting.c_cc[VTIME] = 10;

		// The baud rate, word length, and handshake options can be set as follows:
		switch (bit_rate) 
		{
				case 0:
				        bps = B0;
				        break;
				case 50:
				        bps = B50;
				        break;
				case 75:
				        bps = B75;
				        break;
				case 110:
				        bps = B110;
				        break;
				case 134:
				        bps = B134;
				        break;
				case 150:
				        bps = B150;
				        break;
				case 200:
				        bps = B200;
				        break;
				case 300:
				        bps = B300;
				        break;
				case 600:
				        bps = B600;
				        break;
				case 1200:
				        bps = B1200;
				        break;
				case 1800:
				        bps = B1800;
				        break;
				case 2400:
				        bps = B2400;
				        break;
				case 4800:
				        bps = B4800;
				        break;
				case 9600:
				        bps = B9600;
				        break;
				case 19200:
				        bps = B19200;
				        break;
				case 38400:
				        bps = B38400;
				        break;
				case 57600:
				        bps = B57600;
				        break;
				case 115200:
				        bps = B115200;
				        break;
				case 230400:
				        bps = B230400;
				        break;
				default:
				        if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid bit rate.", bit_rate) < 0)
				                perror(NULL);
				        else
				                perror(msg);
				        return TTY_PARAM_ERROR;
		}

		 cfsetspeed(&tty_setting, bps);		// Set baud rate
		/* word size */
		switch (word_size) 
		{
				case 5:
					    tty_setting.c_cflag |= CS5;
					    break;
				case 6:
					    tty_setting.c_cflag |= CS6;
					    break;
				case 7:
					    tty_setting.c_cflag |= CS7;
					    break;
				case 8:
					    tty_setting.c_cflag |= CS8;
					    break;
				default:

					    fprintf( stderr, "Default\n") ;
					    if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid data bit count.", word_size) < 0)
					            perror(NULL);
					    else
					            perror(msg);

					    return TTY_PARAM_ERROR;
		}

	    /* parity */
	    switch (parity) {
	            case PARITY_NONE:
	                    break;
	            case PARITY_EVEN:
	                    tty_setting.c_cflag |= PARENB;
	                    break;
	            case PARITY_ODD:
	                    tty_setting.c_cflag |= PARENB | PARODD;
	                    break;
	            default:

	                    fprintf( stderr, "Default1\n") ;
	                    if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid parity selection value.", parity) < 0)
	                            perror(NULL);
	                    else
	                            perror(msg);

	                    return TTY_PARAM_ERROR;
	    }

	    /* stop_bits */
	    switch (stop_bits) {
	            case 1:
	                    break;
	            case 2:
	                    tty_setting.c_cflag |= CSTOPB;
	                    break;
	            default:
	                    fprintf( stderr, "Default2\n") ;
	                    if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid stop bit count.", stop_bits) < 0)
	                            perror(NULL);
	                    else
	                            perror(msg);

	                    return TTY_PARAM_ERROR;
	    }

		#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
			// Starting with Tiger, the IOSSIOSPEED ioctl can be used to set arbitrary baud rates
			// other than those specified by POSIX. The driver for the underlying serial hardware
			// ultimately determines which baud rates can be used. This ioctl sets both the input
			// and output speed.

			speed_t speed = 14400; // Set 14400 baud
			if (ioctl(fileDescriptor, IOSSIOSPEED, &speed) == -1)
			{
				printf("Error calling ioctl(..., IOSSIOSPEED, ...) %s - %s(%d).\n",	bsdPath, strerror(errno), errno);
			}
		#endif

		// Cause the new options to take effect immediately.
		if (tcsetattr(t_fd, TCSANOW, &tty_setting) == -1)
		{
		    printf("Error setting tty attributes %s - %s(%d).\n", device, strerror(errno), errno);
		    goto error;
		}

		// To set the modem handshake lines, use the following ioctls.
		// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

		if (ioctl(t_fd, TIOCSDTR) == -1) // Assert Data Terminal Ready (DTR)
		{
		    printf("Error asserting DTR %s - %s(%d).\n", device, strerror(errno), errno);
		}

		if (ioctl(t_fd, TIOCCDTR) == -1) // Clear Data Terminal Ready (DTR)
		{
		    printf("Error clearing DTR %s - %s(%d).\n", device, strerror(errno), errno);
		}

		handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
		if (ioctl(t_fd, TIOCMSET, &handshake) == -1)
		// Set the modem lines depending on the bits set in handshake
		{
		    printf("Error setting handshake lines %s - %s(%d).\n", device, strerror(errno), errno);
		}

		// To read the state of the modem lines, use the following ioctl.
		// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

		if (ioctl(t_fd, TIOCMGET, &handshake) == -1)
		// Store the state of the modem lines in handshake
		{
		    printf("Error getting handshake lines %s - %s(%d).\n", device, strerror(errno), errno);
		}

		printf("Handshake lines currently set to %d\n", handshake);

		#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
			unsigned long mics = 1UL;

			// Set the receive latency in microseconds. Serial drivers use this value to determine how often to
			// dequeue characters received by the hardware. Most applications don't need to set this value: if an
			// app reads lines of characters, the app can't do anything until the line termination character has been
			// received anyway. The most common applications which are sensitive to read latency are MIDI and IrDA
			// applications.

			if (ioctl(t_fd, IOSSDATALAT, &mics) == -1)
			{
					   // set latency to 1 microsecond
				printf("Error setting read latency %s - %s(%d).\n", device, strerror(errno), errno);
				goto error;
			}
		#endif

		*fd = t_fd;
		/* return success */
		return TTY_OK;

// Failure path
error:
		if (t_fd != -1)
		{
		    close(t_fd);
		    *fd = -1;
		}

		return TTY_PORT_FAILURE;
	}
#else
	// Unix - Linux version
	int tty_connect(const char *device, int bit_rate, int word_size, int parity, int stop_bits, int *fd)
	{
		int t_fd=-1;
		char msg[80];
		int bps;
		struct termios tty_setting;

		if ( (t_fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1)
		{
			 *fd = -1;
			return TTY_PORT_FAILURE;
		}

		/* Control Modes
		Set bps rate */
		switch (bit_rate) 
		{
			case 0:
				bps = B0;
				break;
			case 50:
				bps = B50;
				break;
			case 75:
				bps = B75;
				break;
			case 110:
				bps = B110;
				break;
			case 134:
				bps = B134;
				break;
			case 150:
				bps = B150;
				break;
			case 200:
				bps = B200;
				break;
			case 300:
				bps = B300;
				break;
			case 600:
				bps = B600;
				break;
			case 1200:
				bps = B1200;
				break;
			case 1800:
				bps = B1800;
				break;
			case 2400:
				bps = B2400;
				break;
			case 4800:
				bps = B4800;
				break;
			case 9600:
				bps = B9600;
				break;
			case 19200:
				bps = B19200;
				break;
			case 38400:
				bps = B38400;
				break;
			case 57600:
				bps = B57600;
				break;
			case 115200:
				bps = B115200;
				break;
			case 230400:
				bps = B230400;
				break;
			default:
				if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid bit rate.", bit_rate) < 0)
					perror(NULL);
				else
					perror(msg);
				return TTY_PARAM_ERROR;
		}
		if ((cfsetispeed(&tty_setting, bps) < 0) || (cfsetospeed(&tty_setting, bps) < 0))
		{
			perror("tty_connect: failed setting bit rate.");
			return TTY_PORT_FAILURE;
		}

		/* Control Modes
		set no flow control word size, parity and stop bits.
		Also don't hangup automatically and ignore modem status.
		Finally enable receiving characters. */
		tty_setting.c_cflag &= ~(CSIZE | CSTOPB | PARENB | PARODD | HUPCL | CRTSCTS);
		tty_setting.c_cflag |= (CLOCAL | CREAD);

		/* word size */
		switch (word_size) 
		{
			case 5:
				tty_setting.c_cflag |= CS5;
				break;
			case 6:
				tty_setting.c_cflag |= CS6;
				break;
			case 7:
				tty_setting.c_cflag |= CS7;
				break;
			case 8:
				tty_setting.c_cflag |= CS8;
				break;
			default:
				fprintf( stderr, "Default\n") ;
				if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid data bit count.", word_size) < 0)
					perror(NULL);
				else
					perror(msg);
				return TTY_PARAM_ERROR;
		}

		/* parity */
		switch (parity) 
		{
			case PARITY_NONE:
				break;
			case PARITY_EVEN:
				tty_setting.c_cflag |= PARENB;
				break;
			case PARITY_ODD:
				tty_setting.c_cflag |= PARENB | PARODD;
				break;
			default:
				fprintf( stderr, "Default1\n") ;
				if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid parity selection value.", parity) < 0)
					perror(NULL);
				else
					perror(msg);
				return TTY_PARAM_ERROR;
		}

		/* stop_bits */
		switch (stop_bits) 
		{
			case 1:
				break;
			case 2:
				tty_setting.c_cflag |= CSTOPB;
				break;
			default:
				fprintf( stderr, "Default2\n") ;
				if (snprintf(msg, sizeof(msg), "tty_connect: %d is not a valid stop bit count.", stop_bits) < 0)
					perror(NULL);
				else
					perror(msg);
				return TTY_PARAM_ERROR;
		}
		/* Control Modes complete */

		/* Ignore bytes with parity errors and make terminal raw and dumb.*/
		tty_setting.c_iflag &= ~(PARMRK | ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON | IXANY);
		tty_setting.c_iflag |= INPCK | IGNPAR | IGNBRK;

		/* Raw output.*/
		tty_setting.c_oflag &= ~(OPOST | ONLCR);

		/* Local Modes
		Don't echo characters. Don't generate signals.
		Don't process any characters.*/
		tty_setting.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN | NOFLSH | TOSTOP);
		tty_setting.c_lflag |=  NOFLSH;

		/* blocking read until 1 char arrives */
		tty_setting.c_cc[VMIN]  = 1;
		tty_setting.c_cc[VTIME] = 0;

		/* now clear input and output buffers and activate the new terminal settings */
		tcflush(t_fd, TCIOFLUSH);
		if (tcsetattr(t_fd, TCSANOW, &tty_setting)) 
		{
			perror("tty_connect: failed setting attributes on serial port.");
			tty_disconnect(t_fd);
			return TTY_PORT_FAILURE;
		}

		*fd = t_fd;
		/* return success */
		return TTY_OK;
	}
#endif


int tty_disconnect(int fd)
{
    if (fd == -1)
    {
           return TTY_ERRNO;
	}
	
	int err;
	tcflush(fd, TCIOFLUSH);
	err = close(fd);

	if (err != 0)
	{
		return TTY_ERRNO;
	}
	return TTY_OK;
}

void tty_error_msg(int err_code, char *err_msg, int err_msg_len)
{
	char error_string[512];

	switch (err_code)
	{
		case TTY_OK:
			strncpy(err_msg, "No Error\n", err_msg_len);
			break;

		case TTY_READ_ERROR:
			snprintf(error_string, 512, "Read Error: %s\n", strerror(errno));
			strncpy(err_msg, error_string, err_msg_len);
			break;

		   case TTY_WRITE_ERROR:
			snprintf(error_string, 512, "Write Error: %s\n", strerror(errno));
			strncpy(err_msg, error_string, err_msg_len);
			break;

		case TTY_SELECT_ERROR:
			snprintf(error_string, 512, "Select Error: %s\n", strerror(errno));
			strncpy(err_msg, error_string, err_msg_len);
			break;

		case TTY_TIME_OUT:
			strncpy(err_msg, "Timeout error\n", err_msg_len);
			break;

		case TTY_PORT_FAILURE:
			if (errno == EACCES)
				snprintf(error_string, 512, "Port failure Error: %s.\nTry adding your user to the dialout group and restart (sudo adduser $username dialout)\n", strerror(errno));
			else
				snprintf(error_string, 512, "Port failure Error: %s.\nCheck if device is connected to this port.\n", strerror(errno));

			strncpy(err_msg, error_string, err_msg_len);
			break;

		case TTY_PARAM_ERROR:
			strncpy(err_msg, "Parameter error\n", err_msg_len);
			break;

		case TTY_ERRNO:
			snprintf(error_string, 512, "%s\n", strerror(errno));
			strncpy(err_msg, error_string, err_msg_len);
			break;

		default:
			strncpy(err_msg, "Error: unrecognized error code\n", err_msg_len);
			break;


	}	
}

