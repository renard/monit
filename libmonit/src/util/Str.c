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
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>
#include <limits.h>


#include "NumberFormatException.h"
#include "system/System.h"
#include "Str.h"


/**
 * Implementation of the Str interface
 *
 * @author Tildeslash Ltd. - www.tildeslash.com
 * @see www.mmonit.com
 * @file
 */


/* ----------------------------------------------------------- Definitions */


static const uchar_t nonalnum[256]= {
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};


/* -------------------------------------------------------- Public Methods */


char *Str_chomp(char *s) {
        if (s) {
                char *p = s;
                for (; *p; p++)
                        if (*p == '\r' || *p == '\n') {
                                *p = 0; break;
                        }
        }
        return s;
}


char *Str_trim(char *s) {
        return (Str_ltrim(Str_rtrim(s)));
}


char *Str_ltrim(char *s) {
        if (s && isspace(*s)) {
                int i, j;
                for (j = 0; s[j]; j++) ;
                for (i = 0; isspace(s[i]); i++) ;
                memmove(s, s + i, j - i);
                s[j-i] = 0;
        }
        return s;
}


char *Str_rtrim(char *s) {
        if (s && *s) {
                int j;
                for (j = 0; s[j]; j++) ;
                for (j = j - 1; isspace(s[j]); j--) s[j] = 0;
        }
        return s;
}


char *Str_unquote(char *s) {
        if (s && *s) {
                char *t1, *t2;
                t1 = t2 = s;
                // Left trim
                while (*t1==34 || *t1==39) t1++;
                if (t1 != s) {
                        do
                                *t2++ = *t1;
                        while (*t1++);
                }
                // Right trim
                t1 = s;
                while (*t1) t1++;
                do 
                        *(t1--) = 0;
                while (t1 >= s && (*t1==34 || *t1==39));
        }
        return s;
}


char *Str_toLower(char *s) {
        if (s)
                for (int i = 0; s[i]; i++)
                        s[i] = tolower(s[i]);
        return s;
}


char *Str_toUpper(char *s) {
        if (s)
                for (int i = 0; s[i]; i++)
                        s[i] = toupper(s[i]);
        return s;
}


char *Str_ntos(long n, char s[43]) {
        assert(s);
        s[42] = 0;
        char *t = s + 42;
        unsigned long m;
        if (n == LONG_MIN)
                m = LONG_MAX + 1UL;
        else if (n < 0)
                m = -n;
        else
                m = n;
        do
                *--t = m % 10 + '0';
        while ((m /= 10) > 0);
        if (n < 0)
                *--t = '-';
        return t;
}


char *Str_toalnum(char *s) {
        if (s && *s) {
                register int x,y;
                uchar_t *p = (uchar_t*)s;
                for (x = 0; p[x]; x++)
                        while (nonalnum[p[x]]) {
                                for (y = x; p[y + 1]; y++)
                                        p[y] = p[y + 1];
                                p[y] = 0;
                        }
        }
        return s;
}


int Str_isalnum(const char *s) {
        if (s && *s) {
                int i;
                uchar_t *p = (uchar_t*)s;
                for (i = 0; p[i]; i++)
                        if (nonalnum[p[i]])
                                return false;
                return true;
        }
        return false;
}


int Str_parseInt(const char *s) {
        int i;
        char *e;
        if (! (s && *s))
                THROW(NumberFormatException, "For input string null");
        errno = 0;
        i = (int)strtol(s, &e, 10);
        if (errno || (e == s))
                THROW(NumberFormatException, "For input string %s -- %s", s, System_getError(errno));
        return i;
}


long long int Str_parseLLong(const char *s) {
        char *e;
        long long l;
        if (! (s && *s))
                THROW(NumberFormatException, "For input string null");
        errno = 0;
        l = strtoll(s, &e, 10);
        if (errno || (e == s))
                THROW(NumberFormatException, "For input string %s -- %s", s, System_getError(errno));
        return l;
}


double Str_parseDouble(const char *s) {
        char *e;
        double d;
        if (! (s && *s))
                THROW(NumberFormatException, "For input string null");
        errno = 0;
        d = strtod(s, &e);
        if (errno || (e == s))
                THROW(NumberFormatException, "For input string %s -- %s", s, System_getError(errno));
        return d;
}


char *Str_replaceChar(char *s, char old, char new) {
        if (s) {
                for (char *t = s; *t; t++) 
                        if (*t == old) 
                                *t = new;
        }
        return s;
}


int Str_startsWith(const char *a, const char *b) {
        if (a && b) {
                const char *s = a;
                while (*a && *b)
                        if (*a++ != *b++) return false;
                return ((*a == *b) || (a != s && *b == 0));
        }
        return false;
}


int Str_endsWith(const char *a, const char *b) {
        if (a && b) {
                register int i = 0, j = 0;
                while (a[i]) i++;
                while (b[j]) j++;
                for(; (i && j); i--, j--)
                        if(a[i] != b[j]) return false;
                return (i >= j);
        }
        return false;
}


char *Str_sub(const char *a, const char *b) {
        if (a && b && *b) {
                const char *ap, *bp;
                while (*a) {
                        if (toupper(*a) == toupper(*b)) {
                                ap = a;
                                bp = b;
                                do
                                        if (! *bp)
                                                return (char*)a;
                                while (toupper(*ap++) == toupper(*bp++));
                        }
                        a++;
                }
        }
        return NULL;
}


int Str_has(const char *charset, const char *s) {
        if (charset && s) {
                register int x, y;
                for (x = 0; s[x]; x++) {
                        for (y = 0; charset[y]; y++) {
                                if (s[x] == charset[y])
                                        return true; 
                        }
                }
        }
        return false;
}


int Str_isEqual(const char *a, const char *b) {
        if (a && b) { 
                while (*a && *b)
                        if (toupper(*a++) != toupper(*b++)) return false;
                return (*a == *b);
        }
        return false;
}


int Str_isByteEqual(const char *a, const char *b) {
        if (a && b) {
                while (*a && *b)
                        if (*a++ != *b++) return false;
                return (*a == *b);
        }
        return false;
}


char *Str_copy(char *dest, const char *src, int n) {
        char *p = dest;
        if (!(src && dest)) { 
                if (dest) 
                        *dest = 0; 
                return dest; 
        }
        for (; (*src && n--); src++, p++)
                *p = *src;
        *p = 0;
        return dest;
}


// We don't use strdup so we can report MemoryException on OOM
char *Str_dup(const char *s) { 
        char *t = NULL;
        if (s) {
                size_t n = strlen(s); 
                t = ALLOC(n + 1);
                memcpy(t, s, n);
                t[n] = 0;
        }
        return t;
}


char *Str_ndup(const char *s, long n) {
        char *t = NULL;
        assert(n >= 0);
        if (s) {
                size_t l = strlen(s); 
                n = l < n ? l : n; // Use the actual length of s if shorter than n
                t = ALLOC(n + 1);
                memcpy(t, s, n);
                t[n] = 0;
        }
        return t;
}


char *_Str_join(char *dest, int n, ...) {
        char *p, *q;
        va_list ap;
        assert(dest);
        va_start(ap, n);
        for (q = dest, p = va_arg(ap, char *); (p && (n > 0)); p = va_arg(ap, char *))
                while (*p && n--) *q++ = *p++;
        va_end(ap);
        *q = 0;
        return dest;
}


char *Str_cat(const char *s, ...) {
        char *t = NULL;
        if (s) {
                va_list ap;
                va_start(ap, s);
                t = Str_vcat(s, ap);
                va_end(ap);
        }
        return t;
}


char *Str_vcat(const char *s, va_list ap) {
        char *buf = NULL;
        if (s) {
                int n = 0;
                int size = STRLEN;
                buf = ALLOC(size);
                while (true) {
                        va_list ap_copy;
                        va_copy(ap_copy, ap);
                        n = vsnprintf(buf, size, s, ap_copy);
                        va_end(ap_copy);
                        if (n > -1 && n < size)
                                break;
                        if (n > -1)
                                size = n+1;
                        else
                                size *= 2;
                        RESIZE(buf, size);
                }
        }
        return buf;
}


char *Str_trunc(char *s, int n) {
        assert(n>=0);
        if (s) {
                size_t sl = strlen(s);
                if (sl > (n + 4)) {
                        int e = n+3;
                        for (; n < e; n++)
                                s[n] = '.';
                        s[n] = 0;
                }
        }
        return s;
}


char *Str_curtail(char *s, char *t) {
        if (s) {
                char *x = Str_sub(s, t);
                if (x) *x = 0;
        }
        return s;
}


int Str_lim(const char *s, int limit) {
        assert(limit>=0);
        if (s)
                for (; *s; s++) limit--;
        return (limit <= 0);
}


int Str_match(const char *pattern, const char *subject) {
        assert(pattern);
        if (subject && *subject) {
                regex_t regex = {0};
                int error = regcomp(&regex, pattern, REG_NOSUB|REG_EXTENDED);
                if (error) {
                        char e[STRLEN];
                        regerror(error, &regex, e, STRLEN);
                        regfree(&regex);
                        THROW(AssertException, "regular expression error -- %s", e);
                } else {
                        error = regexec(&regex, subject, 0, NULL, 0);
                        regfree(&regex);
                        return (error == 0);
                }
        }
        return false;
}


unsigned int Str_hash(const void *x) {
        const char *s = x;
        unsigned long h = 0, g;
        assert(x);
        while (*s) {
                h = (h << 4) + *s++;
                if ((g = h & 0xF0000000))
                        h ^= g >> 24;
                h &= ~g;
        }
        return (int)h;
}


int Str_cmp(const void *x, const void *y) {
        return strcmp((const char *)x, (const char *)y);
}

