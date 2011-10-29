/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CROSS_COMPILE

#define apr_isalnum(c) (isalnum(((unsigned char)(c))))
#define apr_isalpha(c) (isalpha(((unsigned char)(c))))
#define apr_iscntrl(c) (iscntrl(((unsigned char)(c))))
#define apr_isprint(c) (isprint(((unsigned char)(c))))
#include <ctype.h>
#define APR_HAVE_STDIO_H 1
#define APR_HAVE_STRING_H 1

#else

#include "apr.h"
#include "apr_lib.h"

#if defined(WIN32) || defined(OS2)
#define NEED_ENHANCED_ESCAPES
#endif

#endif

#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

/* A bunch of functions in util.c scan strings looking for certain characters.
 * To make that more efficient we encode a lookup table.
 */
#define T_ESCAPE_SHELL_CMD    (0x01)
#define T_ESCAPE_PATH_SEGMENT (0x02)
#define T_OS_ESCAPE_PATH      (0x04)
#define T_HTTP_TOKEN_STOP     (0x08)
#define T_ESCAPE_LOGITEM      (0x10)
#define T_ESCAPE_FORENSIC     (0x20)
#define T_ESCAPE_URLENCODED   (0x40)

int main(int argc, char *argv[])
{
    unsigned c;
    unsigned char flags;

    printf("/* this file is automatically generated by gen_test_char, "
           "do not edit */\n"
           "#define T_ESCAPE_SHELL_CMD     (%u)\n"
           "#define T_ESCAPE_PATH_SEGMENT  (%u)\n"
           "#define T_OS_ESCAPE_PATH       (%u)\n"
           "#define T_HTTP_TOKEN_STOP      (%u)\n"
           "#define T_ESCAPE_LOGITEM       (%u)\n"
           "#define T_ESCAPE_FORENSIC      (%u)\n"
           "#define T_ESCAPE_URLENCODED    (%u)\n"
           "\n"
           "static const unsigned char test_char_table[256] = {",
           T_ESCAPE_SHELL_CMD,
           T_ESCAPE_PATH_SEGMENT,
           T_OS_ESCAPE_PATH,
           T_HTTP_TOKEN_STOP,
           T_ESCAPE_LOGITEM,
           T_ESCAPE_FORENSIC,
           T_ESCAPE_URLENCODED);

    for (c = 0; c < 256; ++c) {
        flags = 0;
        if (c % 20 == 0)
            printf("\n    ");

        /* escape_shell_cmd */
#ifdef NEED_ENHANCED_ESCAPES
        /* Win32/OS2 have many of the same vulnerable characters
         * as Unix sh, plus the carriage return and percent char.
         * The proper escaping of these characters varies from unix
         * since Win32/OS2 use carets or doubled-double quotes,
         * and neither lf nor cr can be escaped.  We escape unix
         * specific as well, to assure that cross-compiled unix
         * applications behave similiarly when invoked on win32/os2.
         *
         * Rem please keep in-sync with apr's list in win32/filesys.c
         */
        if (c && strchr("&;`'\"|*?~<>^()[]{}$\\\n\r%", c)) {
            flags |= T_ESCAPE_SHELL_CMD;
        }
#else
        if (c && strchr("&;`'\"|*?~<>^()[]{}$\\\n", c)) {
            flags |= T_ESCAPE_SHELL_CMD;
        }
#endif

        if (!apr_isalnum(c) && !strchr("$-_.+!*'(),:@&=~", c)) {
            flags |= T_ESCAPE_PATH_SEGMENT;
        }

        if (!apr_isalnum(c) && !strchr("$-_.+!*'(),:@&=/~", c)) {
            flags |= T_OS_ESCAPE_PATH;
        }

        if (!apr_isalnum(c) && !strchr(".-*_ ", c)) {
            flags |= T_ESCAPE_URLENCODED;
        }

        /* these are the "tspecials" (RFC2068) or "separators" (RFC2616) */
        if (c && (apr_iscntrl(c) || strchr(" \t()<>@,;:\\\"/[]?={}", c))) {
            flags |= T_HTTP_TOKEN_STOP;
        }

        /* For logging, escape all control characters,
         * double quotes (because they delimit the request in the log file)
         * backslashes (because we use backslash for escaping)
         * and 8-bit chars with the high bit set
         */
        if (c && (!apr_isprint(c) || c == '"' || c == '\\' || apr_iscntrl(c))) {
            flags |= T_ESCAPE_LOGITEM;
        }

        /* For forensic logging, escape all control characters, top bit set,
         * :, | (used as delimiters) and % (used for escaping).
         */
        if (!apr_isprint(c) || c == ':' || c == '|' || c == '%'
            || apr_iscntrl(c) || !c) {
            flags |= T_ESCAPE_FORENSIC;
        }

        printf("%u%c", flags, (c < 255) ? ',' : ' ');
    }

    printf("\n};\n");

    return 0;
}
