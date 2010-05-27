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
