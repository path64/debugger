/*

   Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
   PathDB is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation version 3

   PathDB is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with PathDB; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

File:     map_range.h
Created:  Mon May  2 15:25:14 PDT 2005
Author:   James Strother <jims@pathscale.com>

*/

#ifndef _MAP_RANGE_H_
#define _MAP_RANGE_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

template<class RTYPE, class VTYPE>
class Map_Range
{
/* This class provides a lookup for items that fall within
 * some range.  It maintains the items in a sorted list
 * and uses a binary search to do the lookup.  The range
 * is inclusive, so it will match if it is between or equal
 * to one of the limits.
 *
 * It requires a less-than operator, equality operator,
 * and assignment operator for RTYPE.  It requires only
 * an assignment operator for the VTYPE.
 */
public:

   /* Constructors and destructors.  Note that
    * the copy constructor is disallowed.
    */
private:
   Map_Range(const Map_Range&);

public:
   Map_Range() {
      chunk = 10; /* default chunk size */
      data = NULL;
      cpos = 0;
      size = 0;
      unsort = false;
   }

   Map_Range(long _c) {
      if (_c <= 0) abort();

      chunk = _c;
      data = NULL;
      cpos = 0;
      size = 0;
      unsort = false;
   }

   ~Map_Range() {
      if (data != NULL) {
         free(data);
      }
   }

   /* Insert item so list remains sorted */
   bool add(RTYPE lo, RTYPE hi, VTYPE val);

   /* Append list without the sort.  The
    * sort is performed at first call to
    * either get() or find()
    */
   bool raw(RTYPE lo, RTYPE hi, VTYPE val);
   void sort();

   /* Find an item with given range */
   bool get(RTYPE key, VTYPE* val) {
      if (unsort) sort();

      long i = bin_search(data, cpos, key);

      if (i == -1)
         return true;

      if (val != NULL) {
         *val = data[i].val;
      }
      return false;
   }

   class iterator;
   iterator find(RTYPE key) {
      if (unsort) sort();

      long i = bin_search(data, cpos, key);
      return iterator(this, i);
   }

   /* Informative member functions */
   long length() { return cpos; }

   /* Debugging tools */
   bool check();

   /* Iterator for traversing the mapped values */
   class triplet {
   public:
      RTYPE lo;
      RTYPE hi;
      VTYPE val;
   };

   class iterator {
   public:
      iterator(Map_Range<RTYPE,VTYPE>* _data, long _idx) {
         data = _data;
         idx = _idx;
      }

      iterator() {
         data = NULL;
         idx = -1;
      }

      const triplet& operator*() {
         return data->get_triplet(idx);
      }

      const triplet* operator->() {
         return &data->get_triplet(idx);
      }

      iterator operator++() {
         if (idx < 0 || data == NULL ||
             idx >= data->length()-1) {
            idx = -1;
         } else {
            idx += 1;
         }

         return *this; 
      }

      iterator operator++(int) {
         iterator tmp = *this;
 
         if (idx < 0 || data == NULL ||
             idx >= data->length()-1) {
            idx = -1;
         } else {
            idx += 1;
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
      Map_Range<RTYPE,VTYPE>* data;
      long idx; 
   }; 

   /* Map function to pull out iterators */
   iterator begin() {
      if (cpos == 0) {
         return end();
      }
      if (unsort) sort();
      return iterator(this, 0);
   }

   iterator end() {
      if (unsort) sort();
      return iterator(this, -1);
   }

   /* Map function to pull out a triplet */
   const triplet& get_triplet(long idx) {
      if (idx < 0 || idx >= cpos) abort();
      return data[idx];
   }

private:
   
   /* Add room for one more element */
   bool chk_resize();

   /* Workhorse for binary search algorithm */
   long bin_search(triplet*, long, RTYPE);

   /* Workhorse for quicksort algorithm */
   static void quicksort(triplet*, long, long, triplet*);

   triplet* data;
   long   cpos;
   long   size;
   long  chunk;
   bool unsort;
};


template <class RTYPE, class VTYPE>
bool Map_Range<RTYPE, VTYPE>::chk_resize()
{
   if (cpos >= size) {
      triplet* nptr = (triplet*) realloc(data,
         (size+chunk)*sizeof(triplet));

      if (nptr == NULL) {
         return true;
      }
      data = nptr;
      size = size+chunk;
   }

   return false;
}


template <class RTYPE, class VTYPE>
bool Map_Range<RTYPE, VTYPE>::add(
   RTYPE lo,
   RTYPE hi,
   VTYPE val)
{
   if (chk_resize())
      return true;

   if (unsort) sort();

   for (long i=0; i<cpos; i++) {
      if (lo < data[i].lo) {
         memmove(data+i+1, data+i,
           (cpos-i)*sizeof(triplet));

         data[i].lo = lo;
         data[i].hi = hi;
         data[i].val = val;

         cpos++;
         return false;
      } 
   }

   data[cpos].lo = lo;
   data[cpos].hi = hi;
   data[cpos].val = val;

   cpos++;
   return false;
}

template <class RTYPE, class VTYPE>
bool Map_Range<RTYPE, VTYPE>::raw(
   RTYPE lo,
   RTYPE hi,
   VTYPE val)
{
   if (chk_resize())
      return true;

   triplet* p_data = data+cpos;
   p_data->lo = lo;
   p_data->hi = hi;
   p_data->val = val;
   unsort = true;

   cpos++;
   return false;
}

template <class RTYPE, class VTYPE>
void Map_Range<RTYPE, VTYPE>::quicksort(
   triplet* d,
   long l,
   long r,
   triplet* swp)
{
   long i, j;
   RTYPE v;

   if (r <= l) return;

   i = l - 1;
   j = r;
   v = d[r].lo;

   for (;;) {
      while (d[++i].lo < v);
      while (v < d[--j].lo) {
         if (j == l) break;
      }

      if (i >= j) break;

      memcpy(swp,d+j,sizeof(triplet));
      memcpy(d+j,d+i,sizeof(triplet));
      memcpy(d+i,swp,sizeof(triplet));
   }

   if (i != r) {
      memcpy(swp,d+i,sizeof(triplet));
      memcpy(d+i,d+r,sizeof(triplet));
      memcpy(d+r,swp,sizeof(triplet));
   }

   quicksort(d, l, i-1, swp);
   quicksort(d, i+1, r, swp);
}

template <class RTYPE, class VTYPE>
void Map_Range<RTYPE, VTYPE>::sort()
{
   triplet swp;

   unsort = false;

   if (cpos <= 1) return;
   quicksort(data, 0, cpos-1, &swp); 
}

template <class RTYPE, class VTYPE>
long Map_Range<RTYPE, VTYPE>::bin_search(
   triplet* d,
   long lim,
   RTYPE v)
{
   long low, mid, high;

   if (lim == 0) return -1;

   low = 0;
   high = lim - 1;

   while (low <= high) {
      mid = (low+high)/2;
      if (v == d[mid].lo) {
         return mid;
      } else if (v < d[mid].lo) {
         high = mid - 1;
      } else {
         low = mid + 1;
      }
   }

   if (high < 0) {
      high = 0;
   }

   if (v < data[high].lo ||
       data[high].hi < v) {
      return -1;
   }

   return high;
}

template <class RTYPE, class VTYPE>
bool Map_Range<RTYPE, VTYPE>::check()
{
   for (long i=0; i<cpos-1; i++) {
      if (data[i].hi < data[i].lo ||
          data[i+1].lo < data[i].hi ||
          data[i+1].lo == data[i].hi) {
         return true;
      } 
   }
   return false;
}

#endif
