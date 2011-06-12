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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "StringBuffer.h"


/**
 * Implementation of the StringBuffer interface. 
 *
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */


/* ------------------------------------------------------------ Definitions */


#define T StringBuffer_T
struct T {
        int used;
        size_t length;
	char *buffer;
};


/* ---------------------------------------------------------------- Private */


static inline void doAppend(T S, const char *s, va_list ap) {
        va_list ap_copy;
        while (true) {
                va_copy(ap_copy, ap);
                int n = vsnprintf(S->buffer + S->used, S->length - S->used, s, ap_copy);
                va_end(ap_copy);
                if (n > -1 && (S->used + n) < S->length) {
                        S->used += n;
                        break;
                }
                if (n > -1)
                        S->length += STRLEN + n;
                else
                        S->length *= 2;
                RESIZE(S->buffer, S->length + 1);
        }
}


static inline T ctor(int hint) {
        T S;
        NEW(S);
        S->used = 0;
        S->length = hint;
        S->buffer = ALLOC(hint);
        *S->buffer = 0;
        return S;
}


/* ----------------------------------------------------------------- Public */


T StringBuffer_new(const char *s) {
        return StringBuffer_append(ctor(STRLEN), "%s", s);
}


T StringBuffer_create(int hint) {
        if (hint <= 0)
                THROW(AssertException, "Illegal hint value");
        return ctor(hint);
}


void StringBuffer_free(T *S) {
        assert(S && *S);
	FREE((*S)->buffer);
        FREE(*S);
}


T StringBuffer_append(T S, const char *s, ...) {
        assert(S);
        if (s && *s) {
                va_list ap;
                va_start(ap, s);
                doAppend(S, s, ap);
                va_end(ap);
        }
        return S;
}


T StringBuffer_vappend(T S, const char *s, va_list ap) {
        assert(S);
        if (s && *s) {
                va_list ap_copy;
                va_copy(ap_copy, ap);
                doAppend(S, s, ap_copy);
                va_end(ap_copy);
        }
        return S;
}


int StringBuffer_replace(T S, const char *a, const char *b) {
        int n = 0;
        assert(S);
        if (a && b && *a) {
                register int i, j;
                for (i = 0; S->buffer[i]; i++) {
                        if (S->buffer[i] == *a) {
                                j = 0;
                                do
                                        if (! a[++j]) {n++; break;}
                                while (S->buffer[i + j] == a[j]);
                        }
                }
                if (n) {
                        int m = n;
                        size_t bl = strlen(b);
                        size_t diff = bl - strlen(a);
                        if (diff > 0) {
                                size_t required = (diff * n) + S->used + 1;
                                if (required >= S->length) {
                                        S->length = required;
                                        RESIZE(S->buffer, S->length);
                                }
                        }
                        for (i = 0; m; i++) {
                                if (S->buffer[i] == *a) {
                                        j = 0;
                                        do
                                                if (! a[++j]) {
                                                        memmove(S->buffer + i + bl, S->buffer + i + j, (S->used - (i + j)));
                                                        memmove(S->buffer + i, b, bl); 
                                                        S->used += diff;
                                                        i += diff;
                                                        m--;
                                                        break;
                                                }
                                        while (S->buffer[i + j] == a[j]);
                                }
                        }
                        S->buffer[S->used] = 0;
                }
        }
        return n;
}


void StringBuffer_delete(T S, int index) {
        assert(S);
        if (index < 0 || index > S->used)
                THROW(AssertException, "Index out of bounds");
        S->used = index;
        S->buffer[S->used] = 0;
}


int StringBuffer_indexOf(T S, const char *s) {
        assert(S);
        if (s && *s) {
                register int i, j;
                for (i = 0; i <= S->used; i++) {
                        if (S->buffer[i] == *s) {
                                j = 0;
                                do
                                        if (! s[++j])
                                                return i;
                                while (S->buffer[i + j] == s[j]);
                        }
                }
        }
        return -1;
}


int StringBuffer_lastIndexOf(T S, const char *s) {
        assert(S);
        if (s && *s) {
                register int i, j;
                for (i = S->used; i >= 0; i--) {
                        if (S->buffer[i] == *s) {
                                j = 0;
                                do
                                        if (! s[++j])
                                                return i;
                                while (S->buffer[i + j] == s[j]);
                        }
                }
        }
        return -1;
}


const char *StringBuffer_substring(T S, int index) {
        assert(S);
        if (index < 0 || index > S->used)
                THROW(AssertException, "Index out of bounds");
        return S->buffer + index;
}


int StringBuffer_length(T S) {
        assert(S);
        return S->used;
}


void StringBuffer_clear(T S) {
        assert(S);
        S->used = 0;
        *S->buffer = 0;
}


T StringBuffer_put(T S, const char *s, int length) {
	assert(S);
        if (s && (length > 0)) {
                if ((S->used + length) >= S->length) {
                        S->length += length + 1;
                        RESIZE(S->buffer, S->length);
                }
                memcpy(S->buffer + S->used, s, length);
                S->used += length;
                S->buffer[S->used]= 0;
        }
        return S;
}


const char *StringBuffer_toString(T S) {
        assert(S);
        return S->buffer;
}

