/**************************************************************************/
/*  stream_peer_stdio.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "stream_peer_stdio.h"

#include <cstdio>
#include <fcntl.h>

#if defined(WINDOWS_ENABLED)
#include <io.h>
#include <windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

StreamPeerSTDIO::StreamPeerSTDIO() {
#if defined(WINDOWS_ENABLED)
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#else
	// Set stdin to non-blocking mode
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
#endif
}

Error StreamPeerSTDIO::put_data(const uint8_t *p_data, int p_bytes) {
	if (fwrite(p_data, 1, p_bytes, stdout) != (size_t)p_bytes) {
		return ERR_FILE_CANT_WRITE;
	}
	fflush(stdout);
	return OK;
}

Error StreamPeerSTDIO::put_partial_data(const uint8_t *p_data, int p_bytes, int &r_sent) {
	r_sent = fwrite(p_data, 1, p_bytes, stdout);
	fflush(stdout);
	return OK;
}

Error StreamPeerSTDIO::get_data(uint8_t *p_buffer, int p_bytes) {
	if (fread(p_buffer, 1, p_bytes, stdin) != (size_t)p_bytes) {
		return ERR_FILE_CANT_READ;
	}
	return OK;
}

Error StreamPeerSTDIO::get_partial_data(uint8_t *p_buffer, int p_bytes, int &r_received) {
	r_received = 0;

#if defined(WINDOWS_ENABLED)
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD bytesAvailable = 0;
	if (!PeekNamedPipe(hStdin, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
		return FAILED;
	}
	if (bytesAvailable == 0) {
		return OK;
	}

	DWORD bytesToRead = (DWORD)p_bytes;
	if (bytesToRead > bytesAvailable) {
		bytesToRead = bytesAvailable;
	}

	DWORD bytesRead = 0;
	if (ReadFile(hStdin, p_buffer, bytesToRead, &bytesRead, nullptr)) {
		r_received = bytesRead;
		return OK;
	}
	return FAILED;
#else
	ssize_t read_bytes = read(STDIN_FILENO, p_buffer, p_bytes);
	if (read_bytes < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return OK; // No data available, not an error
		}
		return FAILED;
	}
	r_received = read_bytes;
	return OK;
#endif
}

int StreamPeerSTDIO::get_available_bytes() const {
#if defined(WINDOWS_ENABLED)
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD bytesAvailable = 0;
	if (PeekNamedPipe(hStdin, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
		return (int)bytesAvailable;
	}
#endif
	return 0;
}
