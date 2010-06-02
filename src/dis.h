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

file: dis.h
created on: Fri Aug 13 11:02:28 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef dis_h_included
#define dis_h_included

#include <iostream>
#include "dbg_types.h"
#include "opcodes.h"
#include "pstream.h"

class Process ;
class DIE ;

typedef std::map<int, DIE*> LocalMap ;

class Disassembler {
public:
    virtual ~Disassembler() { }
    virtual int disassemble (Process *proc, PStream &os, Address addr, unsigned char *instruction, LocalMap &locals) = 0 ;                // return length of instruction
    void add_annotation (std::string annot) ;
    void print_annotation (PStream &os) ;
    void clear_annotation() ;
    void annotate_address (Process *proc, Address addr) ;
private:
    std::string annotation ;

} ;


class OpteronDisassembler : public Disassembler {
public:
    OpteronDisassembler (bool is64):is64bit(is64) {}
    int disassemble (Process *proc, PStream &os, Address addr, unsigned char *instruction, LocalMap &locals) ;
private:
    unsigned char *instptr ;
    unsigned char *instructions ;
    int rex ;
    bool rexpresent ;

    int modrm ;
    int sib ;
    bool hasmodrm ;
    int64_t disp ;          // displacement
    int64_t immediate ;     // immediate 
    int immediate_len ;     // length of immediate
    int has_flag66;

    void print_operand (PStream &os, const char* mnemonic, const char *opdesc, bool &comma) ;
    Group *find_group (int grpnum, int opcode) ;
    bool needs_modrm (Instruction *inst) ;
    bool needs_sib () ;
    int64_t extract_value (const char *desc, int &len) ;
    void print_mem (PStream &os, int mod, const char *desc) ;
    void print_sib (PStream &os, int mod) ;
    void print_data (PStream &os) ;
    void show_local (int offset) ;
    bool is_frame_pointer (const char *reg) ;

    void fetch_displacement(Instruction *inst) ;
    void fetch_operand_displacement (const char *desc) ;
    void fetch_sib_displacement (int mod) ; 
    void fetch_immediate (const char *desc) ;

    int rex_b() { return rexpresent ? ((rex>>0) & 0x1) : 0 ; }
    int rex_x() { return rexpresent ? ((rex>>1) & 0x1) : 0 ; }
    int rex_r() { return rexpresent ? ((rex>>2) & 0x1) : 0 ; }
    int rex_w() { return rexpresent ? ((rex>>3) & 0x1) : 0 ; }

    void print_reg (int reg, char size, char size2, PStream &os) ;

    Address current_addr ;
    bool is64bit ;
    Process *proc ;
    LocalMap *locals ;
} ;

#endif
