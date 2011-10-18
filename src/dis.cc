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

file: dis.cc
created on: Fri Aug 13 11:07:38 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "dis.h"
#include "process.h"
#include "dbg_elf.h"

extern Instruction one_byte[] ;
extern Instruction two_byte[] ;
extern Instruction two_byte_66[] ;
extern Instruction two_byte_F2[] ;
extern Instruction two_byte_F3[] ;
extern Instruction x87[] ;
extern Instruction x87_alt[] ;
extern Group groups[] ;

void Disassembler::clear_annotation () {
    annotation = "" ;
}

void Disassembler::add_annotation (std::string s) {
    if (annotation != "") {
        annotation += ' ' ;
    }
    annotation += s ;
}

void Disassembler::print_annotation (PStream &os) {
    if (annotation != "") {
#if 0
        // pad to right of screen
        int width = os.get_width() ;
        int x = os.get_column() ;
        int len = annotation.size() ;
        int pad = width - len - x - 1 ;
        while (pad-- > 0) {
            os.print (" ") ;
        }
#endif
        os.print (" %s", annotation.c_str()) ;
    }
}

void Disassembler::annotate_address (Process *proc, Address addr) {
    bool printsym = true ;
    Address _end = proc->lookup_function ("_end") ;
    if (addr >= _end) {
        printsym = false ;
    } else {
        Address _start = proc->lookup_function ("_start") ;
        if (addr < _start) {
            printsym = false ;
        } else {
            Section *section = proc->find_section_at_addr (addr) ;
            if (section != NULL) {
                if (section->get_name() == ".rodata") {
                    printsym = false ;
                }
            }
        }
    }
    if (printsym) {
        Location loc = proc->lookup_address (addr) ;
        if (addr != 0 && loc.get_symname() != "") {
            if (loc.get_offset() < 100000) {
                char buf[256] ;
                if (loc.get_offset() == 0) {
                    snprintf (buf,sizeof(buf), "<%s>", loc.get_symname().c_str()) ;
                } else {
                    snprintf (buf,sizeof(buf), "<%s+%d>", loc.get_symname().c_str(), loc.get_offset()) ;
                }
                add_annotation (buf) ;
            }
        }
   }
}

int OpteronDisassembler::disassemble (Process *proc, PStream &os, Address addr, unsigned char *insts, LocalMap &lcls) {
    this->proc = proc ;
    this->locals = &lcls ;
    instructions = insts ;
    instptr = insts ;
    current_addr = addr ;
    rexpresent = false ;

    clear_annotation() ;

    disp = 0;
    int opcode = (int)*instptr++ ;
    int row = opcode >> 4 ;
    int column = opcode & 0xf ;
    int prefix = 0 ;
    int groupop = 0 ;                   // opcode for group
    bool print_prefix = false ;

    Instruction *instruction = NULL ;

    has_flag66 = (opcode == 0x66);

    if (opcode == 0xf3 || opcode == 0xf2 || opcode == 0x66 || opcode == 0xf0) {
        prefix = opcode ;
        if (opcode != 0x66) {
            groupop = prefix << 16 ;
        }
        opcode = (int)*instptr++ ;
        row = opcode >> 4 ;
        column = opcode & 0xf ;
    }
    if (opcode >= 0x40 && opcode <= 0x4f) {     // REX instruction
        rex = opcode ;
        rexpresent = true ;
        opcode = (int)*instptr++ ;              // fetch another one
        row = opcode >> 4 ;
        column = opcode & 0xf ;
    }

    if (opcode >= 0xd8 && opcode <= 0xdf) {              // x87 opcodes
        modrm = (int)*instptr ;                       // fetch x87 opcode byte
        if (modrm < 0xc0) {                     // alternative table
            int reg = (modrm >> 3) & 0x7 ;
            int index = (opcode - 0xd8) * 8 + reg ;
           instruction = &x87_alt[index] ; 
        } else {
           // the x87 table is a straight linear append of all the tables in pages 387 - 394
           // each section table is 64 entries long and represents one opcode
           int index = (opcode - 0xd8) * 64 + (modrm - 0xc0) ;
           instruction = &x87[index] ; 
        }
    } else if (opcode != 0x0f) {               // single byte?
        // convert the row and column into an index into the instruction table
        int index = 0 ;
        if (column > 7) {
            index = 16*8 + (row * 8) + (column - 8) ;               // second half of table
        } else {
            index = (row * 8) + column ;                            // first half of table
        }
        instruction = &one_byte[index] ;
        print_prefix = true ;
    } else {
        groupop |= 0x0f00 ;                      // add prefix to groupop

        opcode = (int)*instptr++ ;              // fetch second byte of opcode
        row = opcode >> 4 ;
        column = opcode & 0xf ;
        int index = 0 ;
        if (column > 7) {
            index = 16*8 + (row * 8) + (column - 8) ;               // second half of table
        } else {
            index = (row * 8) + column ;                            // first half of table
        }
        switch (prefix) {
        case 0:
            instruction = &two_byte[index] ;
            break ;
        case 0xf2:
            instruction = &two_byte_F2[index] ;
            break ;
        case 0xf3:
            instruction = &two_byte_F3[index] ;
            break ;
        case 0x66:
            instruction = &two_byte_66[index] ;
            break ;
        }
    }

    if (instruction != NULL) {
        hasmodrm = false ;
        if (instruction->mnemonic != NULL) {
            if (instruction->mnemonic[0] == '~') {                    // group?
                int grpnum = atoi (&instruction->mnemonic[1]) ;
                Group *grp = find_group (grpnum, groupop | opcode) ;
                if (grp == NULL) {
                    throw Exception ("unable to find group (%d,%x,%x)", grpnum, groupop, opcode) ;
                }
                modrm = (int)*instptr++ ;               // need to modrm byte
                hasmodrm = true ;                       // note that we have extracted the modrm byte
                int extension = (modrm >> 3) & 0x7 ;            // extract the opcode extension
                instruction = &grp->instructions[extension] ;           // retarget instruction to the group instruction
            }
        }
        if (!hasmodrm && needs_modrm(instruction)) {
            modrm = (int)*instptr++ ;
            hasmodrm = true ;
        }
        if (needs_sib()) {
            sib = (int)*instptr++ ;
        }
        fetch_displacement (instruction) ;

        print_data (os) ;                       // print the actual data

        if (print_prefix) {
            switch (prefix) {
            case 0xf0:
                os.print ("lock ") ;
                break ;
            case 0xf2:
                os.print ("repnz ") ;
                break ;
            case 0xf3:
                if (opcode == 0xa6 || opcode == 0xa7 || opcode == 0xae || opcode == 0xaf) {
                    os.print ("repz ") ;
                } else {
                    os.print ("rep ") ;
                }
                break ;
            }
        }
        int pad = 6 - strlen (instruction->mnemonic) ;
        os.print ("%s ", instruction->mnemonic) ;
        while (pad-- > 0) {
            os.print (" ") ;
        }

        bool comma = false ;
        print_operand (os, instruction->mnemonic, instruction->op2, comma) ;       // AT&T format, op2 comes first
        print_operand (os, instruction->mnemonic, instruction->op1, comma) ;
        print_operand (os, instruction->mnemonic, instruction->op3, comma) ;
    }
    print_annotation(os) ;
    return instptr - instructions ;
}

Group *OpteronDisassembler::find_group (int grpnum, int opcode) {
    for (int i = 0 ; groups[i].number >= 0 ; i++) {
        if (groups[i].number == grpnum && groups[i].opcode == opcode) {
            return &groups[i] ;
        }
    }
    return NULL ;
}


static bool checkmodrm (const char *desc) {
    if (desc == NULL) {
        return false ;
    }
    switch (desc[0]) {
    case 'E':
    case 'C':
    case 'D':
    case 'G':
    case 'M':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'V':
    case 'W':
    case '$':
       return true ;
    }
    return false ;
}

bool OpteronDisassembler::needs_modrm (Instruction *inst) {
    return checkmodrm (inst->op1) ||
           checkmodrm (inst->op2) ||
           checkmodrm (inst->op3) ;
}

bool OpteronDisassembler::needs_sib() {
    if (!hasmodrm) {
        return false ;
    }
    
    return ((modrm >> 6) != 3) && (modrm & 0x7) == 4 ;
}

static const char *regs8[] = {
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
    "r8b",
    "r9b",
    "r10b",
    "r11b",
    "r12b",
    "r13b",
    "r13b",
    "r15b"
} ;

static const char *regs16[] = {
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
    "r8w",
    "r9w",
    "r10w",
    "r11w",
    "r12w",
    "r13w",
    "r13w",
    "r15w"
} ;

static const char *regs32[] = {
    "eax",
    "ecx",
    "edx",
    "ebx",
    "esp",
    "ebp",
    "esi",
    "edi",
    "r8d",
    "r9d",
    "r10d",
    "r11d",
    "r12d",
    "r13d",
    "r13d",
    "r15d"
} ;

static const char *regs64[] = {
    "rax",
    "rcx",
    "rdx",
    "rbx",
    "rsp",
    "rbp",
    "rsi",
    "rdi",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r13",
    "r15"
} ;

static const char *sseregs[] = {
    "xmm0",
    "xmm1",
    "xmm2",
    "xmm3",
    "xmm4",
    "xmm5",
    "xmm6",
    "xmm7",
    "xmm8",
    "xmm9",
    "xmm10",
    "xmm11",
    "xmm12",
    "xmm13",
    "xmm14",
    "xmm15"
} ;

void OpteronDisassembler::print_reg (int reg, char size, char size2, PStream &os) {
    const char **regset = NULL ;
    switch (size) {
    case 'b':
        regset = regs8 ;
        break ;
    case 'w':
        regset = regs16 ;
        break ;
    case 'd':
        regset = regs32 ;
        break ; 
    case 'q':
        regset = regs64 ;  
        break ;
    case 'z':
        regset = regs32 ;
        break ;
    case 'v':
        if (is64bit && rex_w()) {
            regset = regs64 ;
        } else {
            regset = regs32 ;
        }
        break ;
    default:
        regset = regs32 ;                       // XXX: this is wrong
    }
    os.print ("%%%s", regset[reg]) ;
}

static void print_sse_reg (int reg, char size, char size2, PStream &os) {
    os.print ("%%%s", sseregs[reg]) ;
}

// extract an immediate or offset value from the instruction
int64_t OpteronDisassembler::extract_value (const char *desc, int &len) {
    int64_t immed = 0 ;
    switch (desc[0]) {
    case 'b':
        immed = (int64_t)*instptr++ ;
        if (immed & 0x80) {
            immed <<= 24+32 ;
            immed >>= 24+32 ;
        }
        len = 1 ;
        break ;
    case 'w':
        for (int i = 0 ; i < 2 ; i++) {
            immed |= ((int64_t)*instptr++) << (i*8) ;
        }
        if (immed & 0x8000) {
            immed <<= 16+32 ;
            immed >>= 16+32 ;
        }
        len = 2 ;
        break ;
    case 'd':
    case 'z':
        len = has_flag66 ? 2 : 4;
        for (int i = 0 ; i < len ; i++) {
            immed |= ((int64_t)*instptr++) << (i*8) ;
        }
        if (immed & 0x80000000) {
            immed <<= 32 ;
            immed >>= 32 ;
        }
        break ;
    case 'v':
        len = rex_w() ? 8 : 4 ;
        for (int i = 0 ; i < len ; i++) { 
            immed |= ((int64_t)*instptr++) << (i*8) ;
        }
        break ;
    case 'q':
        for (int i = 0 ; i < 8 ; i++) {
            immed |= ((int64_t)*instptr++) << (i*8) ;
        }
        len = 8 ;
        break ;
    }
    return immed ;
}

// fetch the displacement if it is present.  This needs to work out what
// size the displacement is
void OpteronDisassembler::fetch_displacement(Instruction *inst) {
    fetch_operand_displacement (inst->op1) ;
    fetch_operand_displacement (inst->op2) ;
    fetch_operand_displacement (inst->op3) ;

    fetch_immediate (inst->op1) ;
    fetch_immediate (inst->op2) ;
    fetch_immediate (inst->op3) ;
}

void OpteronDisassembler::fetch_operand_displacement (const char *desc) {
    if (desc == NULL) {
       return ; 
    }
    if (desc[0] == 'E' || desc[0] == 'M' || desc[0] == 'W') {
        int mod = (modrm >> 6) & 0x3 ;
        if (mod != 3) {
            int rm = modrm & 0x7 ;
            switch (mod) {
            case 0:
                if (rm == 4) {
                    fetch_sib_displacement (mod) ;
                } else if (rm == 5) {
                    int len ;
                    disp = extract_value ("d", len) ;
                }
                break ;
            case 1: {
                disp = (int)*instptr++ ;
                if (disp & 0x80) {
                   disp <<= 24 + 32;
                   disp >>= 24 + 32;
                }                       // XXX: can the SIB specify a displacement too?
                break ;
                }
            case 2: {
                disp = 0 ;
                for (int i = 0 ; i < 4 ; i++) {
                    disp |= (int)(*instptr++ << (i*8)) ;
                }
                if (disp & 0x80000000) {
                    disp <<= 32 ;
                    disp >>= 32 ;
                }                       // XXX: can the SIB specify a displacement too?
                break ;
                }
            }
        }
    }
}

void OpteronDisassembler::fetch_sib_displacement (int mod) {
    int base = sib & 0x7 ;

    int len ;
    if (base == 5) {            // no base reg?
        if (mod == 1) {
            disp = extract_value ("b", len) ;                // extract disp8
            if (disp & 0x8) {
                disp <<= 24 ;
                disp >>= 24 ;
            }
        } else {
            disp = extract_value ("d", len) ;                // extract disp32
        }
    } 
}

void OpteronDisassembler::fetch_immediate (const char *desc) {
    if (desc == NULL) {
        return ;
    }
    if (desc[0] == 'I' || desc[0] == 'J') {
        immediate = extract_value (desc+1, immediate_len) ;
    }
}


// print the value of an operand of an instruction given its description
void OpteronDisassembler::print_operand (PStream &os, const char* mnemonic, const char *desc, bool &comma) {
    if (desc == NULL) {
       return ; 
    }
    if (comma) {
       os.print (", ") ;
    }
    comma = true ;

    char mode = desc[0] ;
    switch (mode) {
    case '%': {                  // direct register name:
        const char* c = strchr(desc, '/');

        if (c != NULL) {
            if (rex_b()) {
               if (rex_w()) {
                  os.print ("%s", c+1);
               } else {
                  os.print ("%sd", c+1);
               }
            } else {
               if (desc[1] != 'X') {
                  os.print("%.4s", desc);
               }
               else if (rex_w()) {
                  os.print("%%r%.2s", desc+2);
               } else {
                  os.print("%%e%.2s", desc+2);
               }
            }
        }
        else if (desc[1] == 'X') {
            if (is64bit && rex_w()) {
                os.print ("%%r%s", desc+2) ;
            } else {
                os.print ("%%e%s", desc+2) ;
            }
        } else {
            os.print("%s", desc) ;
        }
        break ;
        }
    case '*':                   // direct register name with no conversion
        os.print ("%%%s", desc+1) ;
        break ;
    case 'G': {                 // register specied in modrm (reg field)
        int reg = (modrm >> 3) & 0x7 ;
        print_reg (reg | (rex_r() << 3), desc[1], desc[2], os) ;
        break ;
        }
    case 'E': {                 // register or memory address specified in modrm
        int mod = (modrm >> 6) & 0x3 ;
        switch (mod) {
        case 0:                 // indirect address via register
        case 1:
        case 2:
           // indirect calls have pretty '*' before address
           if (!strcmp(mnemonic,"call") || !strcmp(mnemonic,"jmp")) {
              os.print("*");
           }
           print_mem (os, mod, desc) ;
           break ;
        case 3: {                // register is the bottom 3 bits
            int reg = modrm & 0x7 ;
            print_reg (reg | (rex_b() << 3), desc[1], desc[2], os) ;
            break ;
            }
        }
        break ;
        }
    case 'V': {                  // sse reg
        if (desc[1] == 'R') {           // VR - sse reg specified by r/m field of modrm
            int rm = modrm & 0x7 ;
            print_sse_reg (rm | (rex_r() << 3), desc[1], desc[2], os) ;
        } else {
            int reg = (modrm >> 3) & 0x7 ;
            print_sse_reg (reg | (rex_r() << 3), desc[1], desc[2], os) ;
        }
        break ;
        }
    case 'W': {
        int mod = (modrm >> 6) & 0x3 ;
        switch (mod) {
        case 0:                 // indirect address via register
        case 1:
        case 2:
           print_mem (os, mod, desc) ;
           break ;
        case 3: {                // register is the bottom 3 bits
            int reg = modrm & 0x7 ;
            print_sse_reg (reg | (rex_b() << 3), desc[1], desc[2], os) ;
            break ;
            }
        }
        break ;
        }
    case 'I': {         // immediate value
        if (is64bit && rex_w()) {
            os.print ("$%ld", immediate) ;
        } else {
            os.print ("$%d", immediate) ;
        }
        annotate_address (proc, immediate) ;
        break ;
        }
    case 'J': {         // relative address
        int len = immediate_len ;
        int64_t offset = immediate ;
        Address dest = current_addr + len + 1 + offset ;
        os.print ("0x%llx", dest) ;
        annotate_address (proc, dest) ;
        break ;
        }
    case 'M':
        print_mem (os, (modrm >> 6) & 0x3, desc) ;
        break ;
    case 'X':
        os.print ("%%ds:(%%rsi)") ;
        break ;
    case 'Y':
        os.print ("%%es:(%%rdi)") ;
        break ;
    case '$':           // x87 immediate
        int mod = (modrm >> 6) & 0x3 ;
        print_mem (os, mod, desc) ;
        break ;
    }
}

void OpteronDisassembler::print_sib (PStream &os, int mod) {
    int scale = (sib >> 6) & 0x3 ;
    int index = (sib >> 3) & 0x7 ;
    int base = sib & 0x7 ;
    int mult = 1 << scale ;             // 2^scale
    const char *basereg = NULL ;
    const char *indexreg = NULL ;

    if (base != 5) {            // has base reg?
        basereg = is64bit ?
            regs64[base | (rex_b() << 3)] :
            regs32[base] ;
    }

    // See Table A-17 of the AMD manual
    if (index != 4) {
        indexreg = is64bit ?
            regs64[index | (rex_x() << 3)] :
            regs32[index] ;
    }

    if (basereg == NULL && indexreg == NULL) {
        os.print ("%lld", disp) ;
        return;
    }

    if (mod != 0 && disp != 0) {  /* has displacement */
       os.print("%lld", disp);
    }

    if (basereg != NULL) {       /* has base register */
       os.print ("(%%%s", basereg) ;
    } else {
       if (rex_b()) {
          os.print ("(%%r13");
       } else {
          os.print ("(%%rbp");
       }
    }

    if (indexreg != NULL) {     /* has index register */
       os.print (", %%%s", indexreg) ;
    }

    if (mult != 1) {           /* has scale factor */
       os.print (", %d", mult);
    }

    os.print (")");
}

// modrm and sib have already been fetched
void OpteronDisassembler::print_mem (PStream &os, int mod, const char *desc) {
    int rm = modrm & 0x7 ;              // XXX: is the REX_B added here or later?

    switch (mod) {
    case 0:
        if (rm != 4 && rm != 5) {               // straight register, no messing
            const char *regname = is64bit ?
                regs64[rm | (rex_b() << 3)] :
                regs32[rm] ;

            os.print ("(%%%s)", regname) ;
        } else {
            if (rm == 4) {      // SIB:
                print_sib (os, mod) ;
            } else {
                os.print ("%lld(%%rip)", disp) ;
                Address dest = current_addr + (instptr - instructions) + disp ;
                annotate_address (proc, dest) ;
            }
        }
        break ;
    case 1: 
    case 2: {
        if (rm == 4) {          // SIB?
            print_sib (os, mod) ;
        } else {
            const char *regname = is64bit ?
                 regs64[rm | (rex_b() << 3)] :
                 regs32[rm] ;

            if (disp != 0) {
               os.print ("%lld(%%%s)", disp, regname) ;
            }  else {
               os.print ("(%%%s)", regname) ;
            }

            if (is_frame_pointer (regname)) {
                show_local (disp) ;
            }
        }
        break ;
    }}
}

// print the data.  Since the instructions are variable in length, we need to
// print the data in a minimum field width and let it flow over if necessary

void OpteronDisassembler::print_data (PStream &os) {
    int len = instptr - instructions ;           // number of bytes to print
    unsigned char *data = instructions ;
    int width = 34 ;                            // 8 bytes min width
    os.print(" ") ;
    for (int i = 0 ; i < len ; i++, data++) {
        os.print ("%02x ", *data) ;
        width -= 3 ;
    }
    // pad to the width
    while (width-- > 0) {
        os.print (" ") ;
    }
}

void OpteronDisassembler::show_local (int offset) {
    LocalMap::iterator i = locals->find (offset) ;
    if (i != locals->end()) {
        DIE *die = i->second ;
        char buf[256] ;
        snprintf (buf, sizeof(buf), "<%s>", die->get_name().c_str()) ;
        add_annotation (buf) ;
    }
}

bool OpteronDisassembler::is_frame_pointer (const char *reg) {
    if (reg[1] == 'b' && reg[2] == 'p') {
        return true ;
    }
    return false ;
}

