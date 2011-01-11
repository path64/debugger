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

file: process.cc
created on: Fri Aug 13 11:07:44 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "err_nice.h"
#include "process.h"
#include "arch.h"
#include "thread.h"
#include "type_struct.h"
#include "utils.h"
#include "symtab.h"
#include "breakpoint.h"
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "dbg_thread_db.h"
#include "dwf_cfa.h"
#include "dwf_cunit.h"
#include <signal.h>
#include "target.h"
#include "junk_stream.h"
#include "pcm.h"
#include "cli.h"
#include "dbg_proc_service.h"
#include <sys/stat.h>
#include "trace.h"

#if defined (__linux__)
#define WAITPID_ALL_CHILD_TYPES __WALL
#elif defined (__FreeBSD__)
#define WAITPID_ALL_CHILD_TYPES 0
#endif

// Linux doesn't distinguish between threads and processes, but other kernels
// do so we don't need special handling.
#ifndef __WALL
#	define __WALL 0
#endif
// DW_CFA_offset_extended_sf is an extension, and may not be defined in the
// system's libdwarf
#ifndef DW_CFA_offset_extended_sf
#	define DW_CFA_offset_extended_sf 0x11
#endif

static std::string toString (int n) {
    char buf[30] ;
    snprintf (buf, sizeof(buf), "%d", n) ;
    return buf ;
}

// need to make sure that the symbols in dbg_proc_service.cc are linked in.  

// this will never be called.  It's just to fool the linker into linking the file from the archive
static void force_ps_link() {
    ps_pglobal_lookup(NULL, 0, 0, NULL) ;             
}

ObjectFile::ObjectFile (std::string name, ELF * elf, std::istream & stream, SymbolTable * symtab)
    : name(name),
    elf(elf),
    stream(stream),
    symtab(symtab) {
    (void) force_ps_link;
    (void) toString;
}
                                                                                                                                           
ObjectFile::~ObjectFile() {
    delete elf ;
    delete symtab ;
}

LinkMap::LinkMap (Architecture *arch, Process *proc, Address addr)
    : addr(addr) {

    ps = arch->ptrsize() ;
    base = proc->readptr (addr) ;
    nameaddr = proc->readptr (addr+ps) ;
    if (proc->test_address (nameaddr)) {
        name = proc->read_string (nameaddr) ;
    }
    ld = proc->readptr(addr+ps+ps) ;
    next = proc->readptr(addr+ps+ps+ps) ;
}
                                                                                                                                           
LinkMap::~LinkMap() {
}
                                                                                                                                           
Address LinkMap::get_next() {
        return next ;
}
                                                                                                                                           
void LinkMap::print() {
  printf ("0x%llx %s %llx\n",
	  (unsigned long long) base,
	  name.c_str(),
	  (unsigned long long) ld) ;
}
                                                                                                                                           
std::string LinkMap::get_name() {
       return name ;
}
                                                                                                                                           
Address LinkMap::get_base() {
       return base ;
}
                                                                                                                                           
Address LinkMap::get_addr() {
        return addr ;
}


Frame::Frame (Process *proc, Architecture *arch, int n, Location &loc, Address pc, Address sp, Address fp)
{
	init(proc, arch, n);
	set_pc(pc);
	set_sp(sp);
	set_fp(fp);
	set_loc(loc);
}

Frame::Frame (Process *proc, Architecture *arch, int n)
{
	init (proc, arch, n);
}


void Frame::init (Process* _proc, Architecture* _arch, int _n) {
    /* absorb arguments */
    proc = _proc;
    arch = _arch;
    n = _n;

    /* init others */
    valid = false;
    return_addr = 0;

    regs = arch->main_register_set_properties()->new_empty_register_set();
    fp_regs = arch->fpu_register_set_properties()->new_empty_register_set();
}

Frame::~Frame() {
	// FIXME: Delete register sets.
}

void Frame::set_pc(Address addr)
{
	int num = regs->get_properties()->register_number_for_name("pc");
	regs->set_register(num, addr);
}

void Frame::set_sp(Address addr)
{
	int num = regs->get_properties()->register_number_for_name("sp");
	regs->set_register(num, addr);
}
void Frame::set_fp(Address addr) 
{
	int num = regs->get_properties()->register_number_for_name("fp");
	regs->set_register(num, addr);
}

void Frame::set_loc (Location &l) {
    loc = l ;
}

Location & Frame::get_loc() {
        return loc ;
}

void Frame::print(PStream &os, bool indent, bool current) {
    if (indent) {
        if (current) {
            os.print ("=>") ;
        } else {
            os.print ("  ") ;
        }
    }
    os.print ("#%d  ", n ) ;
    proc->print_loc(loc, this, os) ;
}

void Frame::set_reg(int reg, Address value)
{
	regs->set_register(reg, value);
}

void Frame::set_fpreg(int reg, double value)
{
	fp_regs->set_register(reg, value);
}

void Frame::set_ra(Address value)
{
	return_addr = value ;
}

Address Frame::get_reg(int reg)
{
	return regs->get_register_as_integer(reg);
}

double Frame::get_fpreg(int reg)
{
	return fp_regs->get_register_as_double(reg);
}

Address Frame::get_ra()
{
	return return_addr;
}

void Frame::publish_regs(Thread * thr, bool force)
{
	if (regs->is_dirty())
	{
		thr->soft_set_regs(regs, force);
	}
	if (fp_regs->is_dirty())
	{
		thr->soft_set_fp_regs(fp_regs, force);
	}
}

void Frame::set_valid()
{
	valid = true ;
}

bool Frame::is_valid()
{
	return valid ;
}

Address Frame::get_pc()
{
	int num = regs->get_properties()->register_number_for_name("pc");
	return regs->get_register_as_integer(num);
}

Address Frame::get_sp()
{
	int num = regs->get_properties()->register_number_for_name("sp");
	return regs->get_register_as_integer(num);
}

Address Frame::get_fp()
{
	int num = regs->get_properties()->register_number_for_name("fp");
	return regs->get_register_as_integer(num);
}

#if 0
BadFrame::BadFrame (Process *proc, Architecture *arch, int n, Location& loc, Address pc, Address fp, Address sp) : Frame (proc, arch, n, loc, pc, fp, sp) {
}


BadFrame::~BadFrame() {
}

void BadFrame::print(PStream &os) {
    os.print ("Corrupted stack frame at address fp=0x%llx, sp=0x%llx\n", fp, sp) ;
}
#endif


Rule::Rule (CFARuleType type, int reg, int offset)
    : type(type),
    reg(reg),
    offset(offset) {
}

Rule::~Rule() {
}

std::string Rule::toString()
{
   char buf[32] ;
   switch (type) {
   case CFA_UNDEFINED: ;
       return "u" ;
   case CFA_SAME_VALUE: ;
       return "s" ;
   case CFA_OFFSET: 
       snprintf (buf, sizeof(buf), "c+%d", offset) ;
       return buf ;
   case CFA_REGISTER: ;
       snprintf (buf, sizeof(buf), "r%d", offset) ;
       return buf ;
   case CFA_CFA: ;
       snprintf (buf, sizeof(buf), "[r%d]+%d", reg, offset) ;
       return buf ;
   }
   throw Exception("not reached");
}

CFATable::CFATable (Architecture *arch, Process *proc, Address loc, int ra_reg)
    : arch(arch), proc(proc), loc(loc),
    ra_reg(ra_reg),
    cfa(new Rule (CFA_CFA, 0, 0)),
    ra(new Rule (CFA_UNDEFINED, 0, 0)), saved_regs(NULL), saved_ra(NULL) {

    regs = new Rule * [arch->total_number_of_regs()] ;
    for (int i = 0 ; i < arch->total_number_of_regs(); i++) {
        regs[i] = new Rule (CFA_UNDEFINED, i, 0) ;
    }
}

CFATable::~CFATable() {
    delete [] regs ;
    delete cfa ;
    delete ra ;
    delete [] saved_regs ;
    delete saved_ra ;
}

void CFATable::set_cfa(int reg, int offset) {
        cfa->reg = reg ;
        cfa->offset = offset ;
}

void CFATable::set_cfa_reg(int reg) {
        cfa->reg = reg ;
}

void CFATable::set_cfa_offset(int offset) {
        cfa->offset = offset ;
}

int CFATable::get_cfa_reg() {
        return cfa->reg ;
}

int CFATable::get_cfa_offset() {
        return cfa->offset ;
}

void CFATable::set_reg(int reg, CFARuleType type, int offset) {
        if (reg == ra_reg) {
            ra->type = type ;
            ra->offset = offset ;
        } else {
            regs[reg]->type = type ;
            regs[reg]->offset = offset ;
        }
}

void CFATable::save() {
    saved_regs = new Rule * [arch->total_number_of_regs()] ;
    for (int i = 0 ; i < arch->total_number_of_regs(); i++) {
        saved_regs[i] = new Rule (regs[i]->type, regs[i]->reg, regs[i]->offset) ;
    }
    saved_ra = new Rule (ra->type, ra->reg, ra->offset) ;
}

void CFATable::restore(int reg) {
    if (reg == ra_reg) {
        ra->type = saved_ra->type ;
        ra->offset = saved_ra->offset ;
    } else {
        regs[reg]->type = saved_regs[reg]->type ;
        regs[reg]->offset = saved_regs[reg]->offset ;
    }
}

void CFATable::advance_loc(Address delta) {
        loc += delta ;
}

void CFATable::set_loc(Address l) {
        loc = l ;
}

Address CFATable::get_loc() {
        return loc ;
}

void CFATable::print() {
        std::cout <<"0x" << std::hex <<  loc << " " << cfa->toString() << " " ;
        for (int i = 0 ; i < arch->total_number_of_regs(); i++) {
            std::cout << regs[i]->toString() << " " ;
        }
        std::cout << ra->toString() ;
        std::cout << '\n' ;
}

void CFATable::apply(Frame *from, Frame * to) {

        // set the PC of the next frame to the return address
        //printf ("setting pc to return address\n") ;
        switch (ra->type) {
	case CFA_CFA:
	    break; // added by bos for -Wall niceness
        case CFA_UNDEFINED: ;
        case CFA_SAME_VALUE: ;
            to->set_pc (from->get_pc()) ;
            break ;
        case CFA_OFFSET: {
            int cfa_reg = arch->translate_regnum (cfa->reg) ;
            //printf ("cfa reg is %d (0x%llx)\n", cfa->reg, from->get_reg(cfa_reg)) ;
            //proc->dump (from->get_reg (cfa_reg), 32) ;
            Address v = proc->readptr (from->get_reg (cfa_reg) + cfa->offset + ra->offset) ;
            //printf ("new pc value is 0x%llx\n",v) ;
            to->set_pc (v) ;
            break ;
            }
        case CFA_REGISTER: {
            int sourcereg = arch->translate_regnum (ra->offset) ;
            to->set_pc(from->get_reg (sourcereg)) ;
            break ;
            }
        }

        // set stack pointer of the next frame to the cfa
        int cfa_reg = arch->translate_regnum (cfa->reg) ;
        to->set_sp (from->get_reg (cfa_reg) + cfa->offset) ;

        // set the fp to the previous one
        to->set_fp (from->get_fp()) ;

        //std::cout << "applying cfa rules to registers" << '\n' ;
        int nregs = arch->total_number_of_regs() ;
        for (int i = 0 ; i < nregs; i++) {
            Rule *rule = regs[i] ;
            switch (rule->type) {
	    case CFA_CFA:
		break; // added by bos for -Wall niceness
            case CFA_UNDEFINED: ;
            case CFA_SAME_VALUE: {
                break ;
                }
            case CFA_OFFSET: {
                int cfa_reg = arch->translate_regnum (cfa->reg) ;
                //proc->dump (from->get_reg (cfa_reg), 32) ;
                Address v = proc->readptr (from->get_reg (cfa_reg) + cfa->offset + rule->offset) ;
                std::string regname = arch->reverse_translate_regnum (arch->translate_regnum (rule->reg)) ;
                //std::cout << "    setting reg "  <<  regname  <<  " to 0x" << std::hex <<  v << std::dec << '\n' ;
                to->set_reg (arch->translate_regnum (rule->reg), v) ;           // don't need to worry about floating point
                break ;
                }
            case CFA_REGISTER: {
                if (arch->isfpreg (rule->offset)) {
                    int sourcereg = arch->translate_regnum (rule->offset) ;             // offset into fpregs
                    to->set_fpreg (arch->translate_regnum (rule->reg), from->get_fpreg(sourcereg)) ;
                } else {
                    int sourcereg = arch->translate_regnum (rule->offset) ;
                    std::string regname = arch->reverse_translate_regnum (arch->translate_regnum (rule->reg)) ;
                    std::string sourceregname = arch->reverse_translate_regnum (sourcereg) ;
                    //std::cout << "    setting reg "  <<  regname  <<  " to "  <<  sourceregname  <<  " (0x"  <<  std::hex << frame->get_reg (sourcereg) << std::dec << '\n' ;
                    to->set_reg (arch->translate_regnum (rule->reg), from->get_reg (sourcereg)) ;
                }
                break ;
                }
            }
        }

}


Process::Process (ProcessController * pcm, std::string program, Architecture * arch, Target *target, PStream &os, AttachType at)
    : pcm(pcm),
    program(program),
    arch(arch),
    thread_agent(0),
    target(target),
    pid(0),
    state(IDLE),
    signalnum(0),
    current_thread(threads.end()),
    multithreaded(false),
    bpnum(1),
    ibpnum(1),
    hitbp(NULL),
    r_debug(0),
    stepping_lines(false),
    stepping_over(false),
    main(0),
    frame_cache_valid(false),
    current_frame(-1),
    plt_start(0),
    plt_end(0),
    fixup_addr(0),
    dyn_start(0),
    dyn_end(0),
    os(os),
    current_signal(0),
    displaynum(0),
    attach_type(at),
    last_listed_line(0),
    lastregion(NULL),
    thread_db_initialized(false),
    init_phase(false),
    programtime(0)
{
    if (program != "") {
        struct stat st ;
        if (stat (program.c_str(), &st) != 0) {
            throw Exception ("Unable to read file %s", program.c_str()) ;
        } else {
            programtime = st.st_mtime ;
        }

        // open executable file and read the sections and symbol table
        os.print ("Reading symbols from %s...", program.c_str()) ;
        os.flush() ;
        open_object_file (program, 0, true) ;
        os.print ("done\n") ;
        main = lookup_symbol ("main") ;
        if (main != 0) {
            current_location = lookup_address (main) ;
        }
        // for fortran, we want the file containing MAIN__
        Address MAIN__ = lookup_symbol ("MAIN__") ;
        if (MAIN__ != 0) {
            current_location = lookup_address (MAIN__) ;
        }
    } else {
        open_object_file ("/bin/ls", 0) ;               // anything will do
    }

}

// this is used when the program is being rerun.  Some of the details from the 
// old process need to be copied to the new one
Process::Process (const Process &old) 
    : pcm(old.pcm),
      program(old.program),
      arch(old.arch),
      thread_agent(old.thread_agent),
      target(old.target),
      pid(0),
      state(IDLE),
      signalnum(0),
      current_thread(threads.end()),
      multithreaded(false),
      bpnum(old.bpnum),
      ibpnum(1),
      hitbp(NULL),
      r_debug(0),
      stepping_lines(false),
      stepping_over(false),
      main(0),
      frame_cache_valid(false),
      current_frame(-1),
      plt_start(0),
      plt_end(0),
      fixup_addr(0),
      dyn_start(0),
      dyn_end(0),
      os(old.os),
      current_signal(0),
      displaynum(old.displaynum),
      attach_type(ATTACH_NONE),
      last_listed_line(0),
      lastregion(NULL),
      thread_db_initialized (old.thread_db_initialized),
      init_phase(false),
      programtime(old.programtime)
{
    bool reread_syms = false;

    struct stat st ;
    if (stat (program.c_str(), &st) != 0) {
        throw Exception ("Unable to read file %s", program.c_str()) ;
    } else if (programtime != 0 && st.st_mtime > programtime) {
        reread_syms = true;
    }

    if (reread_syms) {
        printf("Note: program %s has been modified\n", program.c_str()) ;
        if (old.program == "") return ;

        // update modification time
        programtime = st.st_mtime ;

        // open executable file and read the sections and symbol table
        os.print ("Reading symbols from %s...", program.c_str()) ;
        os.flush() ;
        open_object_file (program, 0, true) ;
        os.print ("done\n") ;
        main = lookup_symbol ("main") ;
        if (main != 0) {
            current_location = lookup_address (main) ;
        }
        // for fortran, we want the file containing MAIN__
        Address MAIN__ = lookup_symbol ("MAIN__") ;
        if (MAIN__ != 0) {
            current_location = lookup_address (MAIN__) ;
        }
    }
    else if (program != "") {
        objectfiles.push_back (new ObjectFile (*old.objectfiles[0])) ;          // copy first object file
        main = lookup_symbol ("main") ;
        if (main != 0) {
            current_location = lookup_address (main) ;
        }
    }

    // copy the breakpoints
    if (reread_syms) {
        bpnum = 1 ; // reset breakpoint number
        const BreakpointList &oldbps = old.breakpoints ;
        for ( BreakpointList::const_iterator item = oldbps.begin(); item != oldbps.end(); item++) {
            Breakpoint *bp = *item ;
            int mynum = bp->get_num() ;
            std::string text = bp->get_text() ; 
            std::string::size_type pos ;
            Address addr = 0 ;

            // if not text just bail out
            if (text == "") continue ;

            pos = text.find(":") ;  // check for a filename or a function name
            if (pos != std::string::npos && pos < text.size() - 1 && text[pos+1] != ':') {
                long lineno = strtol(text.substr(pos+1).c_str(), NULL, 10);
                addr = lookup_line(text.substr(0,pos), lineno);
            } else {
                addr = lookup_function(realname(text)) ; 
            }

            // can't find, warn and keep going
            if (addr == 0) {
                if (text != "") {
                    printf("Warning: unable to set breakpoint at %s\n",text.c_str());
                }
                continue ;
            }

            // just assume it's a user breakpoint
            bp = new UserBreakpoint (arch, this, text, addr, mynum);

            // add it to the list
            add_breakpoint(bp) ;

            // make current bpnum maximum number
            if (bpnum <= mynum) bpnum = mynum + 1;
        }
    } else if (program != "") {
        const BreakpointList &oldbps = old.breakpoints ;
        for ( BreakpointList::const_iterator item = oldbps.begin() ; item != oldbps.end() ; item++) {
            Breakpoint *bp = *item ;
            if (bp->is_sw_watchpoint() || bp->is_hw_watchpoint()) {
               os.print("Watchpoint %d was deleted on program restart\n", bp->get_num());
            } else {
               add_breakpoint (bp->clone()) ;
            }
        }
    }

    // copy the displays
    const DisplayList &olddisps = old.displays ;
    for ( DisplayList::const_iterator item = olddisps.begin() ; item != olddisps.end() ; item++) {
        Display *d = *item ;
        displays.push_back (new Display (*d)) ;
        d->node = NULL ;                        // prevent deletion of expression when old display is deleted
    }

    reset() ;
    old.objectfiles[0]->reset() ;           // prevent innards being deleted
}

Process::~Process() {
    if (is_running()) {
       //printf ("detaching target %p", target) ;
        if (multithreaded) {
            if (attach_type == ATTACH_PROCESS) {
                detach_breakpoints() ;
                detach_threads() ;
            } else {
                kill_threads() ;
            }
        } else {
            target->detach(pid) ;
        }
    }

    // the state must be set to idle, so the process will
    // have is_running() as false. This will prevent the
    // deletion of a user breakpoint from trying to restore
    // the original instruction value
    state = IDLE;

    // delete the object files
    for (uint i = 0 ; i < objectfiles.size() ; i++) {
        delete objectfiles[i] ;
    }

    // delete the link maps
    for (uint i = 0 ; i < linkmaps.size() ; i++) {
        delete linkmaps[i] ;
    }

    // delete the open streams
    for (std::list<std::istream*>::iterator i = open_streams.begin();
         i != open_streams.end(); i++) {
       delete *i;
    }

    // delete the threads
    for (ThreadList::iterator i = threads.begin() ; i != threads.end() ; i++) {
        delete *i ;
    }

    // delete the breakpoints
    for (BreakpointList::iterator i = breakpoints.begin() ; i != breakpoints.end() ; i++) {
        delete *i ;
    }

    // delete the displays
    for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
        delete *i ;
    }

    // delete frames from the frame cache
    if(frame_cache_valid) invalidate_frame_cache();
}

const char *Process::get_state() {
    switch (state) {
    case IDLE:
        return "IDLE" ;
        break ;
    case READY:
        return "READY" ;
        break ;
    case RUNNING:
        return "RUNNING" ;
        break ;
    case DISABLED:
        return "DISABLED" ;
        break ;
    case STEPPING:
        return "STEPPING" ;
        break ;
    case EXITED:
        return "EXITED" ;
        break ;
    case ISTEPPING:
        return "ISTEPPING" ;
        break ;
    case CSTEPPING:
	return "CSTEPPING" ;
    }
    throw Exception("not reached");
}

// reset can be called in two circumstances:
//  1. when a new process has been cloned from an existing one.  This happens when the
//     user types 'run' and the program is already running.  There will be no threads
//  2. When the process is in an exit state and the user types 'run'.  In this case,
//     we need to delete all the threads

void Process::reset() {
    // reset the breakpoints so that they will be reapplied when the program starts
    // also, remove the non-user breakpoints as these will be set again
    BreakpointList system_bps ;

    for ( BreakpointList::iterator item = breakpoints.begin() ; item != breakpoints.end() ; item++) {
        (*item)->reset(this) ;
        if (!(*item)->is_user()) {
            system_bps.push_back (*item) ;
        }
    }

    // now go through all system bps and remove them
    for ( BreakpointList::iterator item = system_bps.begin() ; item != system_bps.end() ; item++) {
        remove_breakpoint (*item) ;
    }

    threads.clear() ;           // clear the threads list

    Thread::reset() ;           // reset threads
}

// we have a core file, attach to it (in the target) and behave like we've run until the
// signal that generated the core

void Process::attach_core() {
    CoreTarget *core = dynamic_cast<CoreTarget*>(target) ;
    if (core == NULL) {
       throw Exception ("Target is not a core") ;
    }
    std::string coreprog = core->get_program() ;
    std::string progtail = program ;
    std::string::size_type slash = progtail.rfind ('/') ;
    if (slash != std::string::npos) {
        progtail = progtail.substr (slash+1) ;
    }
    if (progtail != coreprog) {
        printf("warning: Program from core file (%s) does not match executable "
                "name (%s)\n", coreprog.c_str(), program.c_str()) ;
    }

    core->map_file (objectfiles[0]->elf) ;              // map in the main program

    int nthreads = core->get_num_threads() ;

    int signum = core->get_signal() ;
    if (signum > 0) {
        os.print ("Program terminated with signal ") ;
        signal_manager.print (signum, os) ;
        os.print (".\n") ;                  // signal manager doesn't print newline
    }

    // load the threads from the core file
    for (int i = 0 ; i < nthreads ; i++) {
        Thread *thr = new Thread (arch, this, core->get_thread_pid(i), core->get_thread_tid(i)) ;
        threads.push_front (thr) ;                // main thread
        current_thread = threads.begin() ;
        thr->syncin() ;                         // read the thread registers
    }
    if (nthreads > 1) {
        multithreaded = true ;
    }

    // load the dynamic info stuff
    load_dynamic_info(false) ;

    std::vector<ELF *> loadedfiles ;
    for (uint i = 1 ; i < objectfiles.size() ; i++) {
        loadedfiles.push_back (objectfiles[i]->elf) ;
    }
    core->map_code (loadedfiles) ;

    state = DISABLED ;

    // set the current thread to the one that raised the signal
    int threadpid = core->get_terminating_thread() ;
    if (threadpid > 0) {
        current_thread = find_thread (threadpid) ;
    } else {
        current_thread = threads.begin() ; 
    }

    build_frame_cache() ;

    try {
        // show the position of the signal
        Frame *frame =  frame_cache[current_frame] ;
        frame->print (os, false, false); 
        frame->get_loc().show_line(os, get_cli()->isemacs()) ;
        set_current_line (frame->get_loc().get_line()) ;
    } catch (Exception e) {
        e.report (std::cout) ;
    }
}

void Process::attach_process(int pid) {
    threads.push_front (new Thread (arch, this, pid, 0)) ;                // main thread
    current_thread = threads.begin() ;

    this->pid = target->attach (program, pid) ;                     // attach to target process
    state = RUNNING ;
    wait() ; // wait for it to stop
    // state will be READY here


    // load the dynamic info stuff
    load_dynamic_info(false) ;

    (*current_thread)->syncin() ;         // load registers as addresses may not be valid
    build_frame_cache() ;

    // show the position of the signal
    Frame *frame =  frame_cache[current_frame] ;
    frame->print (os, false, false); 
    frame->get_loc().show_line(os, get_cli()->isemacs()) ;
    set_current_line (frame->get_loc().get_line()) ;
}

// attach to a child process
void Process::attach_child (int childpid, State parent_state) {
    threads.push_front (new Thread (arch, this, childpid, 0)) ;                // main thread
    current_thread = threads.begin() ;
    pid = childpid ;

    target = Target::new_live_target (arch) ;                // new target

    // load the dynamic info stuff
    load_dynamic_info(false) ;

    state = READY ;
    if (parent_state == STEPPING) {
        (*current_thread)->syncin() ;         // load registers as addresses may not be valid
        build_frame_cache() ;

        // show the position of the signal
        Frame *frame =  frame_cache[current_frame] ;
        frame->print (os, false, false); 
        frame->get_loc().show_line(os, get_cli()->isemacs()) ;
        set_current_line (frame->get_loc().get_line()) ;
    } else {
        docont() ;              // continue running the process
    }

}

void Process::show_current_thread() {
    if (multithreaded) {
        os.print (" (thread %d)", (*current_thread)->get_num()) ;
    }
}

void Process::new_thread(void * id) {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        if ((*t)->get_tid() == id) {
            return ;                  // thread already exists
        }
    }
    int thr_pid = pid ;
    target->get_thread_tid (thread_agent, id, thr_pid) ;
    Thread * t = new Thread (arch, this, thr_pid, id) ;

    os.print ("[New ") ; t->print (os) ; os.print ("]\n") ;

//     target->attach (info.ti_lid) ;
    threads.push_front (t)  ;

//#if defined (__linux__)
    target->attach (thr_pid) ;

    int status ;
    //printf ("waiting for thread\n") ;
    int ret ;
    do {
        //printf ("waitpid %d\n", info.ti_lid) ;
//         ret = waitpid (info.ti_lid, &status, __WALL) ;
	ret = waitpid (thr_pid, &status, WAITPID_ALL_CHILD_TYPES) ;
        if (ret < 0) {
            perror ("waitpid") ;
        }
    } while ((ret == -1 && errno == EINTR)) ;
//#endif

	t->syncin();

#if 0
    // if there is no child available, try cloned children
    if (ret == -1 && errno == ECHILD) {
        printf ("checking cloned threads\n") ;
        ret = waitpid (info.ti_lid, &status, __WCLONE) ;
        if (ret == -1) {
            perror ("waitpid clone") ;
        }
    }
#endif
    //printf ("status = %x\n", status) ;
}

void Process::validate_thread (int n) {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        if (thr->get_num() == n) {
            return ;
        }
    }
    throw Exception ("Unknown thread %d.", n) ;
}

int Process::get_current_thread() {
    return (*current_thread)->get_num() ;
}

// disable all threads except the current one
void Process::disable_threads() {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        if (t != current_thread) {
            Thread *thr = *t ;
            thr->disable() ;
        }
    }
}

void Process::enable_threads() {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        if (t != current_thread) {
            Thread *thr = *t ;
            thr->enable() ;
        }
    }
}

// kill all the threads
void Process::kill_threads() {
	//FIXME: Factor out
    // first send them all SIGKILL signal
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
	target->thread_kill (*t);
    }
//#if 0
    // now resume them so that they get the signal
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        target->cont (thr->get_pid(), 0) ;
        thr->go() ;
    }
//#endif
    uint nkilled = 0 ;
    int status ;
    while (nkilled < threads.size()) {
        for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
            Thread *thr = *t ;
//             int p = waitpid (thr->get_pid(), &status, __WALL|WNOHANG) ;
	     int p = waitpid (thr->get_pid(), &status, WAITPID_ALL_CHILD_TYPES|WNOHANG) ;
            if (p > 0) {
                nkilled++ ;
            }
        }
    }
}

void Process::resume_threads() {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        if (!thr->is_running()) {
            //thread_db::resume_thread (thread_agent, thr->get_tid()) ;
            thr->syncout() ;                    // synchronize the registers
            int retry = 3 ;
            while (retry-- > 0) {
                try {
                    target->cont (thr->get_pid(), 0) ;
                    break ;
                } catch (...) {
                    printf ("failed to resume thread %d\n", thr->get_num()) ;
                }
            }
            //printf ("thread %d running\n", thr->get_pid()) ;
            thr->go() ;
        }
    }
}


void Process::detach_threads() {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        thr->syncout() ;                    // synchronize the registers
        try {
            target->detach (thr->get_pid()) ;
            break ;
        } catch (...) {
            printf ("failed to detach thread %d\n", thr->get_num()) ;
        }
    }
}


void Process::stop_threads() {
// FIXME: Factor out
    reap_threads() ;
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        if (thr->is_running()) {
            //thread_db::suspend_thread (thread_agent, thr->get_tid()) ;
	    //target->thread_kill (thr);
	    target->thread_suspend (thr);
            int status ;
//             waitpid (thr->get_pid(), &status, __WALL) ;                 // wait for it to stop
	    waitpid (thr->get_pid(), &status, WAITPID_ALL_CHILD_TYPES) ;                 // wait for it to stop
            thr->set_stop_status (status) ;
            thr->stop() ;
        } else {
            // it is possible that a thread has stopped between the call to mt_wait and the call to
            // grope threads.  In that case we need to retrieve its status
            int status ;
            //if (waitpid (thr->get_pid(), &status, __WALL|WNOHANG) > 0) {
	if (waitpid (thr->get_pid(), &status, WAITPID_ALL_CHILD_TYPES|WNOHANG) > 0) {
                printf ("thread %d, status %x\n", thr->get_num(), status); 
                thr->set_stop_status (status) ;
            }
        }
    }
    // now read all the registers
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        thr->syncin() ;
    }

    // to facilitate per-thread resume (see resume_threads()), now place all
    // threads into a suspended state.
//     for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
//         Thread *thr = *t ;
// 	target->suspend_thread (thr);
//     }
}

Process::ThreadList::iterator Process::find_thread (int pid) {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        if ((*t)->get_pid() == pid) {
            return t ;
        }
    }
    throw Exception ("Unable to find thread with given pid") ;
}

// look for a thread whose pc points at a breakpoint.  This is called for SYSTRAP
// all the threads have been synced (their registers are valid).  This is done by
// stop_threads()

void Process::find_bp_threads (std::vector<Process::ThreadList::iterator> &result, std::vector<Process::ThreadList::iterator> &userbps) {
    reap_threads() ;
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        if (!thr->is_running()) {
            int status = thr->get_stop_status() ;
            if (!(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP)) {         // must be on a SIGTRAP
                continue ;
            }
            Address pc = thr->get_reg("pc") - arch->bpsize() ;
            //printf ("thread %d, pc = 0x%llx\n", thr->get_pid(), (unsigned long long)pc);
            BreakpointList *bps = find_breakpoint (pc) ;
            if (bps != NULL) {
                result.push_back (t) ;
                // if any of the breakpoints are user breakpoints, record this
                for (BreakpointList::iterator i = bps->begin() ; i != bps->end() ; i++) {
                    if ((*i)->is_user()) {
                        userbps.push_back (t) ;
                    } 
                }
            }
        }
    }
}

// grope at the threads to see which ones give us a slap
void Process::grope_threads() {
    reap_threads() ;
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        try {
            //thread_db::read_thread_registers (thread_agent, thr->get_tid(), regbuf) ;
            target->get_debug_reg (thr->get_pid(), 6) ;
            //thr->syncin() ;                     // try to read the regs
            thr->stop() ;
        } catch (...) {               // slap! oops, thread is awake (or maybe dead)
        }       
    }
}

// check for any dead threads and reap them
void Process::reap_threads() {
    std::vector<ThreadList::iterator> undertaker ;

    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
        Thread *thr = *t ;
        int status ;
        //int e = waitpid (thr->get_pid(), &status, WNOHANG | __WALL) ;
	int e = waitpid (thr->get_pid(), &status, WNOHANG | WAITPID_ALL_CHILD_TYPES) ;
        if (e > 0) {
            thr->set_stop_status (status) ;
            if (WIFEXITED (status)) {
                os.print ("[") ; thr->print (os) ; os.print (" exited]\n") ;
                undertaker.push_back (t) ;
            }
        }
    }

    // bury the threads that have died
    bool current_dead = false ;
    for (uint i = 0 ; i < undertaker.size() ; i++) {
        if (undertaker[i] == current_thread) {
            current_dead = true ;
        }
        threads.erase (undertaker[i]) ;
    }
    // if the current thread died, choose something else as the current
    // choose the first one in the list, in the absence of a better choice
    if (current_dead) {
        for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
            current_thread = t ;
            break ;
        }
    }
}   

void Process::open_object_file(std::string name, Address baseaddress, bool reporterror) {
    //println ("opening object file " + name)
    ELF * elf = new ELF (name) ;
    std::istream * elfstream = elf->open(baseaddress) ;
    elf->read_symbol_table (*elfstream, baseaddress) ;
    //elf->list_symbols() ;
    SymbolTable * symtab = NULL ;
    try {
        symtab = new SymbolTable (arch, elf, *elfstream, pcm->get_aliases(), pcm->get_dirlist(), &os, reporterror) ;
    } catch (Exception e) {
        os.print ("no debugging information for file %s\n", name.c_str()) ;
    } catch (const char *s) {
        os.print ("no debugging information for file %s\n", name.c_str()) ;
    }
    objectfiles.push_back (new ObjectFile (name, elf, *elfstream, symtab)) ;
    open_streams.push_back(elfstream);

    // add code region to valid region table
    Section *text = elf->find_section (".text") ;
    if (text != NULL) {
        CodeRegion region (text, baseaddress) ;
        //printf ("new code region 0x%llx..0x%llx\n", region.start, region.end) ;
        code_regions.push_back (region) ;
    }
    
    // check for the dynamic linker
    Section* dyn = elf->find_section(".dynamic");
    if (dyn == NULL)  return;

    fixup_addr = elf->find_symbol ("fixup") ;
    Section * plt = elf->find_section (".plt") ;
    if (plt == NULL)  return;

    plt_start = plt->get_addr() + baseaddress ;
    plt_end = plt_start + plt->get_size() ;

    //printf ("PLTbase: 0x%llx\n", plt->get_addr()) ;
    //printf ("PLT: 0x%llx .. 0x%llx\n", plt_start, plt_end) ;
    dyn_start = text->get_addr() + baseaddress ;
    dyn_end = dyn_start + text->get_size() ;

}

// validate that the address is a valid code address
bool Process::is_valid_code_address (Address addr) {
    return true ;               // XXX: for a while
    if (lastregion != NULL && addr >= lastregion->start && addr < lastregion->end) {
        return true ;
    }
    for (uint i = 0 ; i < code_regions.size() ; i++) {
        CodeRegion &region = code_regions[i] ;
        if (addr >= region.start && addr < region.end) {
            lastregion = &region ;
            return true ;
        }
    }
    return false ;
}

int Process::get_language(bool &isauto) {
    isauto = false ;

    LangType lang = (LangType)get_int_opt(PRM_LANGUAGE);

    switch(lang) {
    case LANG_C:
       return DW_LANG_C;
    case LANG_CXX:
       return DW_LANG_C_plus_plus;
    case LANG_F90:
       return DW_LANG_Fortran90;
    case LANG_AUTO:
       isauto = true;
       return get_main_language();
    }
    return DW_LANG_C ;
}

int Process::get_main_language() {
    Address MAIN__ = lookup_symbol ("MAIN__") ;
    if (MAIN__ != 0) {
        Location loc = lookup_address (MAIN__) ;
        if (loc.get_funcloc() != NULL) {
            int lang = loc.get_funcloc()->symbol->die->get_language() ;
            return lang ;
        }
    } else {
        Address main = lookup_symbol ("main") ;
        if (main != 0) {
            Location loc = lookup_address (main) ;
            if (loc.get_funcloc() != NULL) {
                int lang = loc.get_funcloc()->symbol->die->get_language() ;
                return lang ;
            }
        }
    }

    // no main, look at the language parameter
    LangType lang = (LangType)get_int_opt(PRM_LANGUAGE);

    switch(lang) {
    case LANG_C:
       return DW_LANG_C;
    case LANG_CXX:
       return DW_LANG_C_plus_plus;
    case LANG_F90:
       return DW_LANG_Fortran90;
    case LANG_AUTO:
       return DW_LANG_C;
    }
    return DW_LANG_C ;
}

// get a string that shows the value of the parameters of a function
void Process::print_function_paras (Frame *frame, DIE *die) {
    die->check_loaded() ;
    Frame *cf = frame_cache[current_frame] ;
    Thread *ct = (*current_thread) ;

    frame->publish_regs(ct) ;           // set registers to those in frame

    Subprogram *subprogram = dynamic_cast<Subprogram*>(die) ;
    std::vector<DIE*> paras ;
    subprogram->get_formal_parameters (paras) ;
    bool autolang ;
    int language = get_language (autolang) ;
    if (autolang) {
        language = die->get_language() ;
    }
    EvalContext context (this, frame->get_fp(), language, os) ;
    context.show_contents = false ;
    context.show_reference = false ;
    context.truncate_aggregates = true ;
    context.pretty = false ;

    os.print ("(") ;
    bool comma = false ;
    for (uint i = 0 ; i < paras.size() ; i++) {
        DIE *para = paras[i] ;
        std::string name = para->get_name() ;
        if (comma) os.print (", ") ;
        comma = true ;
        os.print ("%s=", name.c_str()) ;
        Value value = para->evaluate (context) ;

        // reals will come back as integers with the correct bit pattern.  We need to convert these to
        // doubles
        DIE *type = para->get_type() ;
        if (type->is_real()) {
            if (type->get_size() == 4) {                    // float?
                value.real = (double)(*(float*)&value.real) ;
            }
            value.type = VALUE_REAL ;
        }

        para->get_type()->print_value (context, value) ;
    }
    os.print (")") ;
    cf->publish_regs(ct) ;
}

void Process::print_expression(std::string expr, Format &fmt, bool terse, bool record) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    int language = get_language(autolang) ;
    DIE *die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.has_debug_info()) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
        } else {
            symtab = objectfiles[0]->symtab ;
        }
    }
    Node *tree = NULL ;

    int end ;
    if (language == DW_LANG_C || language == DW_LANG_C89 || language == DW_LANG_C_plus_plus) {
        CExpressionHandler e (symtab, arch, language) ;
        tree = e.parse (expr, this, end) ;
    } else if (language == DW_LANG_Fortran77 || language == DW_LANG_Fortran90) {
        FortranExpressionHandler e (symtab, arch, language) ;
        tree = e.parse (expr, this, end) ;
    }
    if (tree != NULL) {
        while ((uint) end < expr.size() && isspace (expr[end])) end++ ;
        if ((uint) end != expr.size()) {
            throw Exception ("Junk at end of expression: %s", expr.substr (end).c_str()) ;
        }

        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        context.fmt = fmt ;                          // set format for printing
        Value value = tree->evaluate (context) ;
        DIE *type = tree->get_type() ;
        bool newline = false ;
        if (!terse) { 
            if (record || type != NULL) {
                int n = pcm->get_cli()->new_debugger_var_num() ;
                add_debugger_variable (n, value, type) ;
                symtab->keep_temp_die (type) ;
                os.print ("$%d = ", n) ;
                newline = true ;
            }
        }
        // we don't want to print 'void' if the type is NULL and we were asked not to record
        if (record) {
            if (type == NULL) {
                 os.print ("void") ;
            } else {
                if (value.type == VALUE_VECTOR) {
                    print_vector (context, value, type) ;
                } else {
                    type->print_value (context, value) ;
                }
            }
        } else {
            if (type != NULL) {
                if (value.type == VALUE_VECTOR) {
                    print_vector (context, value, type) ;
                } else {
                    type->print_value (context, value) ;
                }
            }
        }

        // remove tree and all temporary DIE objects created for the tree
        delete tree ;
        symtab->delete_temp_dies() ;

        if (newline) {
            context.os.print ("\n") ;
        } else {
            context.os.flush() ;
        }
    }
}

void Process::display_expression(int num, std::string expr, Node *tree, Format &fmt) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    int language = get_language(autolang) ;
    DIE *die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.get_funcloc() != NULL) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
         } else {
            symtab = objectfiles[0]->symtab ;
         }
    }

    if (tree != NULL) {
        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        context.fmt = fmt ;                          // set format for printing
        Value value = tree->evaluate (context) ;
        DIE *type = tree->get_type() ;
        os.print ("%d: %s = ", num, expr.c_str()) ;
        if (type == NULL) {
             os.print ("void") ;
        } else {
            if (value.type == VALUE_VECTOR) {
                print_vector (context, value, type) ;
            } else {
                type->print_value (context, value) ;
            }
        }
        context.os.print ("\n") ;
    }
}

void Process::print_vector (EvalContext &ctx, Value &v, DIE *type) {
    std::vector<Value> &vec = v.vec ;
    os.print ("{") ;
    bool comma = false ;
    for (uint i = 0 ; i < vec.size() ; i++) {
        if (comma) {
            os.print (", ") ;
        }
        if (vec[i].type == VALUE_VECTOR) {
            print_vector (ctx, vec[i], type) ;
        } else {
            type->print_value (ctx, vec[i]) ;
            comma = true ;
        }
    }
    os.print ("}") ;
}

void Process::print_vector_type (EvalContext &ctx, Value &v, DIE *type) {
    std::vector<Value> &vec = v.vec ;
    ctx.os.print ("[%d]", vec.size()) ;
    for (uint i = 0 ; i < vec.size() ; i++) {
        if (vec[i].type == VALUE_VECTOR) {
            print_vector_type (ctx, vec[i], type) ;
            break ;
        }
    }
}

void Process::print_type(std::string expr, bool show_contents) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    int language = get_language(autolang) ;
    DIE *die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.has_debug_info()) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
        } else {
            symtab = objectfiles[0]->symtab ;
        }
    }
    Node *tree = NULL ;

    if (language == DW_LANG_C || language == DW_LANG_C89 || language == DW_LANG_C_plus_plus) {
        CExpressionHandler e (symtab, arch, language) ;
        tree = e.parse (expr, this) ;
    } else if (language == DW_LANG_Fortran77 || language == DW_LANG_Fortran90) {
        FortranExpressionHandler e (symtab, arch, language) ;
        tree = e.parse (expr, this) ;
    }
    if (tree != NULL) {
        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        context.show_contents = show_contents ;
        context.addressonly = true;                             // constant expressions just ignore this
        Value value = tree->evaluate (context) ;                // type is only known after evaluation
        DIE *type = tree->get_type() ;
        if (type != NULL) {
            type->check_loaded() ;
            // printing the type of a pointer to a struct should print the dynamic type, not the static
            if (type->is_pointer()) {
                DIE *t = type->get_type() ;
                if (t != NULL && t->is_struct()) { 
                    while (t->get_tag() != DW_TAG_structure_type && t->get_tag() != DW_TAG_union_type) {
                        t = t->get_type() ;
                    }
                    t->check_loaded() ;
                    type = dynamic_cast<TypeStruct*>(t)->get_dynamic_type (context, value) ;
                    type = symtab->new_pointer_type (type) ;    // put pointer back
                }
            }
        }
        os.print ("type = ") ;
        if (type == NULL) {
            os.print ("void") ;
        } else {
            if (type->get_tag() == DW_TAG_typedef) {
                type->get_type()->print(context, 0) ; 
            } else {
                if (value.type == VALUE_VECTOR) {
                    type->print(context, 0) ; 
                    context.os.print (" ") ;
                    print_vector_type (context, value, type) ;
                } else {
                    type->print(context, 0) ; 
                }
            }
        }
        os.print ("\n") ;

        // remove tree and all temporary DIE objects created for the tree
        delete tree ;
        symtab->delete_temp_dies() ;
    }
}

Node *Process::compile_expression(std::string expr, int &end, bool single) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    int language = get_language(autolang) ;
    DIE *die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.get_funcloc() != NULL) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
         } else {
            symtab = objectfiles[0]->symtab ;
         }
    }
    Node *tree = NULL ;

    if (language == DW_LANG_C || language == DW_LANG_C89 || language == DW_LANG_C_plus_plus) {
        CExpressionHandler e (symtab, arch, language) ;
        tree = single ? e.parse_single (expr, this, end) : e.parse (expr, this, end) ;
    } else if (language == DW_LANG_Fortran77 || language == DW_LANG_Fortran90) {
        FortranExpressionHandler e (symtab, arch, language) ;
        tree = single ? e.parse_single (expr, this, end) : e.parse (expr, this, end) ;
    }
    symtab->keep_temp_dies (expression_dies) ;          // keep the dies for now
    return tree ;
}

Address Process::evaluate_expression(std::string expr, int &end, bool needint) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    int language = get_language(autolang) ;
    DIE *die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.get_funcloc() != NULL) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
         } else {
            symtab = objectfiles[0]->symtab ;
         }
    }
    Node *tree = NULL ;

    if (language == DW_LANG_C || language == DW_LANG_C89 || language == DW_LANG_C_plus_plus) {
        CExpressionHandler e (symtab, arch, language) ;
        tree = e.parse (expr, this, end) ;
    } else if (language == DW_LANG_Fortran77 || language == DW_LANG_Fortran90) {
        FortranExpressionHandler e (symtab, arch, language) ;
        tree = e.parse (expr, this, end) ;
    }
    //println ("parsed expression")
    if (tree != NULL) {
        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        //XXX: addressonly?
        Value value = tree->evaluate (context) ;
        DIE* type = tree->get_type();
        if (type == NULL) return 0;

        if (needint && (!type->is_function() &&
                        !type->is_integral() && 
                        !type->is_pointer() && 
                        !type->is_array() &&
                        !type->is_address())) {
            throw Exception ("Integral expression expected in this context") ;
        }

        // remove tree and all temporary DIE objects created for the tree
        delete tree ;
        symtab->delete_temp_dies() ;

        return value ;
    } else {
        throw Exception ("Can't evaluate expression") ;
    }

}


Value Process::evaluate_expression(Node *tree, bool addressonly) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    int language = get_language(autolang) ;
    DIE *die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.get_funcloc() != NULL) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
         } else {
            symtab = objectfiles[0]->symtab ;
         }
    }

    //println ("parsed expression")
    if (tree != NULL) {
        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        context.addressonly = addressonly ;
        Value value = tree->evaluate (context) ;
        return value ;
    } else {
        throw Exception ("Can't evaluate expression") ;
    }

}

void Process::complete_symbol (std::string name, std::vector<std::string> &result) {
    build_frame_cache() ;
    Address pc = 0 ;
    if (pc == 0) {
        if (threads.size() != 0) {
            pc = get_reg("pc") ;                // pc is required for lexical blocks
        }
    }
    if (frame_cache_valid) {              // is there a current frame?
        Location &loc = frame_cache[current_frame]->get_loc() ;
        DIE *die = NULL ;
        if (loc.get_funcloc() != NULL) {
            die = loc.get_funcloc()->symbol->die ;
        } 
        if (die != NULL) {
            die->complete_symbol (name, pc, result) ;
        }
    }
}


void Process::complete_function (std::string name, std::vector<std::string> &result) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (file->symtab != NULL) {
            file->symtab->complete_function (name, result) ;
        }
    }
}

void Process::set_value (Value& loc, Value &v, int size) {
    if (loc.type == VALUE_REG) {
        if (v.type == VALUE_INTEGER) {
            set_reg (loc.integer, v.integer) ;
        } else if (v.type == VALUE_REAL) {
            throw Exception ("Can't set the value of a real yet") ;
        } else {
            throw Exception ("Illegal value for a register") ;
        }
    } else if (loc.type == VALUE_INTEGER) {         // address
        if (v.type == VALUE_INTEGER) {
            write (loc.integer, v.integer, size) ;
        } else if (v.type == VALUE_REAL) {
            if (size == 4) {                            // float?
                float f = (float)v.real ;
                write (loc.integer, *(int*)&f, size) ;
            } else {
                write (loc.integer, v.integer, size) ;
            }
        } else {
            throw Exception ("Illegal value for a register") ;
        }
    } else {
        throw Exception ("Illegal address") ;
    }
}

void Process::sync_threads() {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end(); t++) {
       (*t)->syncout() ;
   }
}

Address Process::get_reg(std::string name) {
    if (threads.size() == 0 || state == IDLE || state == EXITED) {
        throw Exception ("No registers") ;
    }
    return (*current_thread)->get_reg (name) ;
}

Address Process::get_fpreg(std::string name) {
    if (threads.size() == 0 || state == IDLE || state == EXITED) {
        throw Exception ("No registers") ;
    }
    return (*current_thread)->get_fpreg (name) ;
}

Address Process::get_reg(int num) {
    return (*current_thread)->get_reg (num) ;
}

void Process::set_reg(std::string name, Address value) {
    (*current_thread)->set_reg (name, value) ;
}

void Process::set_fpreg(std::string name, Address value) {
    (*current_thread)->set_fpreg (name, value) ;
}

void Process::set_reg(int num, Address value) {
    (*current_thread)->set_reg (num, value) ;
}

Address Process::get_debug_reg (int reg) {
    return target->get_debug_reg ((*current_thread)->get_pid(), reg) ;
}

void Process::set_debug_reg (int reg, Address value) {
    target->set_debug_reg ((*current_thread)->get_pid(), reg, value) ;
}

struct StateHolder
{
	StateHolder(RegisterSetProperties *props, RegisterSetProperties *fp_props)
	{
		regs = props->new_empty_register_set();
		fpregs = props->new_empty_register_set();
	}
	~StateHolder()
	{
		delete regs;
		delete fpregs;
	}
	RegisterSet *regs;
	RegisterSet *fpregs;
	Breakpoint *hitbp;
};

StateHolder* Process::save_and_reset_state()
{
	StateHolder *sh = new StateHolder(arch->main_register_set_properties(), 
	                                  arch->fpu_register_set_properties());
	(*current_thread)->save_regs(sh->regs, sh->fpregs) ;
	sh->hitbp = hitbp;
	hitbp = NULL;
	return sh;
}

void Process::restore_state(StateHolder *sh)
{
	(*current_thread)->restore_regs(sh->regs, sh->fpregs) ;
	hitbp = sh->hitbp ;
	delete sh ;
	invalidate_frame_cache() ;
}



void Process::add_breakpoint(Breakpoint * bp, bool update) {
    if (!update) {
        breakpoints.push_back (bp)        ;
    }
    if (bp->is_sw_watchpoint()) {
        sw_watchpoints.push_back (bp) ;
        return ;                        // nothing else to do as there is no address
    }

    Address addr = bp->get_address() ;
    if (addr == 0) {
        return ;
    }
    BreakpointMap::iterator bpi = bpmap.find (addr) ;           // already a breakpoint at that address?
    BreakpointList *bplist = NULL ;
    if (bpi == bpmap.end()) {
        bplist = new BreakpointList() ;
        bpmap[addr] = bplist ;
    } else {
        bplist = bpi->second ;
        if (!bp->is_watchpoint() && bp->is_user()) {
            int n = 0 ;
            for (BreakpointList::iterator i = bplist->begin() ; i != bplist->end() ; i++) {
                if ((*i)->is_user() && !(*i)->is_watchpoint()) {
                    n++ ;
                }
            }
            if (n > 0) {
                os.print ("Note: breakpoint%s ", n==1?"":"s") ;
            }
            bool comma = false ;
            int j = 0 ;
            for (BreakpointList::iterator i = bplist->begin() ; i != bplist->end() ; i++, j++) {
                if ((*i)->is_user() && !(*i)->is_watchpoint()) {
                    if (n > 1 && j == (n-1)) {
                        os.print (" and ") ;
                    } else if (comma) os.print (", ") ;
                    os.print ("%d", (*i)->get_num()) ;
                    comma = true ;
                }
            }

            if (n > 0) {
                os.print (" also set at pc 0x%llx.\n", addr) ;
            }
        }
    }
    bplist->push_back (bp) ;
    //print_bps() ;
}

int Process::breakpoint_count() {
    int n = 0 ;
    for ( BreakpointList::iterator item = breakpoints.begin() ; item != breakpoints.end() ; item++) {
        if ((*item)->is_user()) {
            n++ ;
        }
    }
    return n ;
}

std::list<Breakpoint*> *Process::find_breakpoint(Address addr) {
    int status = arch->get_debug_status(this) ;
    if ((status & 0x0f) != 0) {                         // debug register triggered?
        for (int i = 0 ; i < 4 ; i++) {         // XXX: architecture dependent
            if ((status & (1 << i)) != 0) {
                addr = arch->get_debug_reg (this, i) ;                // get trigger address
                break ;                                         // fall through to find breakpoint
            }
        }
    }

    BreakpointMap::iterator bpi = bpmap.find (addr) ;
    if (bpi == bpmap.end()) {
        return NULL ;
    }
    return bpi->second ;
}

// see if any software watchpoints are enabled
bool Process::sw_watchpoints_active() {
    for (BreakpointList::iterator i = sw_watchpoints.begin() ; i != sw_watchpoints.end() ; i++) {
        if (!(*i)->is_disabled() && (*i)->is_applied()) { 
            return true ;
        }
    }
    return false ;
}

Breakpoint *Process::find_breakpoint (int bpnum) {
    for ( BreakpointList::iterator item = breakpoints.begin() ; item != breakpoints.end() ; item++) {
        if ((*item)->get_num() == bpnum) {
            return *item ;
        }
    }
    return NULL ;
}


void Process::print_bps () {
    for (BreakpointMap::iterator bpi = bpmap.begin() ; bpi != bpmap.end() ; bpi++) {
        BreakpointList *bplist = bpi->second ;
        for (BreakpointList::iterator li = bplist->begin() ; li != bplist->end() ; li++) {
            os.print ("%d at 0x%llx\n", (*li)->get_num(), bpi->first) ;
        }
    }
}

void Process::remove_breakpoint(Breakpoint * bp) {
    //printf ("removing breakpoint\n") ;
    BreakpointMap::iterator bpi = bpmap.find (bp->get_address()) ;
    if (bpi != bpmap.end()) {
        BreakpointList *bplist = bpi->second ;
        for ( BreakpointList::iterator item = bplist->begin() ; item != bplist->end() ; item++) {
            if (*item == bp) {
                bplist->erase (item) ;
                break ;
            }
        }
        if (bplist->size() == 0) {          // last bp removed from list?
            bpmap.erase (bpi) ;
        }
    } else {
        if (!bp->is_sw_watchpoint() && bp->is_user()) {
            os.print ("unknown breakpoint %d\n", bp->get_num()) ;
        }
    }
    for ( BreakpointList::iterator item = breakpoints.begin() ; item != breakpoints.end() ; item++) {
        if (*item == bp) {
            breakpoints.erase (item) ;
            break ;
        }
    }
    if (bp->is_sw_watchpoint()) {
        for ( BreakpointList::iterator item = sw_watchpoints.begin() ; item != sw_watchpoints.end() ; item++) {
            if (*item == bp) {
                sw_watchpoints.erase (item) ;
                break ;
            }
        }
    }
    //print_bps() ;
}

void Process::delete_breakpoint(int num) {
    Breakpoint * bp = NULL ;
    if (num == 0) {            // all breakpoints
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
            bp = *bpi ;
            if (bp->is_user()) {
                if (bp == hitbp) {
                    hitbp = NULL ;
                }       
                if (state != EXITED && state != IDLE) {
                    bp->clear() ;
                }
                delete bp ;            
            }
        }
        breakpoints.clear() ;
        sw_watchpoints.clear() ;
        bpmap.clear() ;
    } else {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
            if ((*bpi)->get_num() == num) {
                bp = *bpi ;
                break ;
            }
        }
        if (bp == NULL) {
           throw Exception ("No breakpoint number %d.", num) ;
        } else {
            remove_breakpoint (bp) ;
            if (state != EXITED && state != IDLE) {
                bp->clear() ;
            }
            if (bp == hitbp) {
                hitbp = NULL ;
            }
            delete bp ;
        }
    }
}

void Process::disable_breakpoint(int num) {
    bool hitone = false ;
    for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
        if ((num == 0  && (*bpi)->is_user()) || (*bpi)->get_num() == num) {
            Breakpoint *bp = *bpi ;
            hitone = true ;
            bp->disable() ;
        }
    }
    if (!hitone) {
        throw Exception ("No breakpoint number %d.", num) ;
    }
}

void Process::clear_breakpoints(Address addr) {
    std::vector<Breakpoint*> bps ;
   
    for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
        if ((*bpi)->is_user() && (*bpi)->get_address() == addr) {
            Breakpoint *bp = *bpi ;
            bps.push_back (bp) ;
        }
    }
    if (bps.size() == 0) {
        throw Exception ("No breakpoint at this address") ;
    }
    if (bps.size() > 0) {
        os.print ("Deleted breakpoint%s", bps.size() == 1?"":"s") ;
        for (uint i = 0 ; i < bps.size() ; i++) {
            os.print (" %d", bps[i]->get_num()) ;
        }
        os.print ("\n") ;
    }
    for (uint i = 0 ; i < bps.size() ; i++) {
        if (hitbp == bps[i]) {
            hitbp = NULL ;
        }
        remove_breakpoint (bps[i]) ;
        bps[i]->clear() ;
        delete bps[i] ;
    }
}

void Process::enable_breakpoint(int num) {
    bool hitone = false ;
    for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
        if ((num == 0  && (*bpi)->is_user()) || (*bpi)->get_num() == num) {
            Breakpoint *bp = *bpi ;
            hitone = true ;
            bp->enable(false) ;
        }
    }
    if (!hitone) {
        throw Exception ("No breakpoint number %d.", num) ;
    }
}

void Process::set_breakpoint_disposition(int num, Disposition disp) {
    bool hitone = false ;
    for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
        if ((num == 0  && (*bpi)->is_user()) || (*bpi)->get_num() == num) {
            Breakpoint *bp = *bpi ;
            hitone = true ;
            bp->set_disposition(disp) ;
        }
    }
    if (!hitone) {
        throw Exception ("No breakpoint number %d.", num) ;
    }
}

void Process::apply_breakpoints() {
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
           Breakpoint *bp = *bpi ;
           if (!bp->is_disabled() && !bp->is_removed() && !bp->is_pending()) {
               try {
                   bp->set() ;
               } catch (Exception e) {
                   os.print ("Deferring breakpoint due to %s\n", e.get().c_str()) ;
               }
           }
        }
    }
}

void Process::tempremove_breakpoints (Address addr) {
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
           Breakpoint *bp = *bpi ;
           if (bp->get_address() == addr && !bp->is_disabled() && !bp->is_removed()) {
               bp->tempremove() ;
           }
        }
    }
}


void Process::temprestore_breakpoints (Address addr) {
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
           Breakpoint *bp = *bpi ;
           if (bp->get_address() == addr && !bp->is_disabled() && bp->is_removed()) {
               bp->temprestore() ;
           }
        }
    }
}

void Process::detach_breakpoints() {
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
           Breakpoint *bp = *bpi ;
           if (!bp->is_disabled() && !bp->is_removed()) {
               bp->detach() ;
           }
        }
    }
}


// these are used for fork/exec handling
void Process::detach_breakpoints (int newpid) {
    int savedpid = pid ;
    pid = newpid ;
    (*current_thread)->set_pid (newpid) ;
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
           Breakpoint *bp = *bpi ;
           if (!bp->is_disabled() && !bp->is_removed()) {
               bp->detach() ;
           }
        }
    }
    (*current_thread)->set_pid (savedpid) ;
    pid = savedpid ;
}

void Process::attach_breakpoints (int newpid) {
    int savedpid = pid ;
    pid = newpid ;
    (*current_thread)->set_pid (newpid) ;
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
           Breakpoint *bp = *bpi ;
           if (!bp->is_disabled() && !bp->is_removed()) {
               bp->attach() ;
           }
        }
    }
    (*current_thread)->set_pid (savedpid) ;
    pid = savedpid ;
}

static Address get_number (Process *proc, std::string expr, int def, int &end) {
    if (expr.size() > 0) {
        int start = end ;
        if (end != 0) {
            expr = expr.substr(end) ;
        }
        Address res = proc->evaluate_expression(expr, end, true) ;
        end += start ;
        return res ;
    }
    return def ;
}
                                                                                                                                                        
void Process::resolve_pending_breakpoints() {
    if (breakpoints.size() > 0) {
        for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
            Breakpoint *bp = *bpi ;
            if (bp->is_pending()) {
               try {
                   std::string text = bp->get_text() ;
                   int end = 0 ;
                   Address addr = 0 ;
                   if (text[0] == '*') {
                        end = 1 ;
                        addr = get_number (this, text, 0, end) ;
                        if (!test_address (addr)) {
                            throw false ;
                        }
                    } else {                            // check for line, filename:line
                        std::string::size_type colon ;
                        if (text[0] != '\'' && (colon = text.find (':')) != std::string::npos && text[colon+1] != ':') {                // disallow ::
                            std::string filename = text.substr (0, colon) ;
                            end = colon + 1 ;
                            if (isdigit (text[end]) || text[end] == '$') {
                                int lineno = get_number (this, text, 0, end) ;
                                int start = lineno ;
                                do {
                                    addr = this->lookup_line (filename, lineno) ;
                                    lineno += 1;
                                } while (addr == 0 && lineno < (start + 100)) ;
                                if (addr == 0) {
                                    throw false ;
                                }
                            } else {
                                std::string func ;
                                while (end < (int)text.size() && isspace (text[end])) end++ ;
                                while (end < (int)text.size() && !isspace (text[end])) {
                                    func += text[end++] ;
                                }
                                addr = this->lookup_function (func, filename, true) ;
                                if (addr == 0) {
                                    throw false ;
                                }
                            }
                        } else if (isdigit (text[0]) || text[0] == '$') {         // simple line number?
                            end = 0 ;
                            int lineno = get_number(this, text, 0, end) ;
                            int start = lineno ;
                            do {
                                addr = this->lookup_line (lineno) ;
                                lineno++ ;
                            } while (addr == 0 && lineno < (start + 100)) ;
                            if (addr == 0) {
                                throw false ;
                            }
                        } else if (text[0] == '+' || text[0] == '-') {      // relative line number
                            end = 1 ;
                            int delta = get_number(this, text, 0, end) ;
                            int sdelta = delta ;
                            do {
                                addr = this->lookup_line (current_location.get_line() + delta) ;
                                delta += 1 ;
                            } while (addr == 0 && delta < (sdelta + 100)) ;
                            if (addr == 0) {
                                throw false ;
                            }
                       } else {
                            std::string func ;
                            if (text[0] == '\'' || text[0] == '"') {
                               char term = text[0] ;
                               end = 1 ;
                               while (end < (int)text.size() && text[end] != term) {
                                    end++ ;
                               }
                               if (end == (int)text.size()) {
                                   func = text.substr(1) ;
                               } else {
                                   func = text.substr (1, end - 1) ;
                                   end++ ;
                               }
                            } else {
                                end = 0 ;
                                while (end < (int)text.size() && isspace (text[end])) end++ ;
                                while (end < (int)text.size() && !isspace (text[end])) {
                                    func += text[end++] ;
                                }
                            }
                            func = pcm->realname (func) ;
                            addr = lookup_function (func, "", true) ;
                            if (addr == 0) {
                                throw false ;
                            }
                       }
                   }
                   if (addr != 0) {
                       // set the address
                       bp->set_address (addr) ;

                       // add it to the breakpoint list
                       BreakpointMap::iterator i = bpmap.find (addr) ;           // already a breakpoint at that address?
                       BreakpointList *bplist = NULL ;
                       if (i == bpmap.end()) {
                           bplist = new BreakpointList() ;
                           bpmap[addr] = bplist ;
                       } else {
                           bplist = i->second ;
                       }
                       bplist->push_back (bp) ;

                       // apply it
                       bp->set() ;

                       // now mark it as non-pending
                       bp->set_pending(false) ;

                       // and tell the user
                       os.print ("Breakpoint %d at 0x%llx", bp->get_num(), addr) ;
                       bp->print_short_location(os) ;
                       os.print ("\n") ;
                       os.print ("Pending breakpoint \"%s\" resolved.\n", bp->get_text().c_str()) ;
                   }
               } catch (...) {
               }
            }
        }
    }
}


void Process::sync() {
    sync_threads() ;
    apply_breakpoints() ;
}


DIE * Process::find_scope(std::string name, bool topdown) {
    DIE *die = NULL ;
    if (topdown) {
        for (uint i = 0 ; i < objectfiles.size(); i++) {
            ObjectFile *file = objectfiles[i] ;
            if (file->symtab != NULL) {
                DIE * sym = file->symtab->find_scope (name) ;
                if (sym != NULL) {
                    return sym ;
                }
            }
        }
    } else {
        build_frame_cache() ;
        if (frame_cache_valid) {              // is there a current frame?
            Location &loc = frame_cache[current_frame]->get_loc() ;
            if (loc.get_funcloc() != NULL) {
                die = loc.get_funcloc()->symbol->die ;
            } 
        } else {
            if (current_location.get_funcloc() != NULL) {
                die = current_location.get_funcloc()->symbol->die ;
            }
        }
 
        if (die != NULL) {
            DIE * sym = die->find_scope (name) ;
            if (sym != NULL) {
                return sym ;
            }
        }
    }
    return NULL ;
}


// main symbol search function->  Look in current location first, followed
// by globally
// returns a DIE for the symbol or an integer->  The caller must deal with both
// being returned
// an integer means that the symbol translated directly into an address in the ELF symbol table
// can also return NULL


// reqpc is the pc for the requested symbol (syntactically @line in the expression handler)
SymbolValue Process::find_symbol(std::string name, Address reqpc) {
    build_frame_cache() ;
    Address pc = reqpc ;
    if (pc == 0) {
        if (threads.size() != 0) {
            pc = get_reg("pc") ;                // pc is required for lexical blocks
        }
    }
    DIE *die = NULL ;
    if (frame_cache_valid) {              // is there a current frame?
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.get_funcloc() != NULL) {
            die = loc.get_funcloc()->symbol->die ;
        } 
    } else {
        if (current_location.get_funcloc() != NULL) {
            die = current_location.get_funcloc()->symbol->die ;
        }
    }
  
    if (die != NULL) {
        std::vector<DIE *> syms ;
        die->find_symbol (name, pc, syms) ;
        if (!syms.empty()) {
            return syms ;
        }
    }

    // not in the local frame, look globally (XXX: C++ class scope)
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (file->symtab != NULL) {
            DIE * sym = file->symtab->find_symbol (name) ;
            if (sym != NULL) {
                return sym ;
            }
        }
    }
    // not in the DWARF symbol tables, need to look in ELF
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        Address addr = file->elf->find_symbol (name, is_case_blind()) ;
        if (addr != 0) {
            return addr ;
        }
    }
    return SymbolValue() ;
}

FDE *Process::find_fde (Address addr) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (file->symtab != NULL) {
            FDE *fde = file->symtab->find_fde (addr) ;
            if (fde != NULL) {
                return fde ;
            }
        }
    }
    return NULL ;
}


Section * Process::lookup_symbol_section(std::string name) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        Section * section = file->elf->find_symbol_section (name, is_case_blind()) ;
        if (section != NULL) {
            return section ;
        }
    }

    return NULL ;
}

Section * Process::find_section_at_addr(Address addr) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        Section * section = file->elf->find_section_at_addr (addr) ;
        if (section != NULL) {
            return section ;
        }
    }

    return NULL ;
}

// note: this is called by get_main_language(), which is called (indirectly) by is_case_blind()
// so we can't do a case blind search here
Address Process::lookup_symbol(std::string name, std::string objectfile) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (objectfile == "" || file->name == objectfile) {
            Address addr = file->elf->find_symbol (name) ;
            if (addr != 0) {
                return addr ;
            }
        }
    }

    return 0 ;
}

// get a list of all the functions matching the name 
// for a C or FORTRAN program this will only be 1, but for C++
// there may be many

void Process::enumerate_functions (std::string name, std::vector<std::string> &results) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (file->symtab != NULL) {
            file->symtab->enumerate_functions (name, results) ;
        }
    }
}

DIE *Process::find_compilation_unit (std::string filename) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        DwCUnit *cu = file->symtab->find_compilation_unit (filename) ;
        if (cu != NULL) {
            return cu->get_cu_die() ;
        }
    }
    return NULL ;
}

// lookup the address of a function, and skip the preamble stuff.
Address Process::lookup_function(std::string name, std::string filename, bool skip_preamble) {
    // looking for a function in a specific file means looking in the dwarf info
    if (filename != "") {
        for (uint i = 0 ; i < objectfiles.size(); i++) {
            ObjectFile *file = objectfiles[i] ;
            DwCUnit *cu = file->symtab->find_compilation_unit (filename) ;
            if (cu != NULL) {
                std::vector<DIE*> syms ;
                cu->get_cu_die()->find_symbol (name, 0, syms) ;
                if (!syms.empty()) {
                    DIE *die = syms[0] ;                // XXX: present menu

                    if (die != NULL && die->get_tag() == DW_TAG_subprogram) {
                        Address addr = (Address)die->getAttribute (DW_AT_low_pc) ;
                        if (addr != 0 && skip_preamble) {
                            Address addr1 = file->symtab->skip_preamble (addr) ;
                            if (addr1 != 0) {
                                addr = addr1 ;
                            }
                            if (state != IDLE && addr == 0) {
                                addr = arch->skip_preamble (this, addr) ;
                            }
                            return addr ;
                        } else {
                            return addr ;
                        }
                    }
                }
            }
        }
        return 0 ;
    }


    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        Address addr = file->elf->find_symbol (name, is_case_blind()) ;
        if (addr != 0) {
            if (skip_preamble && file->symtab != NULL) {         // debug information for it?
                Address addr1 = file->symtab->skip_preamble (addr) ;
                if (addr1 != 0) {
                    addr = addr1 ;
                }
            } 
            if (state != IDLE && addr == 0 && skip_preamble) {
                addr = arch->skip_preamble (this, addr) ;
            }
            return addr ;
        }
    }

    return 0 ;
}

bool Process::is_case_blind() {
    bool autolang ;
    int language = get_language (autolang) ;
    return language == DW_LANG_Fortran77 || language == DW_LANG_Fortran90 ;
}

Location Process::lookup_address(Address addr, bool guess) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (file->symtab != NULL) {
            Location loc = file->symtab->find_address (addr, guess) ;
            if (loc.get_symname() != "") {           // found it?
                return loc ;
            }
        }
    }

    Location loc;
    loc.set_addr(addr);
    return loc;
}


Address Process::lookup_line (std::string filename, int line) {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        if (file->symtab != NULL) {
            Address addr = file->symtab->find_line (filename, line) ;
            if (addr != 0) {
               return addr ;
            }
        }
    }
    return 0 ;
}

// line in current file
Address Process::lookup_line (int line) {
    build_frame_cache() ;
    Location loc ;
    if (frame_cache_valid) {              // is there a current frame?
        loc = frame_cache[current_frame]->get_loc() ;
    } else {
        loc = current_location ;
    }
    return lookup_line (loc.get_filename(), line) ;
}


// call frame handling

// this is a single stack frame->  It holds information about the frame contents
// such as the frame pointer and the register values->  All the registers are
// initially clean and are only published to a thread if they have been modified

// invalidate the frame cache so that it will be build anew next time we stop
void Process::invalidate_frame_cache() {
    frame_cache_valid = false ;
    for (uint i = 0 ; i < frame_cache.size() ; i++) {
        delete frame_cache[i] ;
    }
    frame_cache.clear() ;  
    current_frame = -1 ;
}

bool Process::check_code_address (Address pc) {
    if (!is_valid_code_address (pc)) {
         return false ;
    }
    return true ;
}

void Process::build_frame_cache() {
    if (frame_cache_valid ||
        state == IDLE ||
        state == EXITED)  {
       return;
    }

    /* we want to persist errors */
    try {

    /* setup stack iterators */
    int n = 0;
    frame_corrupted = false;

    /* find current registers */
    sync();
    Address pc = get_reg("pc");
    Address sp = get_reg("sp");
    Address fp = get_reg("fp");
    Location loc = lookup_address (pc);

    /* instantiate top frame */
    Frame *frame = new Frame (this, arch, n++, loc, pc, sp, fp);
    frame_cache.push_back (frame);

    /* iterate up through stack frames */
    while (loc.get_symname() != "main" && frame->get_fp() != 0) {
        Frame *nframe;
        FDE* fde;

        /* don't follow a corrupt pc */
        if ( !test_address(pc) ) {
            frame_corrupted = true;
            break;
        }

        nframe = new Frame (this, arch, n++);

        /* first check for signal trampoline */
        if (arch->in_sigtramp(this, loc.get_symname())) {
            RegisterSet *r = nframe->get_regs();
            arch->get_sigcontext_frame(this, sp, r);
            goto next_iteration;
        } 

        /* then check for an FDE entry */
        fde = find_fde (pc);
        if (fde != NULL) {
            execute_fde (fde, pc, frame, nframe, false); 
            goto next_iteration;
        }

        /* no luck, fall back on frame pointer */
        arch->guess_frame (this, frame, nframe);

next_iteration:

        /* update the loop values */
        pc = nframe->get_pc();
        sp = nframe->get_sp();
        fp = nframe->get_fp();
        loc = lookup_address (pc);
        nframe->set_loc (loc);

        /* check pc before pushing */
        if ( !test_address(pc) )  {
           frame_corrupted = true;
           delete nframe;
           break;
        }

        frame_cache.push_back (nframe);
        frame = nframe;
    }

    /* fall out of frame building */
    } catch (Exception e) {
        /* ignore errors.  they are likely just
         * unable to access memory errors b/c a
         * stack corruption went unnoticed.
         */

        /* e.report(std::cerr); */
    }

    /* mark frame as read */
    frame_cache_valid = true;
    current_frame = 0;
}


void Process::execute_cfa (Architecture *arch, CFATable *table,
   BVector code, Address pc, int caf, int daf, int ra, bool debug) {
    BStream stream (code, ! arch->is_little_endian()) ;
    while (!stream.eof() && table->get_loc() <= pc) {
        int opcode = stream.read1u() ;
        if ((opcode & 0xc0) != 0) {
            switch (opcode & 0xc0) {
            case DW_CFA_advance_loc:
                table->advance_loc ((opcode &0x3f) * caf) ;
                if (debug) {
                    printf ("DW_CFA_advance_loc (%d*%d to 0x%llx\n",
			    opcode&0x3f,
			    caf,
			    (unsigned long long) table->get_loc()) ;
                }
                break ;
            case DW_CFA_offset: {
                int reg = opcode &0x3f ;
                int offset = stream.read_uleb() * daf ;
                table->set_reg (reg, CFA_OFFSET, offset) ;
                if (debug){
                    std::cout << "DW_CFA_offset: setting reg ";
                    std::cout <<  reg  <<  " to CFA(" ;
                    std::cout << table->get_cfa_reg()  <<  " << ";
                    std::cout <<  table->get_cfa_offset()  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_restore: {
                int reg = opcode & 0x3f ;
                table->restore (reg) ;
                break ;
                }
            }
        } else {
            switch (opcode) {
            case DW_CFA_nop:
                break ;
            case DW_CFA_set_loc: {
                Address loc = 0 ;
                if (is_64bit()) {
                    loc = stream.read8u() ;
                } else {
                    loc = stream.read4u() ;
                }
                table->set_loc (loc) ;
                if (debug) {
                    printf ("DW_CFA_set_loc: to 0x%lx\n",
			    (unsigned long) loc) ;
                }
                break ;
            }
            case DW_CFA_advance_loc1: {
                int delta = stream.read1u() ;
                table->advance_loc (delta) ;
                if (debug) {
                    printf ("DW_CFA_advance_loc1: to 0x%lx\n",
			    (unsigned long) table->get_loc()) ;
                }
                break ;
                }
            case DW_CFA_advance_loc2: {
                int delta = stream.read2u() ;
                table->advance_loc (delta) ;
                if (debug) {
                    printf ("DW_CFA_advance_loc2: to 0x%lx\n",
			    (unsigned long) table->get_loc()) ;
                }
                break ;
                }
            case DW_CFA_advance_loc4: {
                int delta = stream.read4u() ;
                table->advance_loc (delta) ;
                if (debug) {
                    printf ("DW_CFA_advance_loc4: to 0x%lx\n",
			    (unsigned long) table->get_loc()) ;
                }
                break ;
                }
            case DW_CFA_offset_extended: {
                int reg = stream.read_uleb() ;
                int offset = stream.read_uleb() * daf ;
                table->set_reg (reg, CFA_OFFSET, offset) ;
                if (debug) {
                    std::cout << "DW_CFA_offset_extended: setting reg "  <<  reg  <<  " to CFA("  <<  table->get_cfa_reg()  <<  "/"  <<  table->get_cfa_offset()  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_restore_extended:
                throw Exception ("DW_CFA_restore_extended not implemented") ;
            case DW_CFA_undefined: {
                int reg = stream.read_uleb() ;
                table->set_reg (reg, CFA_UNDEFINED, 0) ;
                break ;
                }
            case DW_CFA_same_value: {
                int reg = stream.read_uleb() ;
                table->set_reg (reg, CFA_SAME_VALUE, 0) ;
                break ;
                }
            case DW_CFA_register: {
                int reg1 = stream.read_uleb() ;
                int reg2 = stream.read_uleb() ;
                table->set_reg (reg1, CFA_REGISTER, reg2) ;
                if (debug) {
                    std::cout << "DW_CFA_register: setting reg "  <<  reg1  <<  " to "  <<  reg2 << '\n' ;
                }
                break ;
                }
            case DW_CFA_remember_state:
                throw Exception ("DW_CFA_remember_state not implemented") ;
            case DW_CFA_restore_state:
                throw Exception ("DW_CFA_restore_state not implemented") ;
            case DW_CFA_def_cfa: {
                int reg = stream.read_uleb() ;
                int offset = stream.read_uleb() ;
                table->set_cfa (reg, offset) ;
                if (debug) {
                    std::cout << "DW_CFA_def_cfa: to ("  <<  reg  <<  "/"  <<  offset  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_def_cfa_register: {
                int reg = stream.read_uleb() ;
                table->set_cfa_reg (reg) ;
                if (debug) {
                    std::cout << "DW_CFA_def_cfa_register: to ("  <<  reg  <<  " << "  <<  table->get_cfa_offset()  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_def_cfa_offset: {
                int offset = stream.read_uleb() ;
                table->set_cfa_offset (offset) ;
                if (debug) {
                    std::cout << "DW_CFA_def_cfa_offset: to ("  <<  table->get_cfa_reg()  <<  " << "  <<  offset  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_def_cfa_expression:
                throw Exception ("DW_CFA_expression not implemented") ;
            case DW_CFA_expression:
                throw Exception ("DW_CFA_expression not implemented") ;
            case DW_CFA_offset_extended_sf: {
                int reg = arch->translate_regnum (stream.read_uleb()) ;
                int offset = stream.read_sleb() * daf ;
                table->set_reg (reg, CFA_OFFSET, offset) ;
                if (debug) {
                    std::cout << "DW_CFA_offset_extended_sf: setting reg "  <<  reg  <<  " to CFA("  <<  table->get_cfa_reg()  <<  " << "  <<  table->get_cfa_offset()  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_def_cfa_sf: {
                int reg = stream.read_uleb() ;
                int offset = stream.read_sleb() ;
                table->set_cfa (reg, offset) ;
                if (debug) {
                    std::cout << "DW_CFA_def_cfa_sf: to ("  <<  table->get_cfa_reg()  <<  " << "  <<  table->get_cfa_offset()  <<  ")" << '\n' ;
                }
                break ;
                }
            case DW_CFA_def_cfa_offset_sf: {
                int offset = stream.read_sleb() ;
                table->set_cfa_offset (offset) ;
                if (debug) {
                    std::cout << "DW_CFA_def_cfa_offset_sf: to ("  <<  table->get_cfa_reg()  <<  " << "  <<  table->get_cfa_offset()  <<  ")" << '\n' ;
                }
                break ; 
                }
#ifdef DW_CFA_MIPS_advance_loc8
            case DW_CFA_MIPS_advance_loc8: {
                Address delta = stream.read8u() ;
                table->advance_loc (delta) ;
                if (debug) {
                    printf ("DW_CFA_MIPS_advance_loc8: to 0x%lx\n",
			    (unsigned long) delta) ;
                }
                break ;
                }
#endif
#ifdef DW_CFA_GNU_window_save
            case DW_CFA_GNU_window_save:
                throw Exception ("DW_CFA_GNU_window_save not implemented") ;
#endif
#ifdef DW_CFA_GNU_args_size
            case DW_CFA_GNU_args_size: {
                // XXX: ignore until I know what to do with it
                int size = stream.read_uleb();
		(void) size;
                break ;
                }
#endif
            default:
                throw Exception ("Unknown CFA opcode: %x",opcode) ;
            }
        }
    }
}

void Process::execute_fde(FDE * fde, Address pc, Frame * from, Frame *to, bool debug) {
    int caf = 0 ;// code alignment factor
    int daf = 0 ;// data alignment factor
    int ra = 0 ;// return address register
    CIE * cie = fde->get_cie() ;

    err_note("FDE: initial location 0x%lx\n",
       fde->get_start_address());

    if (cie != NULL) {
        caf = cie->get_code_align() ;
        daf = cie->get_data_align() ;
        ra = cie->get_ra() ;
    }

    CFATable table (arch, this, fde->get_start_address(), ra) ;

    //std::cout << "executing FDE" << '\n' ;

    if (cie != NULL) {
        execute_cfa (arch, &table, cie->get_instructions(),
          pc, caf, daf, ra, debug) ;
        table.save() ;
    }
    execute_cfa (arch, &table, fde->get_instructions(),
       pc, caf, daf, ra, debug) ;
    if (debug) {
        table.print() ;
    }
    table.apply (from, to) ;
}

Address Process::get_fde_return_address (FDE *fde, Address pc, Frame *frame) {
    //printf ("getting return address from FDE\n") ;
    int caf = 0 ;// code alignment factor
    int daf = 0 ;// data alignment factor
    int ra = 0 ;// return address register
    CIE * cie = fde->get_cie() ;

    if (cie != NULL) {
        caf = cie->get_code_align() ;
        daf = cie->get_data_align() ;
        ra = cie->get_ra() ;
    }

    CFATable table (arch, this, fde->get_start_address(), ra) ;

    //std::cout << "executing FDE" << '\n' ;

    if (cie != NULL) {
        execute_cfa (arch, &table, cie->get_instructions(),
          pc, caf, daf, ra, false) ;
    }
    execute_cfa (arch, &table, fde->get_instructions(),
       pc, caf, daf, ra, false) ;
    //table.print() ;
    //printf ("return address is in register %d\n", ra) ;
    return frame->get_reg (arch->translate_regnum (ra)) ;
}


Frame *Process::get_current_frame() {
    build_frame_cache() ;
    return frame_cache[current_frame] ;
}

void Process::set_frame(int frameno) {
    build_frame_cache() ;
    if (frameno < 0 || (uint) frameno >= frame_cache.size()) {
       os.print ("Frame number is out of range.\n") ;
       return ;
    }
    //std::cout << "setting frame to "  <<  frameno << '\n' ;
    current_frame = frameno ;
    frame_cache[current_frame]->publish_regs ((*current_thread)) ;
    Address pc = get_reg ("pc") ;
    Address adjusted_pc = pc ;
    // if the pc is at a return address of a function (that is, not the first frame)
    // then it will point to the instruction after the call.  This will probably be
    // an other line, so we need to move it back a little (to the call or close to it)
    // so that the line prints correctly
    if (current_frame != 0) {
        int max = arch->max_call_size() ;
        int min = arch->min_call_size() ;
        for (int i = max ; i >= min ; i--) {
            if (arch->is_call (this, pc-i)) {
                adjusted_pc -= i ;
                break ;
            }
        }
    }
    Location loc = lookup_address (adjusted_pc) ;
    // put pc back so that the address prints correctly
    loc.set_addr(pc);
    print_loc(loc, frame_cache[current_frame], os) ;
    loc.show_line(os, get_cli()->isemacs()) ;
    set_current_line (loc.get_line()) ;
}

void Process::show_frame() {
    build_frame_cache() ;
    Address pc = get_reg ("pc") ;
    Location loc = lookup_address (pc) ;
    print_loc(loc, frame_cache[current_frame], os) ;
    loc.show_line(os, get_cli()->isemacs()) ;
}


void Process::up(int n) {
    build_frame_cache() ;

    // if we are asked to go up by more than one frame, limit it to take us to the
    // top
    if (n > 1) {
        if (uint(current_frame + n) >= frame_cache.size()) {
            n = frame_cache.size() - current_frame - 1 ;
        }
        set_frame (current_frame + n) ;
    } else {
        if (current_frame == (int) frame_cache.size() - 1) {
           os.print ("Initial frame selected; you cannot go up.\n") ;
        } else {
            set_frame (current_frame + 1) ;
        }
    }
}

void Process::down(int n) {
    build_frame_cache() ;
    if (n > 1) {
        if ((current_frame - n) < 0) {
            n = current_frame ;
        }
        set_frame (current_frame - n) ;
    } else {
        if (current_frame == 0) {
            os.print ("Bottom (i.e., innermost) frame selected; you cannot go down.\n") ;
        } else {
            set_frame (current_frame - 1) ;
        } 
    }
}

void Process::dump(Address addr, int size) {
    if (threads.size() == 0) {
        throw Exception ("An active process is required for this operation") ;
    }
    target->dump (os, (*current_thread)->get_pid(), addr, size) ;
}

std::string Process::read_string(Address addr) {
    return target->read_string ((*current_thread)->get_pid(), addr) ;
}

std::string Process::read_string(Address addr, int len) {
    return target->read_string ((*current_thread)->get_pid(), addr, len) ;
}

LinkMap * Process::find_link_map(Address addr) {
   for (uint map = 0 ; map < linkmaps.size(); map++) {
       if (linkmaps[map]->get_addr() == addr) {
            return linkmaps[map] ;
       }
   }
   return NULL ;
}

void Process::load_link_map(Address addr) {
    //printf ("loading link map %llx\n", addr) ;
    for (;;) {
       LinkMap * lm = new LinkMap(arch, this, addr) ;
       linkmaps.push_back (lm) ;
       //lm->print() ;
       addr = lm->get_next() ;
       if (addr == 0) {
          break ;
       }
    }
}

LinkMap * Process::get_new_link_map() {
    int ps = arch->ptrsize() ;
    Address linkmap = readptr (r_debug+ps) ;
    while (linkmap != 0) {
        LinkMap * map = find_link_map (linkmap) ;
        if (map == NULL) {
            LinkMap * lm = new LinkMap (arch, this, linkmap) ;
            linkmaps.push_back (lm) ;
            //lm->print() ;
            return lm ;
        }
        linkmap = readptr (linkmap+ps+ps+ps) ;
    }
    return NULL ;
}

Address Process::get_r_debug_state() {
    int ps = arch->ptrsize() ;
    return readptr (r_debug+ps+ps+ps)                         ;
}

bool Process::is_running() {
    return (attach_type != ATTACH_CORE) && !(state == IDLE || state == EXITED) ;
}

bool Process::is_active() {
    return (attach_type == ATTACH_CORE) || !(state == IDLE || state == EXITED) ;
}

void Process::load_dynamic_info(bool set_break) {
    Address _DYNAMIC = lookup_symbol ("_DYNAMIC") ;

    try {
    if (_DYNAMIC != 0) {
        ELF * elf = objectfiles[0]->elf ;// get the ELF file for the main object file

        Address dyn = _DYNAMIC ;
        //printf ("dyn = 0x%llx\n", dyn) ;
        int tag = 0 ;
        int tagoffset = elf->is_elf64() ? 8 : 4 ;
        while ((tag = readelfxword (elf, dyn)) != 0) { 
            //printf ("tag = %d\n", tag) ;
            if (tag == 6) {         // SYMTAB
                Address symtab = readptr (dyn+tagoffset) ;
		(void) symtab;
                //println ("symbol table = " + format ("0x%x", symtab))
                //dump (symtab, 100) ;
            } else if  (tag == 21) {            // DEBUG
                //dump (dyn, 32) ;
                r_debug = readptr (dyn+tagoffset) ;
                //printf ("r_debug = 0x%llx\n", r_debug) ;
                int r_map_align = arch->ptrsize() == 8 ? 8 : 4 ;
                Address linkmap = readptr (r_debug+r_map_align) ;          // alignment
                load_link_map (linkmap) ;
                if (set_break) {
                    Address r_brk = readptr(r_debug+arch->ptrsize() + r_map_align) ;
                    //println ("r_brk = " + format ("%x", r_brk))
                    new_breakpoint (BP_DSO, "", r_brk) ;
                }
            }
            dyn += tagoffset + arch->ptrsize()  ;
        }
    }} catch (Exception e) {
       e.report(std::cerr);
    }

    try {
    for (uint i = 0 ; i < linkmaps.size(); i++) {
        LinkMap *lm = linkmaps[i] ;
        std::string name = lm->get_name() ;
        if (name != "") {
            Address base = lm->get_base() ;
            //os.print ("link map base = 0x%llx\n", base) ;
            //printf ("opening object file %s\n", name.c_str()) ;
            if (attach_type == ATTACH_CORE) {
                printf ("Reading symbols from %s...", name.c_str()) ;
                fflush(stdout) ;
            }
            try {
               open_object_file (name, base) ;
               if (attach_type == ATTACH_CORE) {
                   printf ("done\n") ;
               }
            } catch (Exception e) {
               // Core files should ignore missing libraries
               if (attach_type == ATTACH_CORE) {
                  e.report(std::cout);
               } else {
                  throw e;
               }
            }
        }
    }} catch (Exception e) {
       e.report(std::cerr);
    }

}


// run the target with the args
bool Process::run(const std::string& args, EnvMap& env) {
    // print GDB style information line
    os.print ("Starting program: %s %s\n", program.c_str(), args.c_str()) ;

    if (program == "") {
        os.print ("No executable file specified.\n") ;
        os.print ("Use the \"file\" command.\n") ;
        return false ;
    }

    //println ("spawining program")
    pid = target->attach (program.c_str(), args.c_str(), env) ;            // attach to target

    //std::cout << "waiting for process " << pid << "\n" ;
    //println ("waiting for " + pid)
    threads.push_front (new Thread (arch, this, pid, 0)) ;                // main thread
    current_thread = threads.begin() ;
    invalidate_frame_cache() ;                       // frame cache is not valid until we stop
    init_phase = true ;
    state = RUNNING ;
    wait() ;

/* only if spawning shell */
#if 1
    // wait for shell to exec
    sync_threads() ;
    target->cont (pid, 0) ;            // don't call docont() as it does apply_breakpoint()
    state = RUNNING ;
    wait() ;
#endif

    // run to main
    // if we don't have a symbol table then we can't insert the temp breakpoint at main
    // and we can't load the dynamic information.  This might happen if we are debugging
    // a stripped program in assembly mode.
    if (main != 0) {
        new_breakpoint (BP_TEMP, "", main) ;

        //printf ("ready\n") ;
        apply_breakpoints() ;
        if (docont()) {
            wait() ;                          // will stop at main
        }

        // load the dynamic info stuff
        load_dynamic_info(true) ;
    } else {
        apply_breakpoints() ;
    }

    // enable fork/vfork/exec event handling
    target->init_events (pid) ;

    sync() ;
    try {
        if (!thread_db_initialized) {
            thread_db::load_thread_db(pcm) ;
        }
        thread_db::new_td_handle (pid, thread_agent, creation_bp, death_bp) ;

        new_breakpoint (BP_THR_CREATE, "", creation_bp) ;
        new_breakpoint (BP_THR_DEATH, "", death_bp) ;

        // remove the single-threaded dummy thread
        delete *current_thread ;
        threads.erase (current_thread) ;

        std::vector<void*> thrds ;
        thread_db::list_threads (thread_agent, thrds) ;
        //std::cout << "THREADS:" << '\n' ;
        for (uint i = 0 ; i < thrds.size(); i++) {
            thread_db::enable_thread_events (thread_agent, thrds[i], 1) ;
            int thr_pid = pid ;
            void *tid = target->get_thread_tid (thread_agent, thrds[i], thr_pid) ;
            Thread *thr = new Thread (arch, this, thr_pid, tid) ;
            threads.push_front (thr) ;
            thr->syncin() ;
        }
        multithreaded = true ;
        current_thread = threads.begin() ;
        thread_db_initialized = true ;
        printf ("Detected a multi-threaded program\n") ;
    } catch (...) {
            // ignore exceptions as they just mean that we are not multithreaded
		printf("threaddb failed\n");
    }

    resolve_pending_breakpoints() ;                     // resolve any pending breakpoints found so far

    init_phase = false ;

    // we will have hit a breakpoint at this point.  It will either be the 
    // temp bp we set above or a user bp.  If it's a user bp, we want
    // to stop, otherwise just continue.
    if (hitbp == NULL || !hitbp->is_user()) {
        return docont() ;
    }
    return false ;
}

void Process::list_breakpoints(bool all) {
    bool title = true ;
    for (BreakpointList::iterator bpi = breakpoints.begin() ; bpi != breakpoints.end(); bpi++) {
        Breakpoint *bp = *bpi ;
        if (all || bp->is_user()) {
            if (title) {
                os.print ("Num Type           Disp Enb Address            What\n") ;
                title = false ;
            }
            bp->print_details (os) ;
        }
    }
    if (title) {
        os.print ("No breakpoints or watchpoints.\n") ;
    }
}

void Process::list_symbols() {
    for (uint i = 0 ; i < objectfiles.size(); i++) {
        ObjectFile *file = objectfiles[i] ;
        os.print ("object file: %s\n", file->name.c_str()) ;
        file->elf->list_symbols(os) ;
    }
}

void Process::list_threads() {
    for (ThreadList::iterator t = threads.begin() ; t != threads.end() ; t++) {
        Thread *thr = *t ;
        os.print ("%c %d ", t==current_thread?'*':' ', thr->get_num()) ;
        thr->print (os) ;
        os.print ("  ") ;
        Address pc = thr->get_reg("pc") ;
        Location loc = lookup_address (pc) ;
        print_loc(loc, get_current_frame(), os) ;
        // XXX: GDB also prints 'from /lib64/tls/libpthread.so.0' and the like
    }
}

void Process::switch_thread(int n) {
    //std::cout << "switching to thread "  <<  n << '\n' ;
    ThreadList::iterator t ;
    for (t = threads.begin() ; t != threads.end() ; t++) {
        if ((*t)->get_num() == n) {
            break ;
        }
    }
    if (t == threads.end()) {
        throw Exception ("No such thread") ;
    }
    current_thread = t ;
    (*t)->syncin() ;

    os.print ("[Switching to thread %d (", n) ;
    (*t)->print (os) ;
    os.print (")]#0 ") ;                 // NB: GDB doesn't print space between ] and the frame number

    invalidate_frame_cache() ;

    build_frame_cache() ;
    Address pc = get_reg ("pc") ;
    Location loc = lookup_address (pc) ;
    print_loc(loc, frame_cache[current_frame], os) ;
    loc.show_line(os, get_cli()->isemacs()) ;
    set_current_line (loc.get_line()) ;
}

bool Process::stepping_stops(Address pc) {
   Location cur_loc = lookup_address(pc, false);
   if (!cur_loc.is_known() || cur_loc.equiv(last_loc)) {
      return false;
   }

   FunctionLocation* func = cur_loc.get_funcloc();
   if (func != NULL && pc != func->get_start_address()) {
      return true;
   }

   return false;
}

bool Process::test_address(Address addr) {
    if (threads.size() == 0) {
        return false ;
    }
    return target->test_address ((*current_thread)->get_pid(), addr) ;
}

// read memory and replace any breakpoints
Address Process::read(Address addr, int size) {
    Address tmp = target->read ((*current_thread)->get_pid(), addr, size) ;
    for (BreakpointList::reverse_iterator i = breakpoints.rbegin(); i != breakpoints.rend(); i++) {
        Breakpoint *bp = *i ;

        if ( !bp->is_software() ) continue;

        Address bpaddr = bp->get_address() ;
        if (bpaddr >= addr && bpaddr < addr + size) {       // address in range being read?
            SoftwareBreakpoint *abp = dynamic_cast<SoftwareBreakpoint*>(bp) ;
            int oldvalue = abp->get_old_value() ;
            int delta = bpaddr - addr ;                      // delta into data read
            memcpy ((char*)&tmp + delta, &oldvalue, arch->bpsize()) ;               // XXX: big endian?
        }
    }

    return tmp ;
}

// read memory directly from target
Address Process::raw_read(Address addr, int size) {
    return target->read ((*current_thread)->get_pid(), addr, size) ;
}

Address Process::readptr(Address addr) {
    return target->readptr ((*current_thread)->get_pid(), addr) ;
}

Address Process::readelfxword(ELF * elf, Address addr) {
   if (elf->is_elf64()) {
        return target->read ((*current_thread)->get_pid(), addr, 8) ;
   } else {
        return target->read ((*current_thread)->get_pid(), addr, 4) ;
   }
}

// step one instruction from the address of a breakpoint.  The breakpoint has already
// been removed and replaced by the original instruction.  We need to check if the
// instruction is a call instruction and step over it if requested.  The function returns
// when the process has stopped at the next instruction

void Process::step_from_breakpoint (Breakpoint * bp) {
    sync() ;
    Address pc = get_reg ("pc") ;                 // current PC value
    bool call = arch->is_call (this, pc) ;        // is the current instruction a call?
    
    //printf ("stepping from breakpoint %d\n", bp->get_num()) ;
    tempremove_breakpoints (pc);

    if (state == STEPPING && call && stepping_over) {                  // do we want to step over it?
        Address nextpc = pc + arch->call_size(this, pc) ;
        Breakpoint *newbp = new_breakpoint (BP_STEP, "", nextpc) ;                      // insert temporary bp at next instruction
	(void) newbp;
        //printf ("setting temp breakpoint at 0x%llx\n", (unsigned long long)nextpc) ;
        sync() ;
        target->cont ((*current_thread)->get_pid(), current_signal) ;
        current_signal = 0 ;
        state = RUNNING ;
        hitbp = NULL ;                           // no breakpoint active now
        wait() ;
    } else {
        //printf ("single stepping one instruction\n") ;
        target->step ((*current_thread)->get_pid()) ;
        state = ISTEPPING ;                       // stepping internally
        hitbp = NULL ;                           // no breakpoint active now
        wait() ;                                // wait for the process to stop
    }
    //printf ("re-enabling breakpoint at 0x%llx\n", bp->get_address()) ;
    temprestore_breakpoints (bp->get_address()) ;                      // reenable the breakpoint

//(*current_thread)->syncin();
// pc = get_reg ("pc") ;
}

// step one instruction, over a call if necessary.  This does not wait for the
// process to complete the step

// returns true if the instruction step was done by setting a breakpoint and
// contining to it
bool Process::step_one_instruction() {
    sync() ;

    (*current_thread)->go() ;                           // mark current thread as running

	if (hitbp)
		tempremove_breakpoints (hitbp->get_address()) ;

    if (multithreaded) {
        resume_threads() ;                              // restart all threads (except current)
    }

    Address pc = get_reg ("pc") ;                 // current PC value
    bool call = arch->is_call (this, pc) ;        // is the current instruction a call?
    //printf ("stepping one instruction, pc = 0x%llx, call = %d\n", (unsigned long long)pc, call) ;
    
    if (call && stepping_over) {                  // do we want to step over it?
        Address nextpc = pc + arch->call_size(this, pc) ;
        //printf ("nextpc = 0x%llx\n", nextpc) ;
        Breakpoint *newbp = new_breakpoint (BP_STEP, "", nextpc) ;
	(void) newbp;
        sync() ;
        target->cont ((*current_thread)->get_pid(), current_signal) ;
        current_signal = 0 ;
        return true ;
    } else {
        target->step ((*current_thread)->get_pid()) ;
        return false ;
    }
}

// main interface to continue
bool Process::cont(int sig) {
    if (!is_running()) {
       throw Exception ("The program is not being run") ;
    }
    os.print ("Continuing.\n") ;
    if (sig != 0) {
        current_signal = sig ;
    }
    return docont() ;
}

bool Process::docont() {
    invalidate_frame_cache() ;                       // frame cache is not valid until we stop

    //println ("continuing")
    if (hitbp != NULL && !hitbp->is_sw_watchpoint()) {
        (*current_thread)->go() ;                           // mark current thread as running
        if (multithreaded) {
            resume_threads() ;                              // restart all threads (except current)
        }

        ///printf ("continuing from breakpoint\n") ;
        //tempremove_breakpoints (hitbp->get_address()) ;
        step_from_breakpoint (hitbp) ;

    } else {
        //println ("hitbp = NULL")
        sync() ;
    }
  
    // in multithreaded mode, if we continue from a breakpoint, all the threads will begin running.
    // one of them might hit a breakpoint.  If it does, the hitbp variable will be the breakpoint hit.
    // in this case, we don't want to continue
    if (hitbp != NULL) {
       state = READY ;
       return false ;
    }

    // if there are software watchpoints active then we need to single step the current
    // thread.  
    if (sw_watchpoints_active()) {
#if 0
        (*current_thread)->go() ;                           // mark current thread as running
        if (multithreaded) {
            resume_threads() ;                              // restart all threads (except current)
        }
#endif
        bool wascont = step_one_instruction() ;
        state = wascont ? CSTEPPING : STEPPING ;
        wait() ;                        // will not return until program stopped
        return false ;
    } else {
        // threads will all be stopped (by wait()) here, so now we resume them
        if (multithreaded) {
            resume_threads() ;                              // restart all threads (except current)
        } else {
            target->cont ((*current_thread)->get_pid(), current_signal) ;
            current_signal = 0 ;
        }
        state = RUNNING ;
        return true ;
    }
}

void Process::single_step() {
    invalidate_frame_cache() ;                       // frame cache is not valid until we stop
    stepped_onto_breakpoint = false ;

    //printf ("single step\n") ;
#if 0
    (*current_thread)->go() ;                           // mark current thread as running
    if (multithreaded) {
        resume_threads() ;                              // restart all threads (except current)
    }
#endif

    if (hitbp != NULL && !hitbp->is_sw_watchpoint()) {                    // stepping from a breakpoint?
        tempremove_breakpoints (hitbp->get_address()) ;         // remove all breakpoints at this address
        state = STEPPING ;

        disable_threads() ;                             // disable all threads
        step_from_breakpoint (hitbp) ;                       // step one instruction and reinsert breakpoint
        enable_threads() ;

        if (stepping_lines) {
#if 0
            (*current_thread)->go() ;                           // mark current thread as running
            if (multithreaded) {
                resume_threads() ;                              // restart all threads (except current)
            }
#endif

           Location cur_loc = lookup_address(get_reg("pc"));
           if (!cur_loc.is_known() || cur_loc.equiv(last_loc)) {
               bool wascont = step_one_instruction() ;
               state = wascont ? CSTEPPING : STEPPING ;
               wait() ;
           }
        }
    } else {
        sync() ;

#if 0
        (*current_thread)->go() ;                           // mark current thread as running
        if (multithreaded) {
            resume_threads() ;                              // restart all threads (except current)
        }
#endif
        bool wascont = step_one_instruction() ;
        state = wascont ? CSTEPPING : STEPPING ;
        wait() ;
    }

}

Utils::RegularExpression stl_oper_regex("std::.*");

void Process::step(bool by_line, bool over, int n) {
    /* check that process is indeed alive and running */
    if (state == IDLE || state == EXITED || state == DISABLED) {
       throw Exception ("The program is not being run") ;
    }

    stepping_lines = by_line ;
    stepping_over = over ;
    Address before_fp = get_reg ("fp") ;
    bool in_stl = false;
    bool skip_stl = false;

    /* check if I should skip through stl junk */
    if (get_language() == DW_LANG_C_plus_plus &&
        get_int_opt(PRM_STL_STEP) == 1) {
        skip_stl = true;
    }

    /* actually perform function stepping */
    while (n > 0 || in_stl) {
        /* increment pc */
        single_step();

        /* check if died */
        if (state != READY) break;
  
        /* match against stl junk */ 
        if (skip_stl) {
           Address pc = get_reg("pc");
           Location loc = lookup_address(pc);
       
           if (stl_oper_regex.matches(loc.get_symname())) {
              in_stl = true;
              continue;
           }
        }

        /* incr for non-stl steps */
        in_stl = false; n--;
    }


    Address after_fp = get_reg ("fp") ;
    if (state == EXITED) {
        return ;
    }

    if (!stepped_onto_breakpoint && !(hitbp != NULL && hitbp->is_user())) {
        Address pc = get_reg ("pc") ;
        Location loc = lookup_address (pc) ;
        build_frame_cache() ;
        if (after_fp != before_fp && frame_cache_valid) {  // stepped into or out of a function?
           print_loc(loc, frame_cache[current_frame], os) ;
        }
        loc.show_line(os, get_cli()->isemacs()) ;
        set_current_line (loc.get_line()) ;
    }
    if (!by_line) {
        Address pc = get_reg("pc") ;
        disassemble (pc, pc, true) ;
    }
    execute_displays() ;
}

// single step until line is increased or out of frame
void Process::until() {
    if (state == IDLE || state == EXITED || state == DISABLED) {
       throw Exception ("The program is not being run") ;
    }
    Address current_sp = get_reg ("sp") ;
    Address current_pc = get_reg ("pc") ;
    stepping_lines = true ;
    stepping_over = true ;
    Address before_fp = get_reg ("fp") ;
    while (state == READY) {
        single_step() ;
        Address sp = get_reg("sp") ;
        Address pc = get_reg("pc") ;
        if (sp > current_sp || pc > current_pc) {
            break ;
        }
    }
    Address after_fp = get_reg ("fp") ;

    if (stepped_onto_breakpoint || (hitbp != NULL && hitbp->is_user())) {
    } else {
        Address pc = get_reg ("pc") ;
        Location loc = lookup_address (pc) ;
        build_frame_cache() ;
        if (after_fp != before_fp && frame_cache_valid) {                     // stepped into or out of a function?
           print_loc(loc, frame_cache[current_frame], os) ;
        }
        loc.show_line(os, get_cli()->isemacs()) ;
        set_current_line (loc.get_line()) ;
    }
    execute_displays() ;
}

// continue until address or end of frame
//  XXX: what about longjmp and exceptions?
void Process::until(Address addr) {
    Breakpoint *bp = new_breakpoint (BP_TEMP, "", addr) ;           // breakpoint at specified address
    Address ra = get_return_addr() ;
    Breakpoint *rabp = new_breakpoint (BP_TEMP, "", ra) ;           // breakpoint at return address
    if (docont()) {                                                  // continue execution
        wait() ;                                                    // wait for stop
    }
    remove_breakpoint (bp) ;
    remove_breakpoint (rabp) ;
    delete bp ;
    delete rabp ;

    Address pc = get_reg ("pc") ;
    Location loc = lookup_address (pc) ;
    build_frame_cache() ;
    if (frame_cache_valid) {
       print_loc(loc, frame_cache[current_frame], os) ;
    }
    loc.show_line(os, get_cli()->isemacs()) ;
    set_current_line (loc.get_line()) ;

    execute_displays() ;
}

// simple - set the pc to the value
bool Process::jump(Address addr) {
    Address pc = get_reg ("pc") ;
    if (pc != addr) {
        hitbp = NULL ;                      // don't want to remove breakpoint at wrong address
    }
    set_reg ("pc", addr) ;
    return cont() ;
}

Address Process::get_return_addr() {  
    build_frame_cache() ;
    Address addr = 0 ;
    if (frame_cache_valid && frame_cache.size() > 1) {
        Frame *frame = frame_cache[1] ;         // frame just above current
        addr = frame->get_reg (arch->translate_regname("pc")) ;
    } else {            // no valid frame cache, lets try to get it the hard way
        Address pc = get_reg("pc") ;
        if (arch->inframemaker (this, pc)) {
            Address sp = get_reg ("sp") ;
            if (test_address(sp)) {
                addr =  readptr (sp) ;
            }
        } else {
            Address fp = get_reg ("fp") + arch->ptrsize() ;
            if (test_address (fp)) {
                addr =  readptr (fp) ;
            }
        }
    }

#if 0
    Frame *frame = frame_cache[0] ;
    FDE *fde = frame->get_fde() ;
    Address addr ;
    if (fde != NULL) {                      // FDE may not exist, deal with it
        addr = get_fde_return_address (fde, frame->get_pc(), frame) ;
    } else {                            // if there is no FDE then we have to try to figure out where the address is
                                        // if we are in the frame builder (before sp is decremented), then the
                                        // address is the first thing on the stack.  Otherwise, get fp and the address
                                        // is the first this above that   XXX: this is architecture dependent, fix it
        Address pc = get_reg("pc") ;
        if (arch->inframemaker (this, pc)) {
            Address sp = get_reg ("sp") ;
            addr =  readptr (sp) ;
        } else {
            Address fp = get_reg ("fp") + arch->ptrsize() ;
            addr =  readptr (fp) ;
        }
    }
    if (!is_valid_code_address (addr)) {
       return 0 ;
    }
#endif
    //printf ("result is 0x%llx\n", addr) ;
    return addr ;
}

void Process::resume_stepping() {
    state = STEPPING ;
}

// interrupt from user
void Process::interrupt() {
    target->interrupt((*current_thread)->get_pid()) ;
}

// after a fork event has been received, the child has a SIGSTOP signal pending.  We need
// to wait for the child to receive this signal
void Process::wait_for_child (pid_t pid) {
    int status = 0;
    for (;;) {
        int result = waitpid (pid, &status, 0) ;
        if (result != -1 || errno != EINTR) {
            break ;
        }
    }
    if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGSTOP) {
       throw Exception ("Child didn't receive SIGSTOP") ;
    }
}


// follow a fork
// The current pid is the parent.  There are a couple of things we can do here:
// 1. follow the parent.  We have to detach any breakpoints from the child so that
//    it won't get a SIGTRAP if it hits one.  We also need to PTRACE_DETACH from it
//    and let it continue on its merry way
// 2. follow the child.  The parent must be allowed to run free so we detach breakpoints
//    from it and PTRACE_DETACH it.  Then reset the pid variable to the child pid
// 3. Follow both.  This involves creating a new Process, attached to the child (to me
//    implemented)

// XXX: vfork nastiness?
void Process::follow_fork (pid_t childpid, bool is_vfork) {
    ForkType followmode = (ForkType)get_int_opt(PRM_FOL_FORK);

    if (followmode == FORK_ASK) {
         for (;;) {
             os.print ("fork() detected, follow parent (%d), child (%d) or both? (p, c or b)? ", pid, childpid) ;
             os.flush() ;
             char ch = getchar() ;
             if (ch == 'p') {
                 followmode = FORK_PARENT;
                 break ;
             } else if (ch == 'c') {
                 followmode = FORK_CHILD;
                 break ;
             } else if (ch == 'b') {
                 followmode = FORK_BOTH ;
                 break ;
             }
             os.print ("Please choose one of p, c, or b.\n") ;
        }
    }

    if (followmode == FORK_CHILD) {
        os.print ("Attaching after fork to child process %d.\n", childpid) ;
        // follow the child.
        //
        // we need to detach all the breakpoints from the parent process and then
        // detach from it.  The pid is then the child pid
        detach_breakpoints (pid) ;                      // detach breakpoints from parent
        target->detach (pid, false) ;                   // detach from parent
        pid = childpid ;                                // point to child
        attach_breakpoints (pid) ;                      // attach breakpoints to child
    } else if (followmode == FORK_PARENT) {
        // follow the parent
        // detach from the child.  The child will continue execution.  If we the call was vfork
        // then we need to wait for the VFORKDONE event before continuing.  Before detaching
        // the child, detach all the breakpoints from it.  If the call was vfork then this
        // will also detach the breakpoints from the parent, so we need to reinsert them.
        os.print ("Detaching after fork from child process %d.\n", childpid) ;
        detach_breakpoints (childpid) ;
        target->detach (childpid, false) ;              // detach from child
        if (is_vfork) {
            // we need to wait for the VFORKDONE event before inserting the breakpoints in the parent again
            target->cont (pid, 0) ;
            int status ;
            waitpid (pid, &status, 0) ;
// FIXME: factor out
#if defined (__linux__)
            if ((status >> 16) != PTRACE_EVENT_VFORK_DONE) {             // XXX: ptrace stuff
                 std::cerr << "didn't get VFORKDONE event\n" ;
             }
#endif
            attach_breakpoints (pid) ;           // reattach the breakpoints
        }
    } else if (followmode == FORK_BOTH) {  
        os.print ("Attaching after fork to both parent (%d) and child (%d) processes.\n", pid, childpid) ;
        Process *childproc = new Process (*this) ;               // copy process
        int newid = pcm->add_process (childproc) ;                  // add it to the pcm
        os.print ("New process for child is %d.\n", newid) ;
        childproc->attach_child (childpid, state) ;            // attach it to the child
    }
}

bool Process::is_child_pid (int p) {
    if (multithreaded) {
        for (ThreadList::iterator i = threads.begin() ; i != threads.end() ; i++) {
            Thread *t = *i ;
            if (p == t->get_pid()) {
                return true ;
            }
        }
        return false ;
    } else {
        return p == pid ;
    }
}

int Process::dowait(int &status) {
    if (multithreaded) {
        for (ThreadList::iterator i = threads.begin() ; i != threads.end() ; i++) {
            Thread *t = *i ;
            //int v = waitpid (t->get_pid(), &status, WNOHANG|__WALL) ;
		int v = waitpid (t->get_pid(), &status, WNOHANG|WAITPID_ALL_CHILD_TYPES) ;
            if (v > 0) {
                //printf ("dowait returning pid %d, status %x\n", v, status) ;
                    t->stop() ;                     // mark thread as having stopped
                    t->set_stop_status (status) ;
                return v ;
            }
        }
        return 0 ;
    } else {
        return waitpid (pid, &status, WNOHANG) ;
    }
}

int Process::mt_wait() {
    int status = -1 ;
    
    while (status == -1) {
        for (ThreadList::iterator i = threads.begin() ; i != threads.end() ; i++) {
            Thread *t = *i ;
            if (!t->is_disabled()) {
                //int v = waitpid (t->get_pid(), &status, WNOHANG|__WALL) ;
		int v = waitpid (t->get_pid(), &status, WNOHANG|WAITPID_ALL_CHILD_TYPES) ;
                if (v > 0) {
                    t->stop() ;                     // mark thread as having stopped
                    t->set_stop_status (status) ;
                }
            }
        }
    }
    return status ;
}

bool Process::handle_signal(bool& stop_hook_executed) {
   int sigact = signal_manager.hit (signalnum, os) ;
   if ((sigact & SIGACT_STOP) && !stop_hook_executed) {
       stop_hook() ;
       stop_hook_executed = true ;
   }
  if (sigact & SIGACT_PRINT) {
      if (multithreaded) {
          os.print ("Program (thread %d) received signal ", (*current_thread)->get_num()) ;
      } else {
          os.print ("Program received signal ") ;
      }
      signal_manager.print (signalnum, os) ;
      os.print (".\n") ;
  }

  if (sigact & SIGACT_PASS) {
      current_signal = signalnum ;
  } else {
      current_signal = 0 ;
  }

  if (!(sigact & SIGACT_STOP)) {
      if (state == STEPPING || state == ISTEPPING) {
          single_step() ;
      } else {
          docont() ;
      }
      return true;
  } else {
      build_frame_cache() ;
      if (state != STEPPING || state == ISTEPPING) {
         Address pc = get_reg ("pc") ;
         Location loc = lookup_address (pc) ;
         print_loc(loc, frame_cache[current_frame], os) ;
         loc.show_line(os, get_cli()->isemacs()) ;
         set_current_line (loc.get_line()) ;
      }
      state = READY ;
      return false;
  }
}

// return true if caller is to continue waiting
bool Process::wait(int status) {
    bool in_dynamic_linker = false ;
    int loopcount = 0 ;
    bool stop_hook_executed = false ;
    proper_state = 0;

    while (state == RUNNING || state == STEPPING || state ==  ISTEPPING || state == CSTEPPING) {
        //printf ("waiting, status = %d, loop: %d, state = %d\n",  status, loopcount, state) ;
        if (loopcount++ > 0 || status == -1) {
            if (multithreaded) {
                status = mt_wait() ;
            } else {
                waitpid (pid, &status, 0) ;
            }
        }

        if (WIFEXITED (status)) {
            if (WEXITSTATUS (status) == 0) {
                os.print ("\nProgram exited normally.\n") ;
            } else {
                os.print ("\nProgram exited with code 0%o\n", WEXITSTATUS (status)) ;   // GDB prints this in octal (yuk)!
            }
            state = EXITED ;
        } else if  (WIFSIGNALED (status)) {
            signalnum = WTERMSIG (status);
            state = EXITED;
            os.print ("\nProgram terminated with signal ");
            signal_manager.print (signalnum, os) ;
            os.print (".\n") ; 
            os.print ("The program no longer exists.\n");
        } else if  (WIFSTOPPED (status)) {
            signalnum = WSTOPSIG (status) ;

            //printf ("wait returned with signal %d\n", signalnum) ;
            if (!multithreaded) {
                (*current_thread)->syncin() ;
                (*current_thread)->stop() ;                 // mark current thread as stopped
            } else {
                grope_threads() ;                       // find stopped threads
                stop_threads() ;                        // stop all that aren't stopped
            }

            //println ("returned from waitpid at address 0x" + format ("%x", get_reg("pc")))

            // stopped by attaching
            if (signalnum == SIGSTOP && attach_type == ATTACH_PROCESS) {
                state = READY ;
                continue ;
            }

            if (signalnum == SIGTRAP) {  // SIGTRAP
                if (status >> 16 != 0) {                         // extended wait status
					// FIXME: factor out
#if defined (__linux__)
                    int event = status >> 16 ;
                    //printf ("extended wait event: %d\n", event) ;
                    switch (event) {
                    case PTRACE_EVENT_VFORK:
                    case PTRACE_EVENT_FORK: {
                        pid_t childpid = target->get_fork_pid(pid) ;
                        wait_for_child (childpid) ;             // wait for child to stop
                        follow_fork (childpid, event == PTRACE_EVENT_VFORK) ;
                        if (state == STEPPING) {
                            state = READY ;
                            continue ;
                        }
                        return docont() ;                      // continue execution
                        }
                    case PTRACE_EVENT_EXEC:             // XXX: do this
                        break ;
                    }
#endif
                }

                bool switched_threads = false ;

                if (multithreaded) {
                    std::vector<ThreadList::iterator> hit_threads ;             // threads that have a hit pending
                    std::vector<ThreadList::iterator> userbp_threads ;             // threads that have a hit pending
                    find_bp_threads (hit_threads, userbp_threads) ;                        // all threads that are on a breakpoint
                    ThreadList::iterator new_current = current_thread ;
                    if (hit_threads.size() == 0) {                      // no threads on breakpoints
                        //printf ("no threads on a breakpoint\n") ;
                    } else if (hit_threads.size() == 1) {               // one thread on a breakpoint
                        new_current = hit_threads[0] ;
                        //printf ("thread %d on a breakpoint\n", (*new_current)->get_num()) ;
                    } else {                                            // more than one on a breakpoint
                        //printf ("hit: %d, user: %d\n", (int)hit_threads.size(), (int)userbp_threads.size()) ;
                        // look to see if any of them are on a user breakpoint
                        if (userbp_threads.size() > 0) {
                            if (userbp_threads.size() == 1) {           // only one on a user breakpoint
                                new_current = userbp_threads[0] ;             
                                //printf ("chose unique user thread %d\n", (*new_current)->get_num()) ;
                            } else {
                                //printf ("more than 1 thread on user bp\n") ;
                                int neo = rand() % userbp_threads.size() ;              // choose among them
                                new_current = userbp_threads[neo] ;
                                //printf ("chose user thread %d\n", (*new_current)->get_num()) ;
                            }
                        } else {
                            //printf ("more than 1 thread on bp\n") ;
                            // choose one at random (like gdb)
                            uint neo = rand() % hit_threads.size() ;
                            //printf ("chose thread %d\n", (*hit_threads[neo])->get_num()) ;
                            new_current = hit_threads[neo] ;             // choose it
                        }

                        // rewind the other threads
                        for (uint i = 0 ; i < hit_threads.size() ; i++) {
                            if (*hit_threads[i] != *new_current) {
                                Thread *t = *hit_threads[i] ;
                                Address pc = t->get_reg("pc") ;
                                pc -= arch->bpsize() ;
                                t->set_reg ("pc", pc) ;
                                //printf ("rewinding thread %d\n", (*hit_threads[i])->get_num()) ;
                            }
                        }
                    }
                    if (current_thread != new_current) {
                       os.print ("[Switching to ") ;
                       (*new_current)->print (os) ;
                       os.print ("]\n") ;
                       switched_threads = true ;
                    }
                    current_thread = new_current ;
                } else {
                    (*current_thread)->syncin() ;
                    (*current_thread)->stop() ;
                }

                // this may not be a breakpoint, so we only move the pc if it stopped at a bp
                // move the pc back to the breakpoint address
                Address pc = get_reg ("pc") ;
                Address backpc = pc - arch->bpsize() ;
                //printf ("PC at wait: 0x%llx\n", (unsigned long long)pc) ;

                // find the list of breakpoints we have stopped at
                Address lookuppc = (switched_threads || state == RUNNING || state == CSTEPPING) ? backpc : pc ;

                bool sw_active = sw_watchpoints_active() ;              // any active software watchpoints?
                BreakpointList *bplist = find_breakpoint (lookuppc) ;             // find all breakpoints at this address
                //if (bplist == NULL) {
                    //printf ("no breakpoint found for address 0x%llx\n", (unsigned long long)lookuppc) ;
                //}
                Breakpoint_action action = BP_ACTION_IGNORE ;
                hitbp = NULL ;
                Breakpoint *activebp = NULL ;
                bool pc_moved_back = false ;                // true is pc bas been moved back
                bool sw_wp_hit = false ;                    // true if a software watchpoint has been hit

                if (sw_active || bplist != NULL) {
                    BreakpointList templist ;
                    // if there are breakpoints active, copy them
                    if (bplist != NULL) {
                        for (BreakpointList::iterator bpi = bplist->begin() ; bpi != bplist->end() ; bpi++) {
                            Breakpoint *bp = *bpi ;
                            templist.push_back (bp) ;
                        }
                    }
                    // if there are software watchpoints active, add them to the list
                    if (sw_active) {
                        for (BreakpointList::iterator bpi = sw_watchpoints.begin() ; bpi != sw_watchpoints.end() ; bpi++) {
                            templist.push_back (*bpi) ;
                        }
                    }
                    for (BreakpointList::iterator bpi = templist.begin() ; bpi != templist.end() ; bpi++) {
                        Breakpoint *bp = *bpi ;
                        if (!bp->is_disabled() && !bp->is_removed() && bp->is_applied()) {
                            if (!bp->is_sw_watchpoint()) {
                                hitbp = bp ;                        // record for remove_breakpoint
                            } else {
                                sw_wp_hit = true ;
                            }
                            // need to move the pc back before calling bp->hit() as it might build the frame cache
                            if (bp->is_applied() && bp->requires_backup() && !pc_moved_back && (state == RUNNING || state == CSTEPPING)) {
                                pc_moved_back = true ;
                                set_reg ("pc", backpc) ;
                                pc = backpc ;
                            } else {
                                if (state == STEPPING || state == CSTEPPING) {
                                    stepped_onto_breakpoint = true ;            // suppress print in single_step
                                }
                                //hitbp = NULL ;          // no backup required
                            }
                            Breakpoint_action act = bp->hit(os) ;
                            if (act == BP_ACTION_STOP) {
                                stop_hook_executed = true ;             // the breakpoint will have executed it
                                action = act ;
                            } else if (act == BP_ACTION_CONT && action == BP_ACTION_IGNORE) {
                                action = act ;
                            }
                            // if the breakpoint survives the hit, then record it
                            if (hitbp != NULL) {
                                activebp = hitbp ;
                            }
                        }
                    }
                }
                // activebp will be non-null if a breakpoint is still inserted at the
                // pc.  Record this in the 'hitbp' variable for processing later
                hitbp = activebp ;

                if (hitbp != NULL) {
                    switch (action) {
                    case BP_ACTION_STOP:
                        state = READY ;
                        break ;
                    case BP_ACTION_CONT:
                        //println ("BP_ACTION_CONT:")
                        if (state == STEPPING) {
                            if (hitbp == NULL) {
                                step_one_instruction() ;
                            } else {
                                step_from_breakpoint (hitbp) ;               // XXX: check this
                            }
                            continue ;
                        } else if (state == CSTEPPING) {
                            if (docont()) {
                                state = CSTEPPING ;
                                continue ;
                            }
                        } else {
                            docont() ;                   // continue execution and return to wait
                            continue ;
                        }
                        break ;
                    case BP_ACTION_IGNORE:
                        break ;
                    default:
                        break ;
                    }
                } else if (action == BP_ACTION_STOP) {          // a temp bp might have asked us to stop
                    state = READY ;
                    continue ;
                } else if (sw_wp_hit) {                 // a software watchpoint was hit but said continue
                    step_one_instruction() ;
                    continue ;
                }


                // we got a SIGTRAP and we're not on a breakpoint, this is eiher
                // the initial ptrace stop or the result of a single step
                if (state == STEPPING || state == CSTEPPING) {
                    bool call = arch->is_call (this, pc) ;
                    // first thing we do is to check if we are at a known line.  If it is, then
                    // we stop here
                    if (stepping_lines) {
                       if (stepping_stops(pc)) {
                          state = READY ;
                          continue ;
                       }
                    } // if we are stepping by instructions then we stop here
                    else {
                        state = READY ;
                        continue ;
                    }

                    bool in_plt = false ;// in the PLT

                    // we don't want to step into the dynamic linker, so check if we are going into it
                    // and if we are, stop in the 'fixup' routine and continue to there
                    if (stepping_lines && !in_dynamic_linker && pc >= dyn_start && pc <= dyn_end) {
                        //println ("inside PLT, continuing")
                        new_breakpoint (BP_DYNLINK, "", fixup_addr) ;
                        docont() ;                          // continue execution until breakpoint at fixup
                        in_dynamic_linker = true ;
                        continue ;
                    } else if  (pc < dyn_start || pc > dyn_end) {
                        in_dynamic_linker = false ;
                    }

                    // if the instruction is a call and we are stepping over, set a breakpoint
                    // at the next instruction and continue to it
                    if (!in_dynamic_linker && call && stepping_over) {
                        Address nextpc = pc + arch->call_size(this, pc) ;
                        //printf ("stepping over call to address 0x%llx\n", nextpc) ;
                        Breakpoint *bp = new_breakpoint (BP_STEP, "", nextpc) ;
			(void) bp;
                        if (docont()) {
                            state = CSTEPPING ;                     // continue until breakpoint and enter step mode
                        }
                        continue ;
                    }

                    Section * section = find_section_at_addr (pc) ;
                    if (stepping_lines && section != NULL) {
                        //printf ("address: 0x%llx, section name is %s\n", pc, section->get_name().c_str()) ;
                        if (section->get_name() == ".plt") {             // calling into the plt?
                            in_plt = true ;
                        }
                    }

                    // the address will either be in the PLT, dynamic linker or a regular function
                    // if it's in a regular function and we don't have debug information.  Single
                    // step until we get somewhere that does have debug information.
                    if (stepping_lines && !in_dynamic_linker && !in_plt) {
                        Location loc = lookup_address (pc) ;
                        if ( !loc.has_debug_info() ) {
                            if ( ! (proper_state & PSTATE_QUIET_STEP) ) {
                               printf ("Single stepping to the end of function %s because it has "
                                       "no debug information.\n", loc.get_symname().c_str()) ;
                            }

                            bool wascont = step_one_instruction();
                            proper_state = PSTATE_QUIET_STEP;
                            state = wascont ? CSTEPPING : STEPPING ;
                            continue;
                        }
                    }

                    if (stepping_lines) {
                        step_one_instruction() ;    // if we get here then we are not on a line
                    } else {
                        state = READY ;
                    }
                } else {
                   if (action == BP_ACTION_STOP || action == BP_ACTION_IGNORE) {
                       state = READY ;
                   } else {
                       docont() ;
                       continue ;
                   }
                }
            } else {
	      if (handle_signal(stop_hook_executed))
                continue;
            }
        } else {
            if (!stop_hook_executed) {
                stop_hook() ;
                stop_hook_executed = true ;
            }
            std::cerr << "Process has stopped for an unknown reason: 0x" << std::hex << status << std::dec << "\n" ;
            state = READY ;
        }
    }
    if (!init_phase && !stop_hook_executed) {
        stop_hook() ;
    }
    return false ;
}

void Process::get_regs(RegisterSet *regs, void *tid)
{
	if (thread_agent != NULL && tid != NULL)
	{
		thread_db::read_thread_registers(thread_agent, tid, regs, arch) ;
	}
	else
	{
		target->get_regs((*current_thread)->get_pid(), regs) ;
	}
}

void Process::set_regs(RegisterSet *regs, void *tid)
{
	if (thread_agent != NULL && tid != NULL) {
		thread_db::write_thread_registers(thread_agent, tid, regs, arch) ;
	}
	else
	{
		target->set_regs((*current_thread)->get_pid(), regs) ;
	}
}

void Process::get_fpregs(RegisterSet *regs, void *tid) {
    arch->get_fpregs((void*)thread_agent, tid, (*current_thread)->get_pid(), target, regs) ;
}

void Process::set_fpregs(RegisterSet *regs, void *tid) {
    arch->set_fpregs((void*)thread_agent, tid, (*current_thread)->get_pid(), target, regs) ;
}

Breakpoint * Process::new_breakpoint(BreakpointType type, std::string text, Address addr, bool pending) {
    Breakpoint * bp = NULL ;
    switch (type) {
    case BP_USER:
        bp = new UserBreakpoint (arch, this, text, addr, bpnum) ;
        bpnum++ ;
        break ;
    case BP_TEMP:
        bp = new TempBreakpoint (arch, this, addr, -ibpnum) ;
        ibpnum++ ;
        break ;
    case BP_DSO:
        bp = new SharedLibraryBreakpoint (arch, this, addr, -ibpnum) ;
        ibpnum++ ;
        break ;
    case BP_DYNLINK:
        bp = new DynamicLinkBreakpoint (arch, this, addr, -ibpnum) ;
        ibpnum++ ;
        break ;
    case BP_STEP:
        bp = new StepBreakpoint (arch, this, addr, -ibpnum) ;
        ibpnum++ ;
        break ;
    case BP_THR_CREATE:
        bp = new ThreadBreakpoint (arch, this, addr, -ibpnum, true) ;
        ibpnum++ ;
        break ;
    case BP_THR_DEATH:
        bp = new ThreadBreakpoint (arch, this, addr, -ibpnum, false) ;
        ibpnum++ ;
        break ;
    case BP_HBREAK:                     // hardware breakpoint
        bp = new HardwareBreakpoint (arch, this, text, addr, bpnum) ;
        bpnum++ ;
        break ;
    case BP_CASCADE:
        bp = new CascadeBreakpoint (arch, this, addr, -ibpnum) ;
        ibpnum++ ;
        break ;
    default:
	break; // added by bos for -Wall niceness
    }
    add_breakpoint (bp) ;
    bp->set_pending (pending) ;
    if (bp->is_user()) {
        if (pending) {
            os.print ("Breakpoint %d (%s) pending.\n", bp->get_num(), bp->get_text().c_str()) ;
        } else {
            os.print ("Breakpoint %d at 0x%llx", bp->get_num(), addr) ;
            bp->print_short_location(os) ;
            os.print ("\n") ;
        }
    }
    return bp ;
}

Watchpoint * Process::new_watchpoint(BreakpointType type, std::string expr, Node *node, Address addr, int size, bool pending) {
    Watchpoint * bp = NULL ;
    switch (type) {
    case BP_CHWATCH:
        bp = new ChangeWatchpoint (arch, this, expr, node, addr, size, bpnum) ;
        bpnum++ ;
        break ;
    case BP_RWWATCH:
        bp = new ReadWriteWatchpoint (arch, this, expr, node, addr, size, bpnum) ;
        bpnum++ ;
        break ;
    case BP_RWATCH:
        bp = new ReadWatchpoint (arch, this, expr, node, addr, size, bpnum) ;
        bpnum++ ;
        break ;
    case BP_WWATCH:
        bp = new WriteWatchpoint (arch, this, expr, node, addr, size, bpnum) ;
        bpnum++ ;
        break ;
    case BP_SWWATCH:
        bp = new SoftwareWatchpoint (arch, this, expr, node, addr, size, bpnum) ;
        bpnum++ ;
        break ;
    default:
	break; // added by bos for -Wall niceness
    }
    bp->set_pending (pending) ;
    if (pending) {
        os.print ("Watchpoint %d (%s) pending.\n", bp->get_num(), bp->get_text().c_str()) ;
    } else {
        bp->show_header (os) ;
        bp->print_short_location(os) ;
        os.print ("\n") ;
    }

    add_breakpoint (bp) ;
    return bp ;
}

Catchpoint *Process::new_catchpoint (CatchpointType type, std::string data) {
    Catchpoint *bp = new Catchpoint (arch, this, ++bpnum, type, data) ;
    os.print ("Catchpoint %d (%s)\n", bp->get_num(), bp->get_type() + 6) ;
    add_breakpoint (bp) ;
    return bp ;
}

// because we print the values of the function arguments in the stack trace, we need
// to unwind the stack fully on a trace

void Process::stacktrace(int n) {
    build_frame_cache() ;
    bool gdbmode = get_cli()->get_flags() & CLI_FLAG_GDB ;

    unsigned first, last;

    if (n > 0) {
       first = current_frame;
       unsigned x_last = current_frame+n;
       if (x_last > frame_cache.size()) {
          last = frame_cache.size();
       } else {
          last = x_last;
       }
    } else {
       first = 0;
       last = frame_cache.size();
    }

    for (unsigned i = first ; i < last; i++) {
        frame_cache[i]->print(os, !gdbmode, i == (uint) current_frame) ;
    }

    if (frame_corrupted) {
        os.print("     <stack corrupted>\n");
    }
}

bool Process::stack_contains(Address fp, Address start_pc, Address end_pc) {
    build_frame_cache();

    for (unsigned i = 0; i < frame_cache.size(); i++) {
        Frame* frame = frame_cache[i];
        Address f_fp = frame->get_fp();
        Address f_pc = frame->get_pc();

        if (f_fp == fp && f_pc >= start_pc && f_pc <= end_pc) {
            return true;
        }
    }
    return false;
}

void Process::print_regs(bool all) {
    if (state == IDLE || state == EXITED) {
       throw Exception ("The program is not being run") ;
    }
    (*current_thread)->print_regs(os, all) ;
}

void Process::print_reg(std::string name) {
    if (state == IDLE || state == EXITED) {
       throw Exception ("The program is not being run") ;
    }
    (*current_thread)->print_reg(name, os) ;
}

// follow the GDB behaviour.  Lookup the address to find the function it is in
// and disassemble the function
// XXX: allow address range
void Process::disassemble (Address addr, bool newline) {
    if (threads.size() == 0) {
        throw Exception ("An active process is required for this operation") ;
    }
    if (addr == 0) {
        addr = get_reg ("pc") ;
    }
    int len = 100 ;
    Location loc = lookup_address (addr) ;
    if (loc.has_debug_info()) {
        len = loc.get_funcloc()->endaddr - loc.get_funcloc()->startaddr + 1 ;
        addr = loc.get_funcloc()->startaddr ;
    }
    while (len > 0) {
        int n =  arch->disassemble (os, this, addr) ;
        if (newline) {
            os.print ("\n") ;
        }
        addr += n ;
        len -= n ;
    }
}

void Process::disassemble (Address start, Address end, bool newline) {
    int len = end - start + 1 ;
    Address addr = start ;

    Location loc = lookup_address (addr) ;
    while (len > 0) {
        int n =  arch->disassemble (os, this, addr) ;
        if (newline) {
            os.print ("\n") ;
        }
        addr += n ;
        len -= n ;
    }
}

void Process::disassemble (Address start, int ninsts, bool newline) {
    Address addr = start ;

    Location loc = lookup_address (addr) ;
    while (ninsts > 0) {
        int n =  arch->disassemble (os, this, addr) ;
        if (newline) {
            os.print ("\n") ;
        }
        addr += n ;
        ninsts-- ;
    }
}

// display command

Display::Display (Process *proc, int n, PStream &os, std::string expr, int start, Format &fmt)
   :n(n), os(os), expr(expr), fmt(fmt), disabled(false), local(false) {

    node = proc->compile_expression (expr.substr(start), start) ;
    if (node == NULL) {
        throw Exception ("Can't add display") ;
    }
    if (node->is_local()) {
        loc = proc->get_current_location() ;
        local = true ;
    }
}

Display::~Display() {
    delete node ;
}

// execute the display command by printing the expression
void Display::execute (Process *proc) {
    if (disabled || !proc->is_running()) {
        return ;
    }
    // if the expression is local, check the current location against that
    // recorded for the valid scope
    if (local) {
        Location l = proc->get_current_location() ;
        if (l.get_funcloc() != NULL && loc.get_funcloc() != NULL) {
            if (l.get_funcloc()->symbol->die != loc.get_funcloc()->symbol->die) {
                return ;
            }
        }
    }
    // according to GDB, the 'i' or 's' format codes use the 'x' command for output
    if (fmt.code == 'i' || fmt.code == 's') {
        Address addr = proc->evaluate_expression (node) ;
        proc->get_os().print ("%d: x%s  ", n, expr.c_str()); 
        proc->examine (fmt, addr) ;
    } else {
        try {
            proc->display_expression (n, expr, node, fmt) ;
        } catch (Exception e) {
            e.report (std::cout) ;
        } catch (const char *s) {
            os.print ("%s\n", s) ;
        } catch (std::string s) {
            os.print ("%s\n", s.c_str()) ;
        }
    }
}

int Process::set_display (std::string expr, int start, Format &fmt) {
    int n = ++displaynum ;
    Display *d = new Display (this, n, os, expr, start, fmt) ;
    displays.push_back (d) ;
    d->execute (this) ;
    return n ;
}

void Process::undisplay (int n) {
    if (n == -1) {                      // all?
        displays.clear() ;
    } else {
        for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
            if ((*i)->n == n) {
                delete *i ;
                displays.erase (i) ;
                return ;
            }
        }
        std::cout << "No display number " << n << ".\n" ;
    }
}

void Process::enable_display (int n) {
    if (n == -1) {                      // all?
        for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
            (*i)->enable() ;
        }
    } else {
        for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
            if ((*i)->n == n) {
                (*i)->enable() ;
                return ;
            }
        }
        std::cout << "No display number " << n << ".\n" ;
    }
}

void Process::disable_display (int n) {
    if (n == -1) {                      // all?
        for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
            (*i)->disable() ;
        }
    } else {
        for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
            if ((*i)->n == n) {
                (*i)->disable() ;
                return ;
            }
        }
        std::cout << "No display number " << n << ".\n" ;
    }
}

void Process::list_displays() {
    for (DisplayList::iterator i = displays.begin() ; i != displays.end() ; i++) {
        (*i)->execute (this) ;
    }
}

void Process::execute_displays() {
    for (DisplayList::reverse_iterator i = displays.rbegin() ; i != displays.rend() ; i++) {
        (*i)->execute (this) ;
    }
}


// breakpoint conditions etc

void Process::set_breakpoint_condition (int bpnum, std::string cond) {
    Breakpoint *bp = find_breakpoint (bpnum) ;
    if (bp == NULL) {
        os.print ("No breakpoint number %d.\n", bpnum) ;
    } else {
        bp->set_condition (cond) ;
        if (cond == "") {
            os.print ("Breakpoint %d now unconditional.\n", bpnum) ;
        }
    }
}

void Process::set_breakpoint_ignore_count (int bpnum, int n) {
    Breakpoint *bp = find_breakpoint (bpnum) ;
    if (bp == NULL) {
        os.print ("No breakpoint number %d.\n", bpnum) ;
    } else {
        bp->set_ignore_count (n) ;
        if (n <= 0) {
            os.print ("Will stop next time breakpoint %d is reached.\n", bpnum) ;
        } else {
            os.print ("Will ignore next %d crossings of breakpoint %d.\n", n, bpnum) ;
        }
    }
}

void Process::set_breakpoint_commands (int bpnum, std::vector<ComplexCommand *>& cmds) {
    Breakpoint *bp = find_breakpoint (bpnum) ;
    if (bp == NULL) {
        os.print ("No breakpoint number %d.\n", bpnum) ;
    } else {
        bp->set_commands (cmds) ;
    }
}

CommandInterpreter *Process::get_cli() {
    return pcm->get_cli() ;
}

DebuggerVar *Process::find_debugger_variable (std::string n) {
    return pcm->get_cli()->find_debugger_variable (n) ;
}

void Process::add_debugger_variable (int n, Value &val, DIE *type) {
    char buf[32] ;
    snprintf (buf, sizeof(buf), "$%d", n) ;
    pcm->get_cli()->add_debugger_variable (buf, val, type) ;
}

DebuggerVar *Process::add_debugger_variable (std::string n, Value &val, DIE *type) {
    return pcm->get_cli()->add_debugger_variable (n,  val, type) ;
}

void Process::list (File *file, int sline, int eline, int currentline) {
    file->open (pcm->get_dirlist()) ;           // in case we changed the directory search path
    file->show_line (sline, eline, os, !(get_cli()->get_flags() & CLI_FLAG_GDB), currentline) ;
    last_listed_line = eline ;
}

void Process::list() {
    build_frame_cache() ;
    File *file = NULL ;
    int currentline = 0 ;
    if (!frame_cache_valid) {
        file = current_location.get_file() ;
        currentline = current_location.get_line() ;
    } else {
        file = frame_cache[current_frame]->get_loc().get_file() ;
        currentline = frame_cache[current_frame]->get_loc().get_line() ;
        current_location = frame_cache[current_frame]->get_loc() ;                  // GDB uses list to set the current location
    }
    if (file == NULL) {
        os.print ("No source file.\n") ;
        return ;
    }
    int line = last_listed_line ;
    if (line == 0) {            // no listed line
        if (!frame_cache_valid) {
            line = current_location.get_line() ;
        } else {
            line = frame_cache[current_frame]->get_loc().get_line() ;
        }
    }
    list (file, line, line+get_int_opt(PRM_LIST_LN), currentline) ;
}

void Process::list_back() {
    build_frame_cache() ;
    File *file = NULL ;
    int currentline = 0 ;
    if (!frame_cache_valid) {
        file = current_location.get_file();
        currentline = current_location.get_line();
    } else {
        file = frame_cache[current_frame]->get_loc().get_file();
        currentline = frame_cache[current_frame]->get_loc().get_line();
        // GDB uses list to set the current location
        current_location = frame_cache[current_frame]->get_loc() ;
    }
    if (file == NULL) {
        os.print ("No source file.\n") ;
        return ;
    }
    int line = last_listed_line;
    if (last_listed_line == 0) {            // no listed line
        if (!frame_cache_valid) {
            line = current_location.get_line();
        } else {
            line = frame_cache[current_frame]->get_loc().get_line();
        }
    } else {
        line -= 2 * get_int_opt(PRM_LIST_LN);
        if (line < 1) {
            line = 1 ;
        }
    }
    list (file, line, line + get_int_opt(PRM_LIST_LN), currentline) ;
}

void Process::list(std::string filename, int line) {
    int listsize = get_int_opt(PRM_LIST_LN);
    line = line - listsize / 2 ;
    if (line < 1) {
        line = 1 ;
    }
    list (filename, line, line+listsize) ;
}

void Process::list(std::string filename, int sline, int eline) {
    File *file = NULL ;
    int currentline = 0 ;
    build_frame_cache() ;
    if (filename == "") {
        if (frame_cache_valid) {
            file = frame_cache[current_frame]->get_loc().get_file();
            currentline = frame_cache[current_frame]->get_loc().get_line();
            // GDB uses list to set the current location
            current_location = frame_cache[current_frame]->get_loc() ;
        } else {
            file = current_location.get_file();
            currentline = current_location.get_line();
        }
        if (file == NULL) {
            os.print ("No source file.\n") ;
            return ;
        }
    } else {
        for (uint i = 0 ; i < objectfiles.size() ; i++) {
            if ((file = objectfiles[i]->symtab->find_file (filename)) != NULL) {
                break ;
            }
        }
        if (file == NULL) {     
            os.print ("Can't find a file named %s\n", filename.c_str()) ;
            return ;
        }
        if (!frame_cache_valid) {
            if (file == current_location.get_file()) {
                currentline = current_location.get_line();
            }
        } else {
            if (file == frame_cache[current_frame]->get_loc().get_file()) {
                currentline = frame_cache[current_frame]->get_loc().get_line() ;
            }
        }
    }
    list (file, sline, eline, currentline) ;
}

void Process::list(Address addr, Address endaddr) {
    Location start = lookup_address (addr) ;
    if (start.get_file() == NULL) {
        os.print ("No source file at address 0x%llx.\n", addr) ;
        return ;
    }
    int currentline = 0 ;
    build_frame_cache() ;
    if (!frame_cache_valid) {
        currentline = current_location.get_line();
    } else {
        currentline = frame_cache[current_frame]->get_loc().get_line();
    }

    current_location = start ;                  // GDB uses list to set the current location

    int delta = 5 ;
    if (endaddr == 0) {
        if (delta > start.get_line()) {
            delta = 0 ;
        }
        list (start.get_file(), start.get_line()-delta,
              start.get_line()+get_int_opt(PRM_LIST_LN)
              -delta, currentline) ;
    } else {
        Location end = lookup_address (endaddr) ;
        if (end.get_file() == NULL) {
            os.print ("No source file at address 0x%llx.\n", endaddr) ;
            return ;
        }
        if (start.get_file() != end.get_file()) {
            os.print ("Can't list lines from different files.\n") ;
            return ;
        }
        if (start.get_line() > end.get_line()) {
            int tmp = start.get_line() ;
            start.set_line(end.get_line());
            end.set_line(tmp);
        }
        list (start.get_file(), start.get_line(),
              end.get_line(), currentline) ;
    }
}

void Process::search (std::string text) {
    build_frame_cache() ;
    File *file = NULL ;
    if (!frame_cache_valid) {
        file = current_location.get_file();
    } else {
        file = frame_cache[current_frame]->get_loc().get_file();
    }
    if (file == NULL) {
        os.print ("No source file.\n") ;
        return ;
    }
    int line = last_listed_line ;
    if (line == 0) {            // no listed line
        if (!frame_cache_valid) {
            line = current_location.get_line() ;
        } else {
            line = frame_cache[current_frame]->get_loc().get_line() ;
        }
    }
    line++ ;                    // next line
    std::string linetext ;
    if (text == "") {
        text = last_search_string ;
    }
    if (text == "") {
        throw Exception ("No previous regular expression.") ;
    }
    last_search_string = text ;
    file->open (pcm->get_dirlist()) ;           // in case we changed the directory search path
    if (file->search (text, line, linetext)) {
        os.print ("%d  %s\n", line, linetext.c_str()) ;
        DebuggerVar *var = find_debugger_variable ("$_") ;
        if (var == NULL) {
            Value lineval(line) ;
            DIE *type = objectfiles[0]->symtab->new_int() ;
            objectfiles[0]->symtab->keep_temp_die (type) ;
            add_debugger_variable ("$_", lineval, type) ;
        } else {
            var->value = line ;
        }
        last_listed_line = line ;
    } else {
        throw Exception ("Expression not found.") ;
    }
    
}

void Process::return_from_func(Address value) {              // return from function with value
    build_frame_cache() ;
    if (frame_cache.size() > 1) {
        Frame *frame = frame_cache[current_frame] ;
        Location loc = frame->get_loc() ;
        char buf[1024] ;
        if (loc.get_symname() != "") {
            snprintf (buf, sizeof(buf), "Make %s return now", loc.get_symname().c_str()) ;
        } else {
            snprintf (buf, sizeof(buf), "Make selected function return now") ;
        }
        bool confirmed = pcm->get_cli()->confirm (NULL, buf) ;
        if (!confirmed) {
            os.print ("Not confirmed.\n") ;
            return ;
        }
    
        current_frame++ ;
        frame = frame_cache[current_frame] ;
        loc = frame->get_loc() ;
        Thread *thr = (*current_thread) ;
        frame->publish_regs (thr, true) ;          // force register values
        thr->set_reg (arch->get_return_reg(), value) ;              // assign return value
        frame->set_n (0) ;              // just to match GDB output
        frame->print(os, false, false) ;
        frame->get_loc().show_line (os, get_cli()->isemacs()) ;
        set_current_line (frame->get_loc().get_line()) ;
    }
    invalidate_frame_cache() ;
}
                                                                                                                                  
void Process::finish() {                                     // finish execution of current function
    build_frame_cache() ;
    if (frame_cache.size() < 2) {
        throw Exception ("\"finish\" not meaningful in the outermost frame.") ;
    }

    os.print ("Run till exit from ") ;
    Frame *frame = frame_cache[current_frame] ;
    frame->print (os, false, false) ;

    // get the current position DIE
    int language ;
    DIE *die ;
    get_current_subprogram (die, language) ;

    Frame *nextframe = frame_cache[current_frame+1] ;

    // set a breakpoint at the return address and continue to it
    Address ra = nextframe->get_pc() ;                 // return address
    new_breakpoint (BP_TEMP, "", ra) ;                      // new temporary breakpoint
    cont() ;                                            // continue execution
    wait() ;                                            // wait for it to stop

    // now show the value returned and record it (if not void)
    DIE* type;
    if (die != NULL && (type = die->get_type())) {                       // not void type
        Value v ;
        if (type->is_real()) {
            std::string retreg = arch->get_return_fpreg() ;           // return value
            v = get_reg (retreg) ;
            v.type = VALUE_REAL ;
        } else {
            std::string retreg = arch->get_return_reg() ;           // return value
            v = get_reg (retreg) ;
        }
        EvalContext context (this, 0, language, os) ;
        int n = pcm->get_cli()->new_debugger_var_num() ;
        add_debugger_variable (n, v, type) ;
        os.print ("Value returned is $%d = ", n) ;
        type->print_value (context, v) ;
        os.print ("\n") ;
    }

    // we will have stopped at the return address now
    if (hitbp != NULL && hitbp->is_user()) {
                // might have stopped due to user breakpoint
    } else {
        build_frame_cache() ;
        Address pc = get_reg ("pc") ;
        Location loc = lookup_address (pc) ;
        print_loc(loc, frame_cache[current_frame], os) ;
        loc.show_line(os, get_cli()->isemacs()) ;
        set_current_line (loc.get_line()) ;
    }

}

void Process::examine (const Format &fmt, Address addr) {
    char linebuf[16] ;
    int len = fmt.count ;
    int itemsize = 1 ;
    int linesize = 16 ;         // 16 bytes per line
    if (fmt.code == 'i') {              // disassembly is different from the rest
        disassemble (addr, len, true) ;
    } else {
        switch (fmt.size) {
        case 'b':
            itemsize = 1 ;
            linesize = 8 ;          // 8 bytes per line
            break ;
        case 'h':
            itemsize = 2 ;
            len *= 2 ;
            break ;
        case 'w':
        case 'n':
            itemsize = 4 ;
            len *= 4 ;
            break ;
        case 'g':
            itemsize = 8 ;
            len *= 8 ;
            break ;
        }
        EvalContext ctx (this, addr, DW_LANG_C, os) ;
        ctx.fmt = fmt ;

        while (len > 0) {
            // print the address
            os.print ("0x%llx: ", addr) ;

            // fill one line of data
            for (int i = 0 ; i < linesize ; i += 4) {
                 int tmp = read (addr, 4) ; // 4 bytes, little endian
                 linebuf[i] = tmp & 0xff ;
                 linebuf[i+1] = (tmp >> 8) & 0xff ;
                 linebuf[i+2] = (tmp >> 16) & 0xff ;
                 linebuf[i+3] = (tmp >> 24) & 0xff ;
                 addr += 4 ;
            }
            // now print the result
            for (int i = 0 ; i < linesize && len > 0; i += itemsize, len -= itemsize) {
                Address tmp = 0 ;
                memcpy (&tmp, &linebuf[i], itemsize) ;
                os.print ("  ") ;

                /* XXX: replace me */
                JunkStream* js = reinterpret_cast<JunkStream*>(&os);
                js->print (ctx, tmp) ;
            }
            os.print ("\n") ;
        }
    }
}

void Process::get_current_subprogram(DIE *&die, int &language) {
    build_frame_cache() ;
    SymbolTable *symtab = NULL ;
    bool autolang ;
    language = get_language(autolang) ;
    die = NULL ;
    if (!frame_cache_valid) {
        symtab = objectfiles[0]->symtab ;
    } else {
        Location &loc = frame_cache[current_frame]->get_loc() ;
        if (loc.has_debug_info()) {
            die = loc.get_funcloc()->symbol->die ; // subprogram die
            die->check_loaded() ;
            symtab = loc.get_symtab();
            if (autolang) {
                language = die->get_language() ;
            }
        } else {
            symtab = objectfiles[0]->symtab ;
        }
    }
}

void Process::print_variable_set (DIE *die, int language, std::vector<DIE*> &vars) {
    Frame *cf = frame_cache[current_frame] ;

    EvalContext context (this, cf->get_fp(), language, os) ;

    for (uint i = 0 ; i < vars.size() ; i++) {
        DIE *var = vars[i] ;
        std::string name = var->get_name() ;
        if (name != "<unknown>") {
            os.print ("%s = ", name.c_str()) ;
            Value value = var->evaluate (context) ;
            var->get_type()->print_value (context, value) ;
            os.print ("\n") ;
        }
    }
}

void Process::info (std::string root, std::string tail) {
    if (root == "args") {
        int language ;
        DIE *die ;
        get_current_subprogram (die, language) ;
        
        Subprogram *subprogram = dynamic_cast<Subprogram*>(die) ;
        std::vector<DIE*> paras ;
        subprogram->get_formal_parameters (paras) ;
        print_variable_set (die, language, paras) ;
    } else if (root == "address") {
    } else if (root == "all-registers") {
        print_regs(true) ;
    } else if (root == "program") {
        switch (state) {
        case IDLE:
            os.print ("The program being debugged is not being run.\n") ;
            break ;
        case READY:
            os.print ("        Using the running image of child process %d.\n", pid) ;  // XXX running?
            os.print ("Program stopped at 0x%llx.\n", get_reg("pc")) ;
            if (hitbp != NULL) {
                os.print ("It stopped at breakpoint %d\n", hitbp->get_num()) ;
            }
	default:
	    break; // added by bos for -Wall niceness
        }
    } else if (root == "catch") {
    } else if (root == "display") {
        if (displays.size() == 0) {
            os.print ("There are no auto-display expressions now.\n") ;
        } else {
            os.print ("Auto-display expressions now in effect:\n") ;
            os.print ("Num  Enb Expression\n") ;
            for (DisplayList::reverse_iterator i = displays.rbegin() ; i != displays.rend() ; i++) {
                Display *disp = *i ;
                os.print ("%-3d: %c  %s\n", disp->n, (disp->disabled?'n':'y'), disp->expr.c_str()) ;
            }
        }
    } else if (root == "frame") {
        show_frame() ;
    } else if (root == "functions") {
        build_frame_cache() ;
        SymbolTable *symtab = NULL ;
        bool autolang ;
        int language = get_language(autolang) ;
        DIE *die = NULL ;
        if (!frame_cache_valid) {
            symtab = objectfiles[0]->symtab ;
        } else {
            Location &loc = frame_cache[current_frame]->get_loc() ;
            if (loc.get_funcloc() != NULL) {
                die = loc.get_funcloc()->symbol->die ; // subprogram die
                die->check_loaded() ;
                symtab = loc.get_symtab();
                if (autolang) {
                    language = die->get_language() ;
                }
             } else {
                symtab = objectfiles[0]->symtab ;
             }
        }

        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        os.print ("All defined functions:\n\n") ;
        for (uint i = 0 ; i < objectfiles.size() ; i++) {
            if (objectfiles[i]->symtab != NULL) {
                objectfiles[i]->symtab->list_functions (context) ;
            }
        }
        os.print ("\nNon-debugging symbols:\n") ;
        for (uint i = 0 ; i < objectfiles.size() ; i++) {
            if (objectfiles[i]->elf->find_section (".debug_info") == NULL) {
                objectfiles[i]->elf->list_functions (os) ;
            }
        }

    } else if (root == "line") {
    } else if (root == "breakpoints" || root == "watchpoints") {
        list_breakpoints(false) ;
    } else if (root == "all-breakpoints") {
        list_breakpoints(true) ;
    } else if (root == "locals") {
        switch (state) {
        case IDLE:
            os.print ("The program being debugged is not being run.\n") ;
            break ;
        case READY: {
            int language ;
            DIE *die ;
            get_current_subprogram (die, language) ;
        
            Subprogram *subprogram = dynamic_cast<Subprogram*>(die) ;
            std::vector<DIE*> vars ;
            subprogram->get_local_variables (vars) ;
            print_variable_set (die, language, vars) ;
            } break ;
        default:
            break;
        }
    } else if (root == "proc") {
    } else if (root == "registers") {
        if (tail != "") {       
            print_reg (tail) ;
        } else {
            print_regs(false) ;
        }
    } else if (root == "scope") {
    } else if (root == "source") {
        int language ;
        DIE *die ;
        get_current_subprogram (die, language) ;
        if (die == NULL) {
            os.print ("No current source file.\n") ;
        } else {
            DwCUnit *cu = die->get_cunit() ;
            cu->info (os); 
        }
    } else if (root == "sharedlibrary") {
    } else if (root == "sources") {
        os.print ("Source files for which symbols have been read in:\n\n") ;
        // all files are read on demand
        os.print ("Source files for which symbols will be read in on demand:\n") ;
        int width = get_int_opt(PRM_WIDTH);
        for (uint i = 0 ; i < objectfiles.size() ; i++) {
            objectfiles[i]->symtab->list_source_files (os, width) ;
        }
    } else if (root == "stack") {
        stacktrace(-1) ;
    } else if (root == "symbol") {
    } else if (root == "signals") {
        os.print ("Signal        Stop      Print   Pass to program Description\n\n") ;
        if (tail == "") {
            signal_manager.show (0, os) ;
        } else {
            int signum = signal_manager.translate_signame (tail) ;
            signal_manager.show (signum, os) ;
        }
    } else if (root == "threads") {
        list_threads() ;
    } else if (root == "types") {
    } else if (root == "variables") {
        build_frame_cache() ;
        SymbolTable *symtab = NULL ;
        bool autolang ;
        int language = get_language(autolang) ;
        DIE *die = NULL ;
        if (!frame_cache_valid) {
            symtab = objectfiles[0]->symtab ;
        } else {
            Location &loc = frame_cache[current_frame]->get_loc() ;
            if (loc.get_funcloc() != NULL) {
                die = loc.get_funcloc()->symbol->die ; // subprogram die
                die->check_loaded() ;
                symtab = loc.get_symtab();
                if (autolang) {
                    language = die->get_language() ;
                }
             } else {
                symtab = objectfiles[0]->symtab ;
             }
        }

        Address frame_base = 0 ;
        if (die != NULL) {
            frame_base = die->get_frame_base (this) ;// calculate the value of the frame base
        }
        EvalContext context (this, frame_base, language, os) ;
        os.print ("All defined variables:\n\n") ;
        for (uint i = 0 ; i < objectfiles.size() ; i++) {
            if (objectfiles[i]->symtab != NULL) {
                objectfiles[i]->symtab->list_variables (context) ;
            }
        }
        os.print ("\nNon-debugging symbols:\n") ;
        for (uint i = 0 ; i < objectfiles.size() ; i++) {
            if (objectfiles[i]->elf->find_section (".debug_info") == NULL) {
                objectfiles[i]->elf->list_variables (os) ;
            }
        }
    } 
}

int Process::get_int_opt(CliParamId id) {
   return pcm->get_cli()->get_int_opt(id);
}

const char* Process::get_str_opt(CliParamId id) {
   return pcm->get_cli()->get_str_opt(id);
}


// signal manager

static Signal sigs[] = {
    {SIGHUP, "SIGHUP", "Hangup", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGINT, "SIGINT", "Interrupt", SIGACT_STOP | SIGACT_PRINT},
    {SIGQUIT, "SIGQUIT", "Quit", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGILL, "SIGILL", "Illegal instruction", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGTRAP, "SIGTRAP", "Trace trap", SIGACT_STOP | SIGACT_PRINT},
    {SIGABRT, "SIGABRT", "Abort", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGBUS, "SIGBUS", "BUS error", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGFPE, "SIGFPE", "Floating-point exception", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGKILL, "SIGKILL", "Kill, unblockable", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGUSR1, "SIGUSR1", "User-defined signal 1", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGSEGV, "SIGSEGV", "Segmentation violation", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGUSR2, "SIGUSR2", "User-defined signal 2", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGPIPE, "SIGPIPE", "Broken pipe", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGALRM, "SIGALRM", "Alarm clock", SIGACT_PASS},
    {SIGTERM, "SIGTERM", "Termination", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
#ifdef SIGSTKFLT
    {SIGSTKFLT, "SIGSTKFLT", "Stack fault", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
#endif
    {SIGCHLD, "SIGCHLD", "Child status has changed", SIGACT_PASS},
    {SIGCONT, "SIGCONT", "Continue", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGSTOP, "SIGSTOP", "Stop, unblockable", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGTSTP, "SIGTSTP", "Keyboard stop", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGTTIN, "SIGTTIN", "Background read from tty", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGTTOU, "SIGTTOU", "Background write to tty", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGURG, "SIGURG", "Urgent condition on socket", SIGACT_PASS},
    {SIGXCPU, "SIGXCPU", "CPU limit exceeded", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGXFSZ, "SIGXFSZ", "File size limit exceeded", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGVTALRM, "SIGVTALRM", "Virtual alarm clock", SIGACT_PASS},
    {SIGPROF, "SIGPROF", "Profiling alarm clock", SIGACT_PASS},
    {SIGWINCH, "SIGWINCH", "Window size change", SIGACT_PASS},
    {SIGIO, "SIGIO", "I/O now possible", SIGACT_PASS},
#ifdef SIGPWR
    {SIGPWR, "SIGPWR", "Power failure restart", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
#endif
    {SIGRTMIN, "SIGRTMIN", "Real-time signal", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {SIGRTMAX, "SIGRTMAX", "Real-time signal", SIGACT_STOP | SIGACT_PRINT | SIGACT_PASS},
    {0, NULL, NULL, 0}
} ;

SignalManager::SignalManager() {
    for (int i = 0 ; sigs[i].description != NULL ; i++) {
        signals[sigs[i].num] = sigs[i] ;
    }

    /* fill in the real time signals */
    int rmin = signals[SIGRTMIN].num;
    int rmax = signals[SIGRTMAX].num;
    for (int i=rmin+1; i < rmax; i++) {
        Signal tmp = signals[SIGRTMIN];

        char* nm;
        nm = (char*)malloc(strlen(tmp.name)+8);
        sprintf(nm, "%s+%d", tmp.name, i-rmin);

        tmp.num = i;
        tmp.name = nm;
        signals[i] = tmp;
    }
}

int SignalManager::hit (int sig, PStream &os) {
    SignalMap::iterator i = signals.find(sig) ;
    if (i == signals.end()) {
        return SIGACT_STOP | SIGACT_PRINT ;
    }
    return i->second.action ;
}

void SignalManager::print (int sig, PStream &os) {
    SignalMap::iterator i = signals.find(sig) ;
    if (i == signals.end()) {
        os.print ("Unknown signal %d", sig) ;
    } else {
        os.print ("%s, %s", i->second.name, i->second.description) ;
    }
}

static void showsig (PStream &os, Signal &s) {
    os.print ("%-13s %-9s %-7s %-15s %s\n", s.name,
       s.action & SIGACT_STOP ? "Yes" : "No",
       s.action & SIGACT_PRINT ? "Yes" : "No",
       s.action & SIGACT_PASS ? "Yes" : "No",
       s.description) ;
}

void SignalManager::show (int sig, PStream &os) {
    if (sig == 0) {                     // all signals
        for (SignalMap::iterator si = signals.begin() ; si != signals.end() ; si++) {
            showsig (os, si->second) ;
        }
    } else {
        SignalMap::iterator i = signals.find(sig) ;
        if (i == signals.end()) {
            os.print ("Unknown signal %d.\n", sig) ;
            return ;
        }
        Signal &s = i->second ;
        showsig (os, s) ;
    }
}


static void setsig (Signal &s, PStream &os, std::vector<std::string> &actions) {
    int acts = s.action ;
    for (uint i = 0 ; i < actions.size() ; i++) {
        std::string &p = actions[i] ;
        if (p == "pass") {
            acts |= SIGACT_PASS ;
        } else if (p == "nopass") {
            acts &= ~SIGACT_PASS ;
        } else if (p == "stop") {
            acts |= SIGACT_STOP ;
        } else if (p == "nostop") {
            acts &= ~SIGACT_STOP ;
        } else if (p == "print") {
            acts |= SIGACT_PRINT ;
        } else if (p == "noprint") {
            acts &= ~SIGACT_PRINT ;
        } else if (p == "ignore") {
            acts &= ~SIGACT_PASS ;
        } else if (p == "noignore") {
            acts |= SIGACT_PASS ;
        } else {
            throw Exception ("Invalid action keyword") ;
        }
    }
    s.action = acts ;
}

void SignalManager::set (int sig, std::vector<std::string> &actions, PStream &os) {
    if (sig == 0) {
        for (SignalMap::iterator si = signals.begin() ; si != signals.end() ; si++) {
            if (si->second.num != SIGTRAP && si->second.num != SIGINT) {              // these are special to debugger
                setsig (si->second, os, actions) ;
            }
        }
    } else {
        SignalMap::iterator si = signals.find(sig) ;
        if (si == signals.end()) {
            char* buf;
            Signal tmp;


            /* XXX: buf is leaked */
            buf = (char*)malloc(512);
            snprintf(buf,512,"SIG%d",sig);
            tmp.num = sig;
            tmp.name = buf;
            tmp.action = SIGACT_PASS | SIGACT_PRINT | SIGACT_STOP;
            tmp.description = "unknown signal";

            setsig (tmp, os, actions);
            signals[sig] = tmp;
        } else {
           setsig (si->second, os, actions) ;
       }
   }
}


int SignalManager::translate_signame (std::string name) {
    for (SignalMap::iterator si = signals.begin() ; si != signals.end() ; si++) {
        if (name == si->second.name) {
            return si->first ;
        }
    }
    throw Exception ("Unknown signal name") ;
}

void Process::set_signal_actions (std::string name, std::vector<std::string> &actions) {
    int signum = 0 ;
    if (name == "all") {
         // signum already is 0
    } else {
        if (isdigit (name[0])) {
            signum = strtol (name.c_str(), NULL, 0) ;
        } else {
            signum = signal_manager.translate_signame (name) ;
        }
    }
    signal_manager.set (signum, actions, os) ;
    os.print ("Signal        Stop      Print   Pass to program Description\n") ;
    signal_manager.show (signum, os) ;
}

Location Process::get_current_location() {
    build_frame_cache() ;
    if (frame_cache_valid) {
        current_location = frame_cache[current_frame]->get_loc() ;
    }
    return current_location ;
}

void Process::spawn_cli (Address endsp) {
    pcm->get_cli()->run (this, endsp) ;
}

void Process::stop_hook() {
    pcm->get_cli()->stop_hook() ;
}

std::string Process::realname(std::string nm) {
   return pcm->realname(nm) ;
}


void Process::print_loc(const Location& loc, Frame *frame, PStream &os, bool showthread) {
    bool print_address = get_int_opt(PRM_P_ADDR) ;

    bool first_line = loc.get_funcloc() == NULL ?
        false : loc.get_funcloc()->at_first_line (loc.get_addr()) ;

    std::string funcname = loc.get_symname();

    if (loc.get_file() == NULL) {
        const char *parens = " ()" ;
        if (funcname != "") {
            if (get_arch()->in_sigtramp (this, funcname)) {
                funcname = "<called by signal handler>" ;
                parens = "" ;
            }
            // don't print () if it's already done
            if (funcname.find('(') != std::string::npos) {
                parens = "" ;
            }
            if (print_address && !first_line) {
                os.print ("0x%016llx in %s%s", loc.get_addr(), funcname.c_str(), parens) ;
            } else {
                os.print ("%s%s", funcname.c_str(), parens) ;
            }
        } else {
            if (print_address && !first_line) {
                os.print ("0x%016llx in ?? ()", loc.get_addr()) ;
            } else {
                os.print ("?? ()") ;
            }
        }
    } else {
        if (print_address && !first_line) {
           os.print ("0x%016llx in %s ", loc.get_addr(), funcname.c_str()) ;
        } else {
           os.print ("%s ", funcname.c_str()) ;
        }
        if (loc.get_funcloc() != NULL) {
           print_function_paras (frame, loc.get_funcloc()->symbol->die) ;
        }

        os.print (" at %s", loc.get_file()->name.c_str());
        if (loc.get_line() != -1) {
           os.print (":%d", loc.get_line());
        }
    }
    if (showthread) {
        show_current_thread() ;
    }
    os.print ("\n") ;
}

void Process::build_local_map (DIE *die, LocalMap &m) {
    Subprogram *subprogram = dynamic_cast<Subprogram*>(die) ;
    std::vector<DIE*> vars ;
    subprogram->get_local_variables (vars) ;

    for (uint i = 0 ; i < vars.size() ; i++) {
        DIE *var = vars[i] ;
        if (var->is_local_var()) {
            int offset = var->get_local_offset() ;
            m[offset] = var ;
        }
    }

    std::vector<DIE*> paras ;
    subprogram->get_formal_parameters (paras) ;

    for (uint i = 0 ; i < paras.size() ; i++) {
        DIE *para = paras[i] ;
        if (para->is_local_var()) {
            int offset = para->get_local_offset() ;
            m[offset] = para ;
        }
    }

}

