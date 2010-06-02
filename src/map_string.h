/*

 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at 
 * http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/CDDL.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END

 * Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
 * Use is subject to license terms.

File:     map_string.h
Created:  Mon May  2 15:25:14 PDT 2005
Author:   James Strother <jims@pathscale.com>

*/

#ifndef _MAP_STRING_H_
#define _MAP_STRING_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

template<class VTYPE>
class Map_String
{
/* This class provides a lookup for a string table.  This
 * supports partial matching, as is useful for completion,
 * but also allows for exact matching.  It maintains the
 * items in a sorted list and uses a binary search to do
 * the lookup.
 *
 * For exact matches, this class can be advantageous over
 * STL maps in that string pointers are stored in contiguous
 * memory rather than as a binary tree.  This reduces the
 * memory footprint and startup cost but increases the cost
 * of adding elements later.  Access speed is improved as
 * much pointer chasing is eliminated.  It does allow "raw"
 * appends so that many items can be added, and the sorting
 * delayed until the list is complete.  These things can be
 * accomplished with an appropriate wrapper over an STL list,
 * but this class was needed for partial matches anyway and
 * it can be optimized for strings rather than general keys.
 *
 * It requires only an assignment operator for the VTYPE.
 */
public:

   /* Constructors and destructors.  Note that
    * the copy constructor is disallowed.
    */
private:
   Map_String(const Map_String&);

public:
   Map_String() {
      chunk = 10; /* default chunk size */
      data = NULL;
      cpos = 0;
      size = 0;
      unsort = false;
   }

   Map_String(long _c) {
      if (_c <= 0) abort();

      chunk = _c;
      data = NULL;
      cpos = 0;
      size = 0;
      unsort = false;
   }

   ~Map_String() {
      for (long i=0; i<cpos; i++) {
         free((char*)data[i].key);
      }
      if (data != NULL) {
         free(data);
      }
   }

   /* Insert item so list remains sorted */
   bool add(const char* key, VTYPE val);

   /* Append list without the sort.  If you
    * call raw() you must must call the sort
    * function before any call to find() or
    * get() member.
    */
   bool raw(const char* key, VTYPE val);
   void sort();

   /* Find list of matching tokens */
   class iterator;
   iterator find(const char* key) {
      if (cpos == 0) return end();
      if (unsort) sort();

      long lo = search_lo(data, cpos, key);
      if (lo == -1) return end();

      long hi = search_hi(data+lo, cpos-lo, key);
      if (hi == -1) return end();

      return iterator(this, lo, hi+1);
   }

   /* Find a particular matching token */
   iterator find_exact(const char* key) {
      if (cpos == 0) return end();
      if (unsort) sort();

      long lo = search_exact(data, cpos, key);
      if (lo == -1) return end();

      return iterator(this, lo, 1); 
   }

   /* Informative member functions */
   long length() { return cpos; }

   /* Debugging tools */
   bool check();

   /* Iterator for traversing the mapped values */
   class doublet {
   public:
      const char* key;
      VTYPE value;
   };

   class iterator {
   public:
      iterator(Map_String<VTYPE>* _data, long _idx, long _left) {
         data = _data;
         idx = _idx;
         left = _left;
      }

      iterator() {
         data = NULL;
         idx = -1;
         left = 0;
      }

      const doublet& operator*() {
         return data->get_doublet(idx);
      }

      const doublet* operator->() {
         return &data->get_doublet(idx);
      }

      iterator operator++() {
         if (idx < 0 || left <= 1) {
            idx = -1;
            left = 0;
         } else {
            idx += 1;
            left -= 1;
         }

         return *this; 
      }

      iterator operator++(int) {
         iterator tmp = *this;
 
         if (idx < 0 || left <= 1) {
            idx = -1;
            left = 0;
         } else {
            idx += 1;
            left -= 1;
         }

         return tmp;
      }

      bool operator==(const iterator& X) {
         return (idx == X.idx);
      }

      bool operator!=(const iterator& X) {
         return (idx != X.idx);
      }

   private:
      Map_String<VTYPE>* data;
      long idx;
      long left;
   }; 

   /* Map function to pull out iterators */
   iterator begin() {
      if (cpos == 0) {
         return end();
      }
      if (unsort) sort();
      return iterator(this, 0, cpos);
   }

   iterator end() {
      if (unsort) sort();
      return iterator(this, -1, 0);
   }

   /* Map function to pull out a doublet */
   const doublet& get_doublet(long idx) {
      if (idx < 0 || idx >= cpos) abort();
      return data[idx];
   }

private:
   
   /* Add room for one more element */
   bool chk_resize();

   /* Workhorses for binary search algorithm */
   long search_lo(doublet*, long, const char*);
   long search_hi(doublet*, long, const char*);
   long search_exact(doublet*, long, const char*);

   /* Workhorse for quicksort algorithm */
   static void quicksort(doublet*, long, long, doublet*);

   /* String comparison function hooks */
   static bool s_less(const char* a, const char* b) {
      return strcmp(a, b) < 0;
   }
   static int s_cmp(const char* a, const char* b) {
      return strcmp(a, b);
   }
   static bool s_match(const char* a, const char* b) {
      while (*a != '\0' && *a == *b) {
         a++; b++;
      }

      if (*a == '\0') {
         return true;
      }

      return false;
   }

   doublet* data;
   long   cpos;
   long   size;
   long  chunk;
   bool unsort;
};


template <class VTYPE>
bool Map_String<VTYPE>::chk_resize()
{
   if (cpos >= size) {
      doublet* nptr = (doublet*) realloc(data,
         (size+chunk)*sizeof(doublet));

      if (nptr == NULL) {
         return true;
      }
      data = nptr;
      size = size+chunk;
   }

   return false;
}


template <class VTYPE>
bool Map_String<VTYPE>::add(
   const char* key,
   VTYPE value)
{
   if (chk_resize())
      return true;

   if (unsort) sort();

   for (long i=0; i<cpos; i++) {
      if ( s_less(key,data[i].key) ) {
         memmove(data+i+1, data+i,
           (cpos-i)*sizeof(doublet));

         data[i].key = strdup(key);
         data[i].value = value;

         cpos++;
         return false;
      } 
   }

   data[cpos].key = strdup(key);
   data[cpos].value = value;

   cpos++;
   return false;
}

template <class VTYPE>
bool Map_String<VTYPE>::raw(
   const char* key,
   VTYPE value)
{
   if (chk_resize())
      return true;

   doublet* p_data = data+cpos;
   p_data->key = strdup(key);
   p_data->value = value;
   unsort = true;

   cpos++;
   return false;
}

template <class VTYPE>
void Map_String<VTYPE>::quicksort(
   doublet* d,
   long l,
   long r,
   doublet* swp)
{
   const char* v;
   long i, j;

   if (r <= l) return;

   i = l - 1;
   j = r;
   v = d[r].key;

   for (;;) {
      while ( s_less(d[++i].key, v) );
      while ( s_less(v, d[--j].key) ) {
         if (j == l) break;
      }

      if (i >= j) break;

      memcpy(swp,d+j,sizeof(doublet));
      memcpy(d+j,d+i,sizeof(doublet));
      memcpy(d+i,swp,sizeof(doublet));
   }

   if (i != r) {
      memcpy(swp,d+i,sizeof(doublet));
      memcpy(d+i,d+r,sizeof(doublet));
      memcpy(d+r,swp,sizeof(doublet));
   }

   quicksort(d, l, i-1, swp);
   quicksort(d, i+1, r, swp);
}

template <class VTYPE>
void Map_String<VTYPE>::sort()
{
   doublet swp;

   unsort = false;

   if (cpos <= 1) return;
   quicksort(data, 0, cpos-1, &swp); 
}

template <class VTYPE>
long Map_String<VTYPE>::search_lo(
   doublet* d,
   long lim,
   const char* v)
{
/* Searchs for the lowest element which
 * still matches against the given key.
 */

   long low, mid, high;

   if (lim == 0) return -1;

   low = 0;
   high = lim - 1;

   while (low <= high) {
      mid = (low+high)/2;

      int out = s_cmp(v, d[mid].key);
      if ( out == 0 ) {
         return mid;
      } else if (out < 0) {
         high = mid - 1;
      } else {
         low = mid + 1;
      }
   }

   if (high >= 0 && high < lim &&
       s_match(v, d[high].key)) {
      return high; 
   }

   if (low >= 0 && low < lim &&
       s_match(v, d[low].key)) {
      return low;
   }

   return -1;
}

template <class VTYPE>
long Map_String<VTYPE>::search_hi(
   doublet* d,
   long lim,
   const char* v)
{
/* Searchs for the highest element which
 * still matches against the given key.
 */
   long low, mid, high;

   if (lim == 0) return -1;

   low = 0;
   high = lim - 1;

   while (low < high - 1) {
      mid = (low+high)/2;

      if ( s_match(v, d[mid].key) ) {
         low = mid;  
      } else {
         high = mid - 1;
      }
   }

   for (long i=high; i>=low; i--) {
      if (s_match(v, d[i].key)) {
         return i;
      }
   }

   return -1;
}

template <class VTYPE>
long Map_String<VTYPE>::search_exact(
   doublet* d,
   long lim,
   const char* v)
{
/* Searchs for an exact match to the 
 * given key in the stored list.
 */

   long low, mid, high;

   if (lim == 0) return -1;

   low = 0;
   high = lim - 1;

   while (low <= high) {
      mid = (low+high)/2;

      int out = s_cmp(v, d[mid].key);
      if ( out == 0 ) {
         return mid;
      } else if (out < 0) {
         high = mid - 1;
      } else {
         low = mid + 1;
      }
   }

   return -1;
}

template <class VTYPE>
bool Map_String<VTYPE>::check()
{
   for (long i=0; i<cpos-1; i++) {
      if ( s_less(data[i+1].key, data[i].key) ) {
         return true;
      }
   }
   return false;
}

#endif
