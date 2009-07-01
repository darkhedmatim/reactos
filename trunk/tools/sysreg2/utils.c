/*
 * PROJECT:     ReactOS System Regression Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various auxiliary functions
 * COPYRIGHT:   Copyright 2008-2009 Christoph von Wittich <christoph_vw@reactos.org>
 *              Copyright 2009 Colin Finck <colin@reactos.org>
 */

#include "sysreg.h"

ssize_t safewrite(int fd, const void *buf, size_t count)
{
    size_t nwritten = 0;
    while (count > 0) {
        ssize_t r = write(fd, buf, count);

        if (r < 0 && errno == EINTR)
            continue;
        if (r < 0)
            return r;
        if (r == 0)
            return nwritten;
        buf = (const char *)buf + r;
        count -= r;
        nwritten += r;
    }
    return nwritten;
}

char * ReadFile (const char *filename)
{
    char *buffer = NULL, *oldbuffer;
    int len = 0, fd, r;
    char b[1024];

    fd = open (filename, O_RDONLY);
    if (fd == -1) 
        return NULL;

    for (;;) {
        r = read (fd, b, sizeof b);
        if (r == -1) 
        {
          if (buffer) free (buffer);
          close(fd);
          return NULL;
        }
        if (r == 0) break;  /* end of file. */
        oldbuffer = buffer;
        buffer = realloc (buffer, len+r);
        if (buffer == NULL) {
            /* out of memory */
            close(fd);
            return NULL;
        }
        memcpy (buffer+len, b, r);
        len += r;
    }

    buffer = realloc (buffer, len+1);
    if (buffer == NULL) 
    {
        /* out of memory */
        close(fd);
        return NULL;
    }
    buffer[len] = '\0';
    close (fd);
    return buffer;
}

void SysregPrintf(const char* format, ...)
{
    va_list args;

    printf("[SYSREG] ");

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
