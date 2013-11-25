/*
 * ttycom.h
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


#ifndef TTYCOM_H
	#define TTYCOM_H

	#include <time.h>

	#define TTY_ERRMSG_SIZE 1024

	/* TTY Error Codes */
	enum TTY_ERROR { TTY_OK=0, TTY_READ_ERROR=-1, TTY_WRITE_ERROR=-2, TTY_SELECT_ERROR=-3, TTY_TIME_OUT=-4, TTY_PORT_FAILURE=-5, TTY_PARAM_ERROR=-6, TTY_ERRNO = -7};

	/**
	 * \defgroup ttyFunctions TTY Functions: Functions to perform common terminal access routines.
	*/

	/*@{*/

	/** \brief read buffer from terminal
		\param fd file descriptor
		\param buf pointer to store data. Must be initilized and big enough to hold data.
		\param nbytes number of bytes to read.
		\param timeout number of seconds to wait for terminal before a timeout error is issued.
		\param nbytes_read the number of bytes read.
		\return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
	*/
	int tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read);

	/** \brief read buffer from terminal with a delimiter
		\param fd file descriptor
		\param buf pointer to store data. Must be initilized and big enough to hold data.
		\param stop_char if the function encounters \e stop_char then it stops reading and returns the buffer.
		\param timeout number of seconds to wait for terminal before a timeout error is issued.
		\param nbytes_read the number of bytes read.
		\return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
	*/

	int tty_read_section(int fd, char *buf, char stop_char, int timeout, int *nbytes_read);


	/** \brief Writes a buffer to fd.
		\param fd file descriptor
		\param buffer a null-terminated buffer to write to fd.
		\param nbytes number of bytes to write from \e buffer
		\param nbytes_written the number of bytes written
		\return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
	*/
	int tty_write(int fd, const char * buffer, int nbytes, int *nbytes_written);

	/** \brief Writes a null terminated string to fd.
		\param fd file descriptor
		\param buffer the buffer to write to fd.
		\param nbytes_written the number of bytes written
		\return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
	*/
	int tty_write_string(int fd, const char * buffer, int *nbytes_written);


	/** \brief Establishes a tty connection to a terminal device.
		\param device the device node. e.g. /dev/ttyS0
		\param bit_rate bit rate
		\param word_size number of data bits, 7 or 8, USE 8 DATA BITS with modbus
		\param parity 0=no parity, 1=parity EVEN, 2=parity ODD
		\param stop_bits number of stop bits : 1 or 2
		\param fd \e fd is set to the file descriptor value on success.
		\return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
		\author Wildi Markus
	*/

	int tty_connect(const char *device, int bit_rate, int word_size, int parity, int stop_bits, int *fd);

	/** \brief Closes a tty connection and flushes the bus.
		\param fd the file descriptor to close.
		\return On success, it returns TTY_OK, otherwise, a TTY_ERROR code.
	*/
	int tty_disconnect(int fd);

	/** \brief Retrieve the tty error message
		\param err_code the error code return by any TTY function.
		\param err_msg an initialized buffer to hold the error message.
		\param err_msg_len length in bytes of \e err_msg
	*/
	void tty_error_msg(int err_code, char *err_msg, int err_msg_len);

	int tty_timeout(int fd, int timeout);

#endif
