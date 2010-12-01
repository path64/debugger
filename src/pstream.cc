/*
 * Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
 * Unpublished -- rights reserved under the copyright laws of the United
 * States. USE OF A COPYRIGHT NOTICE DOES NOT IMPLY PUBLICATION OR
 * DISCLOSURE. THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE
 * SECRETS OF PATHSCALE, INC. USE, DISCLOSURE, OR REPRODUCTION IS
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF PATHSCALE,
 * INC.
 * 
 * U.S. Government Restricted Rights: The Software is a "commercial item," 
 * as that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting of
 * "commercial computer software" and "commercial computer software
 * documentation," as such terms are used in 48 C.F.R. 12.212 (SEPT 1995). 
 * Consistent with 48 C.F.R. 12.212 and 48 C.F.R. 227-7202-1 through
 * 227-7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * Software with only those rights set forth in the accompanying license
 * agreement. PathScale, Inc. 477 N. Mathilda Ave; Sunnyvale, CA 94085.
 * 
 * file: pstream.cc
 * created on: Fri Aug 13 11:07:37 PDT 2004
 * author: David Allison <dallison@pathscale.com>
 * 
 */

#include "pstream.h"
#include "readline.h"
#include "dbg_except.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define DEFAULT_WIDTH    72
#define DEFAULT_HEIGHT   30
#define DEFAULT_PAGING   false
#define DEFAULT_BUFSIZE  8196


PStream::PStream(int _fd)
 : width(DEFAULT_WIDTH),
   height(DEFAULT_HEIGHT),
   paging(DEFAULT_PAGING),
   buf_len(DEFAULT_BUFSIZE)
{
   ch = buf_data = (char*)malloc(DEFAULT_BUFSIZE);
   if (!buf_data) throw Exception("out of memory");

   fd = _fd;
   depth = 0;  
 
   reset();
}

PStream::PStream(const char* fname)
 : width(DEFAULT_WIDTH),
   height(DEFAULT_HEIGHT),
   paging(DEFAULT_PAGING),
   buf_len(DEFAULT_BUFSIZE)
{
   ch = buf_data = (char*)malloc(DEFAULT_BUFSIZE);
   if (!buf_data) throw Exception("out of memory");

#if defined (__linux__)
   fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
#else
   fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC);
#endif
   if (fd == -1) {
      throw Exception("Unable to open file for output");
   }

   depth = 0;  
   reset();
}

PStream::~PStream()
{
   free(buf_data);
}

void
PStream::close()
{
   ::close(fd);
}

void
PStream::reset()
{
   linesout = 0;
   colsout = 0;
}

// empty the buffer to file descriptor
void
PStream::flush()
{
   int len = ch - buf_data;
   int offset = 0;

   while (len > 0) {
      int n = write(fd, buf_data + offset, len);
      if (n <= 0) throw Exception("write to stdout failed");

      len -= n;
      offset += n;
   }

   ch = buf_data;
}

// check that line doesn't overflow page
void
PStream::lineout()
{
   if (!paging || width <= 0 || height <= 0) {
      return;
   }

   linesout++;
   colsout = 0;

   if (linesout >= height - 2) {
      page_full();
   }
}

// the page is full, prompt to continue or quit
void
PStream::page_full()
{
   static const char* query = 
     "---Type <return> to continue, or q to quit---";
   int n;

   // dump stored lines
   flush();
   reset();

   // call write directly, avoid recursion
   n = write(fd, query, strlen(query));
   if (n <= 0) throw Exception("write to stdout failed");

   // read in prompt answer
   int c = readl.getchar("q\r\n");

   // push newline, readline doesn't
   n = write(fd, "\n", 1);
   if (n <= 0) throw Exception("write to stdout failed");

   if (c == 'q') {
      throw QuitOutput();
   }
}

// paged equivalent of normal fprintf
void
PStream::print(const char *fmt, ...)
{
   char pbuf[DEFAULT_BUFSIZE];
   int print_len;

   // write the answer to a temporary buffer
   // this will only do DEFAULT_BUFSIZE at once
   va_list arg;
   va_start(arg, fmt);
   print_len = vsnprintf(pbuf, DEFAULT_BUFSIZE, fmt, arg);
   va_end(arg);

   // make room in buffer new data.  buffer will never
   // contain a newline, it is checked for and flushed
   if (ch+print_len > buf_data+buf_len) {
      flush();
   }

   // scan new data for newlines, drop lines
   // one at a time to generate correct prompt
   bool doflush = false;
   for (int i = 0; i < print_len; i++) {
      *ch++ = pbuf[i];
      colsout++;
      if (pbuf[i] == '\n') {
	 doflush = true;
	 lineout();
      } else if (colsout >= width) {
	 lineout();
      }
   }

   // flush buffer if contains newline
   if (doflush) {
      flush();
   }
}

