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
