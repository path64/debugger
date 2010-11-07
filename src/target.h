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

file: target.h
created on: Fri Aug 13 11:02:33 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef target_h_included
#define target_h_included

// debug targets

#include "dbg_types.h"
#include "dbg_elf.h"
#include "pstream.h"
#include <list>
#include <map>
#include <vector>
#include <string>
#include "map_range.h"
#include <sys/procfs.h>
#include <limits.h>
#include "thread.h"

/* find the offset of X into struct user (from sys/user.h) */
/* XXX: change long to Address, after Address is reset to long */
#define STRUCT_USER_OFFSET(X) ((long)&(((struct user*)0)->X))

typedef std::map<std::string,std::string> EnvMap;

class Architecture ;
class RegisterSet;

class Target {
public:
    Target (Architecture *arch) ;
    virtual ~Target(){}

    virtual int attach (const char* prog, const char* args, EnvMap&) = 0;      // attach to a file
    virtual int attach (std::string fn, int pid) = 0 ;                         // attach to a process
    virtual int attach (int pid) = 0 ;                                         // attach to a process id
    // XXX: other attach options?  remote process?

    virtual void detach(int pid, bool kill = true) = 0 ;                                 // detach from target

    void dump (PStream &os, int pid, Address addr, int size) ;                // dump memory
    std::string read_string (int pid, Address addr)  ;            // read a string (nil terminated)
    std::string read_string (int pid, Address addr, int len) ;   // read a string (length terminated)
    virtual void write_string (int pid, Address addr, std::string s) = 0 ;

    virtual void interrupt(int pid) = 0 ; 

    virtual bool test_address (int pid, Address addr) = 0 ;                  // check if address is good
    virtual void write (int pid, Address addr, Address data, int size=4) = 0 ;   // write a word
    virtual Address read (int pid, Address addr, int size=4) = 0 ;           // read a number of words
    virtual Address readptr (int pid, Address addr) = 0 ;
    virtual void get_regs(int pid, RegisterSet *regs) = 0 ;               // get register set
    virtual void set_regs(int pid, RegisterSet *regs) = 0 ;
    virtual void get_fpregs(int pid, RegisterSet *regs) = 0 ;               // get floating point register set
    virtual void set_fpregs(int pid, RegisterSet *regs) = 0 ;
	// FIXME: This should be providing a generic mechanism for setting extra
	// register sets.
    virtual void get_fpxregs(int pid, RegisterSet *regs) = 0 ;               // get floating point extended register set

    virtual void init_events (int pid) = 0 ;
    virtual pid_t get_fork_pid (pid_t pid) = 0 ;

    // factory method to make a new target
    static Target *new_live_target (Architecture *arch) ;
    virtual void cont (int pid, int signal) = 0 ;
    virtual void step(int pid) = 0 ;
    virtual long get_debug_reg (int pid, int reg) = 0 ;
    virtual void set_debug_reg (int pid, int reg, long value) = 0 ;

    virtual void* get_thread_tid (void *agent, void *threadhandle, int &thr_pid) {throw Exception ("Not support") ;}
    virtual void thread_suspend (Thread *) {throw Exception ("Not support") ;}
    virtual void thread_kill (Thread *) {throw Exception ("Not support") ;}
protected:
    Architecture *arch ;
} ;


/**
 * A target talking to a live process.  Concrete subclasses of this exist for
 * each supported platform.  For some platforms, there may be different live
 * targets for different supported architectures.  For example, debugging x86
 * and x86-64 programs typically requires slightly different calls to access
 * the registers of the remote process, although both kinds of program can be
 * run on an x86-64 host.
 *
 * Live targets will also provide the interface to the remote debugger, once
 * this support is implemented.
 */
class LiveTarget : public Target {
protected:
	/**
	 * Live target has a protected constructor.  Use CreateLiveTarget() to
	 * construct an instance of the correct concrete subclass for the current
	 * platform.
	 */
    LiveTarget (Architecture *arch) : Target(arch) {}
public:
	/**
	 * Returns a live target for this platform.
	 */
	static LiveTarget* Create(Architecture *arch);
    ~LiveTarget() {}
    void write_string (int pid, Address addr, std::string s) ;

    virtual void set_fpregs(int pid, RegisterSet *regs) = 0 ;               // set floating point register set
	virtual void set_regs(int pid, RegisterSet *regs) = 0 ;               // set register set
    virtual void set_fpxregs(int pid, RegisterSet *regs) = 0 ;               // set floating point extended register set

    virtual void cont (int pid, int signal) = 0 ;                                // continue execution
    virtual void step(int pid) = 0 ;                                           // single step
	// FIXME: I don't like exposing the debug registers here.  We should be
	// exposing generic functionality, which can be implemented either via the
	// debug registers, in software, or throw a not-supported exception.
    virtual long get_debug_reg (int pid, int reg) = 0 ;
    virtual void set_debug_reg (int pid, int reg, long value) = 0 ;
} ;


// struct CoreThread
// {
//     CoreThread() : id(++nextid) {}
//     int id ;
// 	// FIXME: Target specific.
//     elf_prstatus prstatus ;             // process status
//     RegisterSet *registers;
//
// private:
//     static int nextid;
// } ;

struct CoreThread {
	CoreThread() : id(++nextid) {}
	int	id;
	int	sig;
	int	pid;
	char	*reg;
	static int nextid ;
} ;

class CoreTarget : public Target {
public:
    CoreTarget (Architecture *arch, std::string corefile) ;
    ~CoreTarget() ;

    int attach (const char* prog, const char* args, EnvMap&);    // attach to a file
    int attach (std::string fn, int pid) ;                       // attach to a process
    int attach (int pid) ;                                       // attach to a process id

    void detach(int pid, bool kill = false) ;                    // detach from target
    void interrupt(int pid) ;

    bool test_address (int pid, Address addr) {                  // check if address is good
       return ! regions.get(addr,NULL);
    }

    void write (int pid, Address addr, Address data, int size=4) ;   // write a number of bytes
    Address read (int pid, Address addr, int size=4) ;           // read a number of words
    Address readptr (int pid, Address addr)  ;
    virtual void get_regs(int pid, RegisterSet *regs);               // get register set
    virtual void get_fpregs(int pid, RegisterSet *regs);               // get floating point register set

    // methods particular to this object, not general target
    int get_signal() ;
    int get_terminating_thread() ;
    std::string get_program() ;

    void map_file (ELF *file) ;                 // map segments from file
    void map_code (std::vector<ELF*> &files) ;

    void cont (int pid, int signal)  ;                                // continue execution
    void step(int pid)  ;                                           // single step

    int get_num_threads() ;
    int get_thread_pid (int n) ;
    void *get_thread_tid (int n) ;
    void write_string (int pid, Address addr, std::string s);
    void init_events (int pid);
    pid_t get_fork_pid (pid_t pid);
    void set_regs(int pid, RegisterSet *regs);
    void set_fpregs(int pid, RegisterSet *regs);
    long get_debug_reg (int pid, int reg);
    void set_debug_reg (int pid, int reg, long value);
    void get_fpxregs(int pid, RegisterSet *regs);
    void set_fpxregs(int pid, RegisterSet *regs);

private:
    void read_note (ProgramSegment *note, std::istream &s) ;            // read a set of notes
    void load_segment (ProgramSegment *seg) ;

    typedef std::list<ProgramSegment*> SegmentList ;

    Map_Range<Address,char*> regions;
    SegmentList pending_segments ;     // segments that need loaded
    std::vector<int> open_files ;

    std::string corefile ;
    int fd ;
    ELF *core ;
    std::string pname;

    void new_thread() ;
    CoreThread *find_thread (int pid) ;
    std::vector<CoreThread*> threads ;
    int current_thread ;
} ;

#endif
