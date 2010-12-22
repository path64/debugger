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
#include "register_set.h"

#include "pstream.h"
#include "os.h"

class Process ;
class Architecture ;

class Thread {
public:
    Thread(Architecture * arch, Process * proc, int pid, void* tid) ;
    ~Thread() ;

    // integer registers
    Address get_reg (const std::string &name) ;
    Address get_reg (int num) ;                 // get register given DWARF regnum
	void set_regs(RegisterSet *r);
	void set_fp_regs(RegisterSet *r);
	void soft_set_regs(RegisterSet *r, bool force=false);
	void soft_set_fp_regs(RegisterSet *r, bool force=false);

    void set_reg (const std::string &name, Address value) ;
    void set_reg (int num, Address value) ;

    // floating point registers
    double get_fpreg(const std::string &name);
    double get_fpreg(int num);                 // get register given DWARF regnum
    void set_fpreg(const std::string &name, double v);

    void syncout () ;
    void syncin () ;
    void print_regs (PStream &os, bool all) ;
    void print_reg (const std::string &name, PStream &os) ;
    void* get_tid() { return tid ; }
    int get_pid() { return pid ; }
    void set_pid (int p) { pid = p ; }
    int get_num() { return num ; }
	// FIXME: These should take something that allows more than two register
	// sets (e.g. vector registers)
    void save_regs(RegisterSet *main, RegisterSet *fpu);
    void restore_regs(RegisterSet *main, RegisterSet *fpu); 

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
	Architecture * arch;
	Process * proc;
	int pid;
	void * tid;
	int num;
	RegisterSet *regs;
	RegisterSet *fpregs;

	bool running;              // thread is running
	bool disabled;             // thread is disabled
	int status;
	static int nextid;
};

#endif
