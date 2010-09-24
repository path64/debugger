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
#include "register_set.h"

class Process ;
class Target ;

// classes

typedef std::map<std::string, int> RegMap ;
typedef std::vector<std::string> RegnameVec ;
typedef std::map<int, int> RegnumMap ;

typedef std::vector<RegisterSetProperties*> RegisterSetInfoList;
// register types
enum RegisterType
{
	/**
	 * Register stores integer values.
	 */
	RT_INTEGRAL,
	/**
	 * Register stores integer values and is used for addressing.  Contents
	 * should be treated as unsigned.
	 */
	RT_ADDRESS,
	/**
	 * Register stores unsigned values.
	 */
	RT_FLOAT,
	/**
	 * Not a valid register name.
	 */
	RT_INVALID
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

class Architecture
{
public:
	virtual ~Architecture() {}
	/**
	 * Returns whether the architecture is little endian.
	 */
    virtual bool is_little_endian () = 0;

	/**
	 * Returns whether the register indicated by a DWARF register number is a
	 * floating point register.
	 */
	bool isfpreg(int dwarfnum);
	/**
	 * Finds the type of the register.
	 */
	RegisterType type_of_register(int dwarfnum);
	/**
	 * Finds the type of the register for a given name.
	 */
	RegisterType type_of_register(const std::string &name);
	/**
	 * Returns the size of the register, in bytes.  Returns -1 if the named
	 * register does not exist.
	 */
	int size_of_register(const std::string &name);
	/**
	 * Determines whether the named register contains an address or an integer
	 * value.  When called with the name of a floating point register, the
	 * result is undefined.
	 */
    RegisterType register_type(std::string reg);

	/**
	 * Returns a vector containing all of the register sets available on this
	 * architecture.  Most architectures will have two - a main and a floating
	 * point register set.  Some may omit the floating point set, others may
	 * have special-purpose sets for vector units or similar.
	 */
	virtual RegisterSetInfoList& register_properties() const = 0;
	/**
	 * Returns the properties of the main register set for this architecture.
	 */
	virtual RegisterSetProperties *main_register_set_properties() = 0;
	/**
	 * Returns the properties for the floating point register set for this
	 * architecture, or NULL if this architecture has no FPU.
	 */
	virtual RegisterSetProperties *fpu_register_set_properties()
		{ return NULL; };

	/**
	 * Returns the total number of registers.  This is calculated in the
	 * superclass by summing the total in declared in the register set
	 * properties.  Subclasses may provide a more efficient implementation.
	 */
	virtual int total_number_of_regs() const
	{
		RegisterSetInfoList &sets = register_properties();
		int total = 0;
		for (RegisterSetInfoList::iterator i=sets.begin(), e=sets.end() ; 
		     i!=e ; i++)
		{
			total += (*i)->number_of_registers();
		}
		return total;
	}
	// FIXME: We shouldn't use this, we should map from DWARF numbers to
	// {register set, register index} pairs
	int translate_regnum(int dwarfnum)
	{
		return main_register_set_properties()->register_number_for_dwarf_number(dwarfnum);
	}
	int translate_regname(const std::string &name)
	{
		return main_register_set_properties()->register_number_for_name(name);
	}
	std::string reverse_translate_regnum(int num)
	{
		return main_register_set_properties()->name_for_register_number(num);
	}
	/**
	 * Sets a breakpoint at a specific address and returns the old value at
	 * that address.  This is used for software breakpoints, which are
	 * implemented by replacing the specified instruction with something that
	 * will cause a trap, then replacing it, single-stepping over the
	 * instruction, and then continuing.
	 */
	virtual int set_breakpoint(Process * proc, Address addr) = 0;
	/**
	 * Tests whether there is a software breakpoint present at the specified
	 * address.
	 */
	virtual bool breakpoint_present(Process *proc, Address addr) = 0 ;

	/**
	 * Returns the size of a breakpoint.  This is the size of the value that is
	 * written over an instruction to create a software breakpoint.  
	 */
    virtual int bpsize () = 0;
	/**
	 * Size of a pointer for this architecture.
	 */
    virtual int ptrsize() = 0;
	/**
	 * Returns whether the program counter is inside frame marker instructions.
	 */
    virtual bool inframemaker(Process * proc, Address pc) = 0;
	/**
	 * Find the frame above the current frame on the stack.
	 */
    virtual void guess_frame (Process *proc, Frame* oframe, Frame* nframe) = 0;
	/**
	 * Returns whether the address is a call instruction.
	 */
    virtual bool is_call (Process * proc, Address addr)=0;
	/**
	 * Returns the size of the call instruction at the specified address.
	 */
    virtual int call_size (Process * proc, Address addr) = 0;
	/**
	 * Returns the maximum size of a call instruction.
	 */
    virtual int max_call_size() = 0 ;
	/**
	 * Returns the minimum size of a call instruction.
	 */
    virtual int min_call_size() = 0 ;
    virtual Address skip_preamble (Process * proc, Address addr) = 0 ;                    // address after preamble
    virtual int frame_base_reg() = 0 ;                  // what is the frame base register
    virtual int disassemble (PStream &os, Process *proc, Address addr) = 0 ;
    virtual std::string get_return_reg(int n=1) = 0 ;
    virtual std::string get_return_fpreg(int n=1) = 0 ;
    virtual void write_call_arg (Process *proc, int argnum, Address value, bool isfp=false) = 0 ;
    virtual void write_call_arg (Process *proc, int argnum, const void *value, int size) = 0 ;
    virtual Address stack_space (Process *proc, int bytes) = 0 ;           // allocate some stack space
    virtual Address write_call (Process *proc, Address addr, std::string &buffer) = 0 ;
    virtual void align_stack (Process *proc) = 0 ;

    // signal trampolines
    virtual bool in_sigtramp (Process *proc, std::string name) = 0 ;
    virtual void get_sigcontext_frame(Process *proc, Address sp, RegisterSet *regs) = 0;

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
    virtual void get_fpregs(void *agent, void* tid, int pid, Target *target, RegisterSet *regs);
    virtual void set_fpregs(void *agent, void* tid, int pid, Target *target, RegisterSet *regs);
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
	virtual RegisterSetInfoList& register_properties() const;
	virtual RegisterSetProperties *main_register_set_properties();
	virtual RegisterSetProperties *fpu_register_set_properties();
    RegnameVec &get_regnames (bool all) ;
    int bpsize () ;
    int get_num_fpregs () ;
    int get_reg_size()  ;
    int get_fpreg_size()  ;
    int regnum2offset(int rnum) ;
    int fpregnum2offset(int rnum) ;
    bool isfpreg (int dwarfnum) ;
    int translate_regname (std::string name) ;
    std::string reverse_translate_regnum (int num) ;
    int translate_fpregname (std::string name) ;
    int set_breakpoint (Process* proc, Address addr) ;
    bool breakpoint_present (Process *proc, Address addr) ;
    int ptrsize () ;
    bool inframemaker (Process* proc, Address pc) ;
    Address skip_preamble (Process * proc, Address addr)  ;                    // address after preamble
    int frame_base_reg() ;                  // what is the frame base register
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
    virtual void get_sigcontext_frame(Process *proc, Address sp, RegisterSet *regs);
    bool is_64bit() { return false ; }
    bool is_small_struct(int size) { return false ; }
    virtual RegisterSet *get_fpregs(void *agent, void* tid, int pid, Target *target);
    virtual void set_fpregs(void *agent, void* tid, int pid, Target *target, RegisterSet *regs);
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
	virtual RegisterSetInfoList& register_properties() const;
	virtual RegisterSetProperties *main_register_set_properties();
	virtual RegisterSetProperties *fpu_register_set_properties();
    int bpsize () ;
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
    virtual void get_sigcontext_frame(Process *proc, Address sp, RegisterSet *regs);
    bool is_64bit() { return mode == 64 ; }
    virtual RegisterSet *get_fpregs(void *agent, void* tid, int pid, Target *target);
    virtual void set_fpregs(void *agent, void* tid, int pid, Target *target, RegisterSet *regs);
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
