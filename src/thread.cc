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

file: thread.cc
created on: Fri Aug 13 11:07:48 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "thread.h"
#include "arch.h"
#include "process.h"
#include "pstream.h"

int Thread::nextid = 0 ;

Thread::Thread (Architecture * arch, Process * proc, int pid, void*tid)
    : arch(arch),
    proc(proc),
    pid(pid),
    tid(tid),
    regs_dirty(false),
    fpregs_dirty(false), running(false), disabled(false), status(0) {

    regsize = arch->get_reg_size() ;
    fpregsize = arch->get_fpreg_size() ;
    regs = new unsigned char [arch->get_regbuffer_size()] ;
    fpregs = new unsigned char [arch->get_fpregbuffer_size()] ;
    num = nextid++ ;
}

Thread::~Thread() {
}

Address Thread::get_reg(std::string name) {
    int offset = arch->translate_regname (name) ;
    Address rval = 0 ;
    memcpy (&rval, regs + offset, regsize) ;
    return rval ;
}

Address Thread::get_reg(int num) {
    int offset = arch->translate_regnum (num) ; 
    Address rval = 0 ;
    memcpy (&rval, regs + offset, regsize) ;
    return rval ;
}

void Thread::set_reg(std::string name, Address value) {
    //std::cout << "setting reg %"  <<  name  <<  " to value: "  <<  "0x" << std::hex << value << std::dec << '\n' ;
    int offset = arch->translate_regname (name) ;
    memcpy (regs + offset, &value, regsize) ;
    regs_dirty = true ;
}

void Thread::set_reg(int num, Address value) {
    int offset = arch->translate_regnum (num) ;
    memcpy (regs + offset, &value, regsize) ;
    regs_dirty = true ;
}

void Thread::soft_set_reg(int offset, Address value, bool force) {
    memcpy (regs + offset, &value, regsize) ;
    if (force) {
       regs_dirty = true ;
    }
}


Address Thread::get_fpreg(std::string name) {
    int offset = arch->translate_regname (name) ;
    Address rval = 0 ;
    memcpy (&rval, fpregs + offset, fpregsize) ;
    return rval ;

}

Address Thread::get_fpreg(int num) {
    int offset = arch->translate_regnum (num) ; 
    Address rval = 0 ;
    memcpy (&rval, fpregs + offset, fpregsize) ;
    return rval ;
}

void Thread::set_fpreg(std::string name, Address v) {
    //std::cout << "setting reg %"  <<  name  <<  " to value: "  <<  "0x" << std::hex << value << std::dec << '\n' ;
    int offset = arch->translate_regname (name) ;
    memcpy (fpregs + offset, &v, fpregsize) ;
    fpregs_dirty = true ;
}

void Thread::soft_set_fpreg(int offset, Address v) {
    memcpy (fpregs + offset, &v, fpregsize) ;
}

void Thread::syncout() {
    if (regs_dirty) {
       //std::cout << "setting thread registers" << '\n' ;
       proc->set_regs (regs, tid) ;
       regs_dirty = false ;
    }
    if (fpregs_dirty) {
        proc->set_fpregs (fpregs, tid) ;
        fpregs_dirty = false ;
    }
}

void Thread::syncin() {
    proc->get_regs (regs, tid) ;
    regs_dirty = false ;

#if 0
    unsigned char linebuf [16] ;
    unsigned char *ch = regs ;
    int size = arch->get_regbuffer_size() ;
    while (size > 0) {
        printf ("%08llX ", ch) ;
        for (int i = 0 ; i < 16 ; i ++) { ;
             linebuf[i] = *ch++ ;
        }
        for (int ch = 0 ; ch < 16; ch++) {
            printf ("%02X ", linebuf[ch]) ;
        }
        printf (" ") ;
        for (int i = 0 ; i < 16; i++) {
            unsigned char ch = linebuf[i] ;
            if (ch < 32 || ch >= 127) {
                printf (".") ;
            } else {
                printf ("%c", ch) ;
            }
        }
        printf ("\n") ;
        size -= 16 ;
    }
#endif
    proc->get_fpregs (fpregs, tid) ;
    fpregs_dirty = false ;
}

// XXX: this needs to work on floating point regs too
void Thread::print_regs(PStream &os, bool all) {
    RegnameVec &regnames = arch->get_regnames(all) ;
    for (uint i = 0 ; i < regnames.size(); i++) {
        std::string reg = regnames[i] ;
        Address value = get_reg(reg) ;
        os.print ("%%%s\t0x%016llx %12lld ", reg.c_str(), value, value) ;
        bool one = false ;
        for (int i = 63 ; i >= 0 ; i--) {
            Address v = (value & (1LL << i)) ; 
            if (v != 0 || one) {
                os.print ("%c", v ? '1' : '0') ;
                one = true ;
            }
        }
        if (!one) {
            os.print ("0") ;
        }
        os.print ("\n") ;
    }
}

void Thread::print_reg(std::string name, PStream &os) {
    Address value = get_reg(name) ;
    os.print ("%%%s\t0x%016llx %12lld ", name.c_str(), value, value) ;
    bool one = false ;
    for (int i = 63 ; i >= 0 ; i--) {
        Address v = (value & (1LL << i)) ; 
        if (v != 0 || one) {
            os.print ("%c", v ? '1' : '0') ;
            one = true ;
        }
    }
    if (!one) {
        os.print ("0") ;
    }
    os.print ("\n") ;
}

void Thread::save_regs (unsigned char *sr, unsigned char *sfpr) {
    memcpy (sr, regs, arch->get_regbuffer_size()) ;
    memcpy (sfpr, fpregs, arch->get_fpregbuffer_size()) ;
}

void Thread::restore_regs (unsigned char *sr, unsigned char *sfpr) {
    memcpy (regs, sr, arch->get_regbuffer_size()) ;
    memcpy (fpregs, sfpr, arch->get_fpregbuffer_size()) ;
    regs_dirty = true ;
    fpregs_dirty = true ;
    syncout() ;
}

void Thread::print (PStream &os) {
    os.print ("Thread %lld (LWP %d)", tid, pid) ;
}

void Thread::reset() {
    nextid = 0 ;
}

