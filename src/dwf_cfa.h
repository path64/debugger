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

file: dwf_cfa.h
created on: Fri Aug 13 11:02:27 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef _DWF_CFA_H_
#define _DWF_CFA_H_

/* size of augmentation string */
#define MAX_AUGSTR 16 

class CFAEntry {
public:
    CFAEntry(DwInfo *dwarf) ;
    ~CFAEntry() ;
    BVector get_instructions () ;
protected:
    void read_instructions (BStream & stream, int len) ;
    BVector instructions ;
    DwInfo *dwarf ;
private:
} ;

class CIE: public CFAEntry  {
public:
    CIE(DwInfo *dwarf, Offset cieoffset) ;
    ~CIE() ;
    void read (Section *section, BStream & stream, int id, int length, bool is_eh) ;
    Offset get_offset () ;
    int get_code_align() ;
    int get_data_align() ;
    int get_ra () ;
    void print() ;
    bool is_z_aug() { return has_z_aug ;}
    int get_fde_encoding() { return fde_enc; }
    int get_encoding(int size) ;
protected:
private:
    char augstr[MAX_AUGSTR+1];
    Offset cieoffset ;
    BVector instructions ;
    int code_align;
    int data_align;
    int retaddr;
    Address eh_ptr;
    bool has_z_aug;
    byte version;
    int fde_enc;
} ;

class FDE: public CFAEntry  {
public:
    FDE(DwInfo *dwarf) ;
    ~FDE() ;
    void read (Section *section, BStream & stream, int id, Offset offset, int length, bool is_eh) ;
    Address get_start_address () ;
    Address get_end_address() { return end_address ; }
    void print () ;
    CIE* get_cie () ;
protected:
private:
    Address read_value (Section *section, CIE *cie, BStream & stream, int mask) ;
    CIE *cie ;
    Address start_address ;
    Address end_address ;
    BVector instructions ;
} ;

#endif
