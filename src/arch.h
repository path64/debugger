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

file: arch.h
created on: Fri Aug 13 11:02:25 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef arch_h_included
#define arch_h_included

#include "pstream.h"
#include "dbg_types.h"
#include "dis.h"

#include "breakpoint.h"

class Process ;
class Target ;

// classes

typedef std::map<std::string, int> RegMap ;
typedef std::vector<std::string> RegnameVec ;
typedef std::map<int, int> RegnumMap ;

// register types
enum RegisterType {
    RT_INTEGRAL, RT_ADDRESS
} ;

class EvalContext;

// register buffers:
//   the machine registers are stored in a buffer that is large enough to hold
//   the result of ptrace(PTRACE_GETREGS).  They are held in 'native' order
//   and translations are provided to convert between names/dwarf numbers to
//   offsets into the buffer.  Clients of this class should use the
//   result of the translations as offsets into their own buffer.  There is
//   no 'register numbering' scheme as such, a register number being an offset
//   into the buffer.  The exception to this is the regnum2offset() function that
//   provides the client with a way to iterate through all the registers and
//   translate a pseudo register number into an offset.

class Architecture {
public:
    virtual ~Architecture() {}
    virtual int get_regbuffer_size() = 0 ;                 // size of the buffer for registers
    virtual int get_fpregbuffer_size() = 0 ;              // size of the buffer for fp registers
    virtual int get_num_regs () = 0 ;                      // get the number of registers
    virtual int get_num_fpregs () = 0 ;                    // get the number of floating point registers
    virtual int get_reg_size() = 0 ;
    virtual int get_fpreg_size() = 0 ;
    virtual int regnum2offset(int rnum) = 0 ;
    virtual int fpregnum2offset(int rnum) = 0 ;
    virtual bool isfpreg (int dwarfnum) = 0 ;
    virtual int translate_regname (std::string name) = 0 ;            // translate a name into an offset into the reg buffer
    virtual int translate_fpregname (std::string name) = 0 ;          // translate a name into an offset into the  floating point buffer 
    virtual int translate_regnum (int dwarfnum) = 0 ;
    virtual std::string reverse_translate_regnum (int num) = 0 ;
    virtual RegnameVec &get_regnames (bool all) = 0 ;                      // get a vector of register names
    virtual int set_breakpoint (Process * proc, Address addr) = 0 ;               // set breakpoint and return old value
    virtual bool breakpoint_present (Process *proc, Address addr) = 0 ;
    virtual int bpsize () = 0 ;                            // size of a breakpoint
    virtual int ptrsize () = 0 ;                           // size of a pointer
    virtual bool inframemaker (Process * proc, Address pc) = 0 ;                      // is the pc inside the frame maker instructions
    virtual void guess_frame (Process *proc, Frame* oframe, Frame* nframe) = 0;
    virtual bool is_little_endian () = 0 ;                  // is the architecture little endian
    virtual bool is_call (Process * proc, Address addr) = 0 ;                 // does the address contain a call instruction
    virtual int call_size (Process * proc, Address addr) = 0 ;               // size of a call instruction
    virtual int max_call_size() = 0 ;
    virtual int min_call_size() = 0 ;
    virtual Address skip_preamble (Process * proc, Address addr) = 0 ;                    // address after preamble
    virtual int frame_base_reg() = 0 ;                  // what is the frame base register
    virtual int disassemble (PStream &os, Process *proc, Address addr) = 0 ;
    virtual RegisterType get_register_type (std::string reg) = 0 ;
    virtual std::string get_return_reg(int n=1) = 0 ;
    virtual std::string get_return_fpreg(int n=1) = 0 ;
    virtual void write_call_arg (Process *proc, int argnum, Address value, bool isfp=false) = 0 ;
    virtual void write_call_arg (Process *proc, int argnum, const void *value, int size) = 0 ;
    virtual Address stack_space (Process *proc, int bytes) = 0 ;           // allocate some stack space
    virtual Address write_call (Process *proc, Address addr, std::string &buffer) = 0 ;
    virtual void align_stack (Process *proc) = 0 ;

    // signal trampolines
    virtual bool in_sigtramp (Process *proc, std::string name) = 0 ;
    virtual void get_sigcontext_frame (Process *proc, Address sp, unsigned char *regs) = 0 ;

    // hardware breakpoint/watchpoint 
    virtual int allocate_debug_reg(Address addr) = 0 ;
    virtual void free_debug_reg (int reg) = 0 ; 
    virtual void set_watchpoint (Process *proc, int reg, WatchpointType type, int size) = 0 ;
    virtual void clear_watchpoint (Process *proc, int reg) = 0 ;
    virtual int get_debug_status(Process *proc) = 0 ;
    virtual void clear_all_watchpoints(Process *proc ) = 0 ;
    virtual Address get_debug_reg (Process *proc,int reg) = 0 ;
    virtual int get_available_debug_regs() = 0 ;                // number of debug registers available
    virtual bool is_64bit() = 0 ;
    virtual bool is_small_struct(int size) = 0 ;
    virtual void get_fpregs (void *agent, void* tid, int pid, Target *target, unsigned char *regs) = 0 ;
    virtual void set_fpregs (void *agent, void* tid, int pid, Target *target, unsigned char *regs) = 0 ;
    virtual int classify_struct (EvalContext &ctx, DIE *s) = 0 ;

protected:
    Disassembler *disassembler ;
} ;

class IntelArch : public Architecture {
public:
    IntelArch(int num_debug_regs) ;
    ~IntelArch() ;
    int allocate_debug_reg(Address addr) ;
    void free_debug_reg (int reg) ; 
    void set_watchpoint (Process *proc, int reg, WatchpointType type, int size) ;
    void clear_watchpoint (Process *proc, int reg) ;
    int get_debug_status(Process *proc) ;
    void clear_all_watchpoints(Process *proc) ;
    int get_available_debug_regs() ;                // number of debug registers available
    Address get_debug_reg (Process *proc,int reg) ;
    void guess_frame (Process *proc, Frame* oframe, Frame* nframe);
    Address stack_space (Process *proc, int bytes) ;           // allocate some stack space
    bool is_little_endian () ;
    void align_stack (Process *proc) ;
private:

    // debug register stuff
    int num_debug_regs ;
    Address *debug_regs ;                       // values of the debug registers
    int *refcounts ;                            // reference counts for the debug registers
    unsigned int status ;                       // status register
    unsigned int control ;                      // control register
    bool regs_valid ;
    static const int DR_STATUS = 6 ;
    static const int DR_CONTROL = 7 ;

    static const int DR_RW_EXECUTE = 0 ;
    static const int DR_RW_WRITE = 1 ;
    static const int DR_RW_READ = 3 ;

} ;

class i386Arch: public IntelArch {
public:
    i386Arch() ;
    ~i386Arch() ; 
    RegnameVec &get_regnames (bool all) ;
    int bpsize () ;
    int get_regbuffer_size() ;                 // size of the buffer for registers
    int get_fpregbuffer_size() ;              // size of the buffer for fp registers
    int get_num_regs () ;
    int get_num_fpregs () ;
    int get_reg_size()  ;
    int get_fpreg_size()  ;
    int regnum2offset(int rnum) ;
    int fpregnum2offset(int rnum) ;
    bool isfpreg (int dwarfnum) ;
    int translate_regname (std::string name) ;
    int translate_regnum (int dwarfnum) ;
    std::string reverse_translate_regnum (int num) ;
    int translate_fpregname (std::string name) ;
    int set_breakpoint (Process* proc, Address addr) ;
    bool breakpoint_present (Process *proc, Address addr) ;
    int ptrsize () ;
    bool inframemaker (Process* proc, Address pc) ;
    Address skip_preamble (Process * proc, Address addr)  ;                    // address after preamble
    int frame_base_reg() ;                  // what is the frame base register
    RegisterType get_register_type (std::string reg)  ;
    std::string get_return_reg(int n=1)  ;
    std::string get_return_fpreg(int n=1)  ;
    bool is_call (Process* proc, Address addr) ;
    int call_size (Process* proc, Address addr) ;
    int max_call_size() { return 7 ; } 
    int min_call_size() { return 2 ; }
    void write_call_arg (Process *proc, int argnum, Address value, bool isfp=false) ;
    void write_call_arg (Process *proc, int argnum, const void *value, int size) ;
    Address write_call (Process *proc, Address addr, std::string &buffer) ;
    bool in_sigtramp (Process *proc, std::string name) ;
    void get_sigcontext_frame (Process *proc, Address sp, unsigned char *regs) ;
    bool is_64bit() { return false ; }
    bool is_small_struct(int size) { return false ; }
    void get_fpregs (void *agent, void* tid, int pid, Target *target, unsigned char *regs) ;
    void set_fpregs (void *agent, void* tid, int pid, Target *target, unsigned char *regs) ;
    int classify_struct (EvalContext &ctx, DIE *s) ;        
protected:
private:
    RegMap regnames ; 
    RegnameVec allnames ; 
    RegnameVec commonnames ; 
    RegnumMap regnums ; // mapping for DWARF register numbers to the internal form

    int disassemble (PStream &os, Process *proc, Address addr) ;
} ;

enum x86_64ArgClass {
    X8664_AC_MEMORY, X8664_AC_NO_CLASS, X8664_AC_INTEGER, X8664_AC_SSE, X8664_AC_SSEUP, X8664_AC_X87, X7664_AC_X87UP
} ;

class x86_64Arch: public IntelArch {            // AMD will be upset it's called Intel!
public:
    x86_64Arch(int mode) ;
    ~x86_64Arch() ; 
    RegnameVec &get_regnames (bool all) ;
    int bpsize () ;
    int get_regbuffer_size() ;                 // size of the buffer for registers
    int get_fpregbuffer_size() ;              // size of the buffer for fp registers
    int get_num_regs () ;
    int get_num_fpregs () ;
    int get_reg_size()  ;
    int get_fpreg_size()  ;
    int regnum2offset(int rnum) ;
    int fpregnum2offset(int rnum) ;
    bool isfpreg (int dwarfnum) ;
    int translate_regname (std::string name) ;
    int translate_regnum (int dwarfnum) ;
    std::string reverse_translate_regnum (int num) ;
    int translate_fpregname (std::string name) ;
    int set_breakpoint (Process* proc, Address addr) ;
    bool breakpoint_present (Process *proc, Address addr) ;
    int ptrsize () ;
    bool inframemaker (Process* proc, Address pc) ;
    Address skip_preamble (Process * proc, Address addr)  ;                    // address after preamble
    int frame_base_reg() ;                  // what is the frame base register
    RegisterType get_register_type (std::string reg)  ;
    bool is_call (Process* proc, Address addr) ;
    int call_size (Process* proc, Address addr) ;
    int max_call_size() { return 8 ; } 
    int min_call_size() { return 2 ; }
    std::string get_return_reg(int n=1)  ;
    std::string get_return_fpreg(int n=1)  ;
    void write_call_arg (Process *proc, int argnum, Address value, bool isfp=false) ;
    void write_call_arg (Process *proc, int argnum, const void *value, int size) ;
    Address write_call (Process *proc, Address addr, std::string &buffer) ;
    bool in_sigtramp (Process *proc, std::string name) ;
    void get_sigcontext_frame (Process *proc, Address sp, unsigned char *regs) ;
    bool is_64bit() { return mode == 64 ; }
    void get_fpregs (void *agent, void* tid, int pid, Target *target, unsigned char *regs) ;
    void set_fpregs (void *agent, void* tid, int pid, Target *target, unsigned char *regs) ;
    bool is_small_struct(int size) { return size <= 16 ; }
    int classify_struct (EvalContext &ctx, DIE *s) ;        

protected:
private:
    RegMap regnames ; 
    RegnameVec allnames ; 
    RegnameVec commonnames ; 
    RegnumMap regnums ; // mapping for DWARF register numbers to the internal form
    int mode ;                  // 32 or 64

    int disassemble (PStream &os, Process *proc, Address addr) ;
} ;


// functions

#endif
