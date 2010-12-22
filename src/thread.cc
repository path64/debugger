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

Thread::Thread (Architecture * arch, OS *osc, Process * proc, int pid, void*tid)
    : arch(arch), osc(osc),
    proc(proc),
    pid(pid),
    tid(tid),
    running(false), disabled(false), status(0) {

    regs = arch->main_register_set_properties()->new_empty_register_set();
    fpregs = arch->fpu_register_set_properties()->new_empty_register_set();
    num = nextid++;
}

Thread::~Thread() {
}

Address Thread::get_reg(const std::string &name)
{
	return regs->get_register_as_integer(name);
}

Address Thread::get_reg(int num)
{
	// FIXME: Is num meant to be the dwarf register num here?
	return regs->get_register_as_integer(num);
}

void Thread::set_reg(const std::string &name, Address value)
{
	regs->set_register(name, value);
}

// FIXME: Is num meant to be the dwarf register num here?  If so, we should rename this function
void Thread::set_reg(int num, Address value)
{
	regs->set_register(num, value);
}


double Thread::get_fpreg(const std::string &name)
{
	return regs->get_register_as_integer(name);
}

double Thread::get_fpreg(int num)
{
	return regs->get_register_as_integer(num);
}

void Thread::set_fpreg(const std::string &name, double v)
{
	fpregs->set_register(name, v);
}

void Thread::syncout()
{
	// FIXME: Allow other register sets.
	if (regs->is_dirty())
	{
		proc->set_regs(regs, tid) ;
		regs->clear_dirty_flag();
	}
	if (fpregs->is_dirty())
	{
		proc->set_fpregs(fpregs, tid) ;
		fpregs->clear_dirty_flag();
	}
}

void Thread::syncin()
{
	proc->get_regs(regs, tid) ;
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
	proc->get_fpregs(fpregs, tid) ;
	regs->clear_dirty_flag();
	fpregs->clear_dirty_flag();
}

void Thread::print_regs(PStream &os, bool all)
{
	regs->print(os);
	fpregs->print(os);
}

void Thread::print_reg(const std::string &name, PStream &os) {
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

void Thread::save_regs(RegisterSet *sr, RegisterSet *sfpr)
{
	sr->take_values_from(regs);
	sfpr->take_values_from(fpregs);
}

void Thread::restore_regs(RegisterSet *sr, RegisterSet *sfpr)
{
	regs->take_values_from(sr);
	fpregs->take_values_from(sfpr);
    syncout() ;
}

void Thread::print (PStream &os) {
    os.print ("Thread %lld (LWP %d)", tid, pid) ;
}

void Thread::reset() {
    nextid = 0 ;
}

void Thread::soft_set_regs(RegisterSet *r, bool force) {
	if (force || r->is_dirty())
	{
		proc->get_regs(r, tid) ;
		r->clear_dirty_flag();
	}
}

void Thread::soft_set_fp_regs(RegisterSet *r, bool force) {
	if (force || r->is_dirty())
	{
		proc->get_fpregs(r, tid) ;
		r->clear_dirty_flag();
	}
}
