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
#include <thread_db.h>
#include <sys/procfs.h>         // for the notes
#include <list>
#include <map>
#include <vector>
#include <string>

#include "map_range.h"

typedef std::map<std::string,std::string> EnvMap;

class Architecture ;

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
    virtual void get_regs (int pid, unsigned char *regs) = 0 ;               // get register set
    virtual void set_regs (int pid, unsigned char *regs) = 0 ;               // set register set
    virtual void get_fpregs (int pid, unsigned char *regs) = 0 ;               // get floating point register set
    virtual void set_fpregs (int pid, unsigned char *regs) = 0 ;               // set floating point register set
    virtual void get_fpxregs (int pid, unsigned char *regs) = 0 ;               // get floating point extended register set
    virtual void set_fpxregs (int pid, unsigned char *regs) = 0 ;               // set floating point extended register set

    virtual void cont (int pid, int signal) = 0 ;                                // continue execution
    virtual void step(int pid) = 0 ;                                           // single step
    virtual long get_debug_reg (int pid, int reg) = 0 ;
    virtual void set_debug_reg (int pid, int reg, long value) = 0 ;

    virtual void init_events (int pid) = 0 ;
    virtual pid_t get_fork_pid (pid_t pid) = 0 ;

    // factory method to make a new target
    static Target *new_live_target (Architecture *arch) ;
protected:
    Architecture *arch ;
} ;


// a target talking to a live process
class LiveTarget : public Target {
public:
    LiveTarget (Architecture *arch) : Target(arch) {}
    ~LiveTarget() {}
    void write_string (int pid, Address addr, std::string s) ;
} ;

#if 0
// these might be defined in a system header someday
#ifndef PTRACE_EVENT_FORK
                                                                                                                                  
#define PTRACE_SETOPTIONS       (__ptrace_request)0x4200
#define PTRACE_GETEVENTMSG      (__ptrace_request)0x4201
                                                                                                                                  
// option bits
#define PTRACE_O_TRACESYSGOOD   0x01
#define PTRACE_O_TRACEFORK      0x02
#define PTRACE_O_TRACEVFORK     0x04
#define PTRACE_O_TRACECLONE     0x08
#define PTRACE_O_TRACEEXEC      0x10
#define PTRACE_O_TRACEVFORKDONE 0x20
#define PTRACE_O_TRACEEXIT      0x40
                                                                                                                                  
// event codes
#define PTRACE_EVENT_FORK       1
#define PTRACE_EVENT_VFORK      2
#define PTRACE_EVENT_CLONE      3
#define PTRACE_EVENT_EXEC       4
#define PTRACE_EVENT_VFORKDONE  5
#define PTRACE_EVENT_EXIT       6
                                                                                                                                  
#endif
#endif


// a live target that uses ptrace to control the process
class PtraceTarget : public LiveTarget {
public:
    PtraceTarget (Architecture *arch) ;
    ~PtraceTarget() {}

    int attach (const char* prog, const char* args, EnvMap&);      // attach to a file
    int attach (std::string fn, int pid) ;                         // attach to a process
    int attach (int pid) ;                                         // attach to a process id

    void detach(int pid, bool kill = false) ;                                 // detach from target

    void interrupt(int pid) ;
    bool test_address (int pid, Address addr) ;                  // check if address is good
    Address read (int pid, Address addr, int size=4) ;           // read a number of words
    Address readptr (int pid, Address addr)  ;
    void write (int pid, Address addr, Address data, int size) ;    // write a word
    void get_regs (int pid, unsigned char *regs) ;               // get register set
    void set_regs (int pid, unsigned char *regs) ;               // set register set
    void get_fpregs (int pid, unsigned char *regs) ;               // get register set
    void set_fpregs (int pid, unsigned char *regs) ;               // set register set
    void get_fpxregs (int pid, unsigned char *regs) ;               // get register set
    void set_fpxregs (int pid, unsigned char *regs) ;               // set register set
    long get_debug_reg (int pid, int reg) ;
    void set_debug_reg (int pid, int reg, long value) ;

    void step(int pid)  ;                                           // single step
    void cont (int pid, int signal)  ;                              // continue execution
    void init_events (int pid) ;
    pid_t get_fork_pid (pid_t pid) ;
private:

    pid_t pid;
    bool is_attached ;
} ;

struct CoreThread {
    CoreThread() : id(++nextid) {}

    int id ;
    elf_prstatus prstatus ;             // process status

#if __WORDSIZE == 64
    struct user_fpregs_struct fpregset ;           // floating point register set and extended ones too
#else
    struct user_fpregs_struct fpregset ;           // floating point register set
    struct user_fpxregs_struct fpxregset ;        // extended floating point register set
#endif
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
    void get_regs (int pid, unsigned char *regs) ;               // get register set
    void set_regs (int pid, unsigned char *regs) ;               // set register set
    void get_fpregs (int pid, unsigned char *regs) ;               // get register set
    void set_fpregs (int pid, unsigned char *regs) ;               // set register set
    void get_fpxregs (int pid, unsigned char *regs) ;               // get register set
    void set_fpxregs (int pid, unsigned char *regs) ;               // set register set

    /* XXX: shouldn't be able to set regs either */

    /* core files have no registers or events */
    void init_events (int pid) {
        throw Exception ("No events in core file");
    }
    pid_t get_fork_pid (pid_t pid) {
        throw Exception ("No events in core file");
    }
    void write_string (int pid, Address addr, std::string s) {
        throw Exception ("Cannot write to core file");
    }
    long get_debug_reg (int pid, int reg) {
        throw Exception ("Cannot get debug registers on a core file");
    }
    void set_debug_reg (int pid, int reg, long value) {
        throw Exception ("Cannot set debug registers on a core file");
    }

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
    elf_prpsinfo prpsinfo ;             // process info

    void new_thread() ;
    CoreThread *find_thread (int pid) ;
    std::vector<CoreThread*> threads ;
    int current_thread ;
} ;

#endif
