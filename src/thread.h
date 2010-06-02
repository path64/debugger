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

file: thread.h
created on: Fri Aug 13 11:02:33 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef thread_h_included
#define thread_h_included

#include "dbg_types.h"

#include "pstream.h"

class Process ;
class Architecture ;

class Thread {
public:
    Thread(Architecture * arch, Process * proc, int pid, void* tid) ;
    ~Thread() ;

    // integer registers
    Address get_reg (std::string name) ;
    Address get_reg (int num) ;                 // get register given DWARF regnum
    void set_reg (std::string name, Address value) ;
    void set_reg (int num, Address value) ;
    void soft_set_reg (int offset, Address value, bool force) ;             // set register but don't commit it (unless force)

    // floating point registers
    Address get_fpreg (std::string name) ;
    Address get_fpreg (int num) ;                 // get register given DWARF regnum
    void set_fpreg (std::string name, Address v) ;
    void soft_set_fpreg (int offset, Address v) ;             // set register but don't commit it

    void syncout () ;
    void syncin () ;
    void print_regs (PStream &os, bool all) ;
    void print_reg (std::string name, PStream &os) ;
    void* get_tid() { return tid ; }
    int get_pid() { return pid ; }
    void set_pid (int p) { pid = p ; }
    int get_num() { return num ; }
    void save_regs (unsigned char *sr, unsigned char *sfpr); 
    void restore_regs (unsigned char *sr, unsigned char *sfpr); 

    bool is_running() { return running ; }
    void stop() { running = false ;}
    void go() { running = true ; }

    void disable() { disabled = true ; }
    void enable() { disabled = false ; }
    bool is_disabled() { return disabled ; }

    void set_stop_status (int s) { status = s ; }
    int get_stop_status() { return status ; }

    void print (PStream &os) ;
    static void reset() ;
protected:
private:
    Architecture * arch ;
    Process * proc ;
    int pid ;
    void * tid ;
    int num ;
    unsigned char * regs ;
    unsigned char * fpregs ;
    bool regs_dirty ;
    bool fpregs_dirty ;
    int regsize ;
    int fpregsize ;

    bool running ;              // thread is running
    bool disabled ;             // thread is disabled
    int status ;
    static int nextid ;
} ;

#endif
