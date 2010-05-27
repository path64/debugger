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
 * File: pstream.h
 * Created on: Fri Aug 13 11:02:27 PDT 2004
 * Author: David Allison <dallison@pathscale.com>
 * 
 */

#ifndef _PSTREAM_H_
#define _PSTREAM_H_

// this is thrown to stop the output
class QuitOutput {
};

class PStream {
 public:
   PStream(int fd);
   PStream(const char* fname);
   ~PStream();

   void close();
   void reset();
   void flush();

   void print(const char *format, ...);

   void set_width(int w) {
      width = w;
   }
   void set_height(int h) {
      height = h;
   }

   void set_paging(bool v) {
      paging = v;
   }
   void begin_paging() {
      set_paging(true);
      depth++;
   }
   void end_paging() {
      depth--;
      reset();
      set_paging(false);
   }

   int get_width() {
      return width;
   }
   int get_hight() {
      return height;
   }
   int get_column() {
      return colsout;
   }

 protected:
   void lineout();		// called when new line is detected
   void page_full();		// called when the page is full

   int fd;                      // file descriptor
   char* ch;                    // position in buffer
   int width;                   // width of the window
   int height;                  // height of the window
   bool paging;                 // should page output
   int depth;                   // paging nest depth
   char* buf_data;              // allocated buffer
   int buf_len;                 // max buffer size
   int linesout;                // number of lines output
   int colsout;                 // number of columns output
};

#endif
