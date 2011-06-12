/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.  
 */


#include "Config.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#ifdef OPENBSD
#include <sys/uio.h>
#endif

#include "Str.h"
#include "system/Net.h"
#include "OutputStream.h"


/**
 * Implementation of the OutputStream interface. 
 *
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */


/* ----------------------------------------------------------- Definitions */


// Aprox two TCP frames
#define BUFFER_SIZE 3000

#define T OutputStream_T
struct T {
        int fd;
        int offset;
        int length;
        time_t timeout;
        boolean_t isClosed;
        long long int bytesWritten;
        uchar_t buffer[BUFFER_SIZE];
};


/* --------------------------------------------------------------- Private */


/* Write the output buffer */
static int doWrite(T S, time_t timeout) {
        if (S->isClosed)
                return -1;
        errno = 0;
        int n = (int)Net_write(S->fd, S->buffer + S->offset, BUFFER_SIZE - S->length, timeout);
        if (n > 0) // TODO do a memmove to put remaining at the start of the buffer
                S->offset += n;
        else if ((n < 0) || (! (errno == EAGAIN || errno == EWOULDBLOCK))) { // write error or peer closed connection
                n = -1;
                S->isClosed = true;
                S->offset = S->length = 0;
        }
        return n;
}


/* ---------------------------------------------------------------- Public */


T OutputStream_new(int descriptor) {
        T S;
        NEW(S);
        S->fd = descriptor;
        S->timeout = NET_WRITE_TIMEOUT;
        // TODO offset / length
        return S;
}


void OutputStream_free(T *S) {
        assert(S && *S);
        FREE(*S);
}


/* ------------------------------------------------------------ Properties */


int OutputStream_getDescriptor(T S) {
        assert(S);
        return S->fd;
}


int OutputStream_buffered(T S) {
        assert(S);
        return S->offset;
}


void OutputStream_setTimeout(T S, time_t timeout) {
        assert(S);
        assert(timeout >= 0);
        S->timeout = timeout;
}


time_t OutputStream_getTimeout(T S) {
        assert(S);
        return S->timeout;
}


int OutputStream_isClosed(T S) {
        assert(S);
        return S->isClosed;
}


void OutputStream_print(T S, const char *s, ...) {
        assert(S);
        if (! S->isClosed && s && *s) {
                va_list ap;
                va_start(ap, s);
                // TODO
                va_end(ap);
        }
}


void OutputStream_vprint(T S, const char *s, va_list ap) {
        assert(S);
        if (! S->isClosed && s && *s) {
                va_list ap_copy;
                va_copy(ap_copy, ap);
                // TODO
                va_end(ap_copy);
        }
}


void OutputStream_write(T S, const void *b, int size) {
        assert(S);
        if (! S->isClosed && b) {
                ; // TODO
        }
}


int OutputStream_flush(T S) {
        assert(S);
        if (S->isClosed)
                return -1;
        if (S->offset) // TODO
                return doWrite(S, S->timeout);
        return 0;
}


void OutputStream_clear(T S) {
        assert(S);
        S->offset = S->length = 0;
}

