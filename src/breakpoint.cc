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

file: breakpoint.cc
created on: Fri Aug 13 11:07:33 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "breakpoint.h"
#include "process.h"
#include "junk_stream.h"
#include "arch.h"
#include <link.h>
#include "dbg_thread_db.h"
#include "cli.h"
#include "gen_loc.h"

Breakpoint::Breakpoint (Architecture *_arch, Process *_proc, std::string _text, Address _addr, int _num)
    : addr(_addr),
      disabled(false),
      arch(_arch),
      proc(_proc),
      num(_num),
      text(_text),
      condition(NULL),
      ignore_count(0),
      hit_count(0),
      applied(false),
      removed(false),
      silent(false),
      pending(false),
      disp(DISP_KEEP)
{
}

Breakpoint::~Breakpoint() {
    for (uint i = 0 ; i < commands.size() ; i++) {
        delete commands[i] ;
    }
}

void Breakpoint::set_thread (int n) {
    proc->validate_thread (n) ;
    threads.push_back(n) ;
}

// only copy the fields that should survive a rerun
void Breakpoint::copy (Breakpoint *dest) {
    dest->condition_expr = condition_expr ;
    dest->condition = NULL ;
    dest->ignore_count = ignore_count ;
    for (uint i = 0 ; i < commands.size() ; i++) {
        dest->commands.push_back (commands[i]->clone()) ;
    }
    dest->silent = silent ;
    dest->disp = disp ;
    dest->threads = threads ;
}


Breakpoint_action Breakpoint::hit (PStream &os) {
    bool threadok = threads.size() == 0 ;

    for (uint i = 0 ; i < threads.size() ; i++) {
        if (proc->get_current_thread() == threads[i]) {
            threadok = true ;
            break ;
        }
    }
    if (!threadok) {
        return BP_ACTION_CONT ;
    }

    hit_count++ ;
    // check ignore count
    if (ignore_count > 0) {
        if (ignore_count-- > 0) {
            return BP_ACTION_CONT ;
        }
    }

    // check condition
    if (condition_expr != "") {
        try {
            if (condition == NULL) {
                int end  ;
                condition = proc->compile_expression (condition_expr, end) ;
            }
            if (condition != NULL) {
                if ((int)proc->evaluate_expression (condition) == 0) {
                    return BP_ACTION_CONT ;
                }
            }
        } catch (const char *s) {
            os.print (s) ;
        } catch (std::string s) {
            os.print (s.c_str()) ;
        }
    }

    // inject the commands, if any are present

    for (uint i = 0 ; i < commands.size() ; i++) {
        proc->get_cli()->inject_command (commands[i]) ;
    }

    // breakpoint is active, get it to do its stuff
    return this->hit_active (os) ;
}

void Breakpoint::disable() {
    if (!disabled) {
        clear() ;
        disabled = true ;
    }
}

void Breakpoint::enable(bool now) {
   if (disabled) {
       if (now) {
           set() ;
       }
       disabled = false ;
   }
}

bool Breakpoint::is_disabled() {
    return disabled ;
}

void Breakpoint::reset(Process *newproc) {
    applied = false ;
    proc = newproc ;
}

void Breakpoint::set_condition (std::string cond)  {
    condition_expr = cond ;
    condition = NULL ;
}

void Breakpoint::set_ignore_count (int n) {
    ignore_count = n ;
    hit_count = 0 ;
}

void Breakpoint::set_commands (std::vector<ComplexCommand *> &cmds) {
    // the string "silent" on the first line says that the breakpoint should be silent
    // this is not really a command
    silent = false ;
    if (cmds.size() > 0) {
        ComplexCommand *cmd0 = cmds[0] ;
        if (cmd0->code == CMD_REG && cmd0->head == "silent") {
            silent = true ;
            // remove the 'silent' command
            for (uint i = 0 ; i < cmds.size() - 1 ; i++) {
                cmds[i] = cmds[i+1] ;
            }
            cmds.resize (cmds.size() - 1) ;
            delete cmd0 ;
        }
    }
    commands = cmds ;
}

const char *Breakpoint::get_disposition() {
    switch (disp) {
    case DISP_KEEP:
        return "keep" ;
        break ;
    case DISP_DELETE:
        return "del" ;
        break ;
    case DISP_DISABLE:
        return "dis" ;
        break ;
    default:
        return "unk" ;
    }
}


SoftwareBreakpoint::SoftwareBreakpoint (Architecture *_arch, Process *_proc, std::string _text, Address _addr, int _num)
    : Breakpoint (_arch, _proc, _text, _addr, _num),
    oldvalue(0),
    deleted(false) {

    location = proc->lookup_address (_addr) ;
}

SoftwareBreakpoint::~SoftwareBreakpoint() {
    clear() ;
}

void SoftwareBreakpoint::set_address (Address a) {
    addr = a ;
    location = proc->lookup_address (addr) ;
}

void SoftwareBreakpoint::set() {
    if (!disabled && !applied) {
        if (proc->is_running()) {
            if (arch->breakpoint_present (proc, addr)) {
                //std::cout << "there is already a breakpoint present at this address\n" ;
            } else {
                //std::cout << "setting breakpoint "  <<  num  <<  " at addr 0x"  <<  std::hex << addr << std::dec << '\n' ;
                oldvalue = arch->set_breakpoint (proc, addr) ;
                applied = true ;
            }
        }
    }
}

void SoftwareBreakpoint::tempremove() {
    clear() ;
    removed = true ;
}

void SoftwareBreakpoint::temprestore() {
    set() ;
    removed = false ;
}

void SoftwareBreakpoint::clear() {
    if (!disabled && applied) {
        //std::cout << "clearing breakpoint "  <<  num << '\n' ;
        if (proc->is_running()) {
            proc->write (addr, oldvalue) ;
        }
        applied = false ;
    }
}

void SoftwareBreakpoint::attach() {
    if (!disabled && applied) {
        if (proc->is_running()) {
            if (arch->breakpoint_present (proc, addr)) {
                //std::cout << "there is already a breakpoint present at this address\n" ;
            } else {
                //std::cout << "setting breakpoint "  <<  num  <<  " at addr 0x"  <<  std::hex << addr << std::dec << '\n' ;
                oldvalue = arch->set_breakpoint (proc, addr) ;
                applied = true ;
            }
            //std::cout << "clearing breakpoint "  <<  num << '\n' ;
            proc->write (addr, oldvalue) ;
        }
    }
}

void SoftwareBreakpoint::detach() {
    if (!disabled && applied) {
        if (proc->is_running()) {
            //std::cout << "clearing breakpoint "  <<  num << '\n' ;
            proc->write (addr, oldvalue) ;
        }
    }
}

void SoftwareBreakpoint::print_location(PStream &os) {
//     if ( location.is_known() ) {
//         proc->print_loc(location, proc->get_current_frame(), os, true);
//         location.show_line(os, proc->get_cli()->isemacs());
//     } else {
//         proc->print_loc(location, proc->get_current_frame(), os, true) ;
//     }
}

void SoftwareBreakpoint::print_short_location(PStream &os) {
    if ( location.is_known() ) {
        os.print (": file %s, line %d.",
          location.get_filename().c_str(),
          location.get_line()) ;
    }
}

void SoftwareBreakpoint::print_details(PStream &os) {
    os.print ("%-3d %-14s %-4s %-3s ", num, get_type(), get_disposition(),
                            is_disabled()?"n":"y") ;
    if (show_address()) {
        if (is_pending()) {
            os.print ("<PENDING>          %s", text.c_str()) ;
        } else {
            os.print ("0x%016llx ", addr) ;
        }
    }
    if (location.get_file() != NULL) {
        os.print ("in %s at %s:%d",
           location.get_symname().c_str(),
           location.get_filename().c_str(),
           location.get_line()) ;
    }
    os.print ("\n") ;
    if (hit_count > 0) {
       os.print ("        breakpoint already hit %d time%s\n", hit_count, hit_count==1?"":"s") ;
    }
    if (condition_expr != "") {
        os.print ("        stop only if %s\n", condition_expr.c_str()) ;
    }
    if (threads.size() > 0) {
        bool comma = false ;
        os.print ("        stop only in thread%s ", threads.size() == 1 ? "" : "s") ;
        for (uint i = 0 ; i < threads.size() ; i++) {
            if (comma) os.print (", ") ;
            comma = true ;
            os.print ("%d", threads[i]) ;
        }
        os.print ("\n") ;
    }
    if (ignore_count > 0) {
        os.print ("        ignore next %d hits\n", ignore_count) ;
    }
    if (silent) {
        os.print ("        silent\n") ;
    }
    for (uint i = 0 ; i < commands.size() ; i++) {
        commands[i]->print (os, 8) ;
    }
}



void SoftwareBreakpoint::remove() {
    deleted = true ;
}




// this is called when a breakpoint is hit.  It should check the condition
// if any and call hit_active to get the actual breakpoint action

UserBreakpoint::UserBreakpoint (Architecture *_arch, Process *_proc, std::string _text, Address _addr, int _num)
    : SoftwareBreakpoint(_arch, _proc, _text, _addr, _num)
 {

}

Breakpoint *UserBreakpoint::clone() {
    UserBreakpoint *bp = new UserBreakpoint (arch, proc, text, addr, num) ;
    copy (bp) ;
    return bp ;
}

UserBreakpoint::~UserBreakpoint() {
}

Breakpoint_action UserBreakpoint::hit_active(PStream &os) {
    proc->stop_hook() ;
    if (!silent) {
        os.print ("Breakpoint %d, ", num) ;
        print_location(os) ;
    }
    switch (disp) {
    case DISP_KEEP:
        return BP_ACTION_STOP ;
    case DISP_DELETE:
        clear() ;
        remove() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        return BP_ACTION_STOP ;
    case DISP_DISABLE:
        disable() ;
        return BP_ACTION_STOP ;
    }
    throw Exception("not reached");
}

void UserBreakpoint::print(PStream &os) {
    if (!silent) {
        os.print ("Breakpoint %d, ", num) ;
        print_location(os) ;
    }
}

Catchpoint::Catchpoint (Architecture *_arch, Process *_proc, int _num, CatchpointType _type, std::string _data)
    : SoftwareBreakpoint(_arch, _proc, "", 0, _num), type(_type), data(_data)
 {
}

void Catchpoint::set() {
    if (addr == 0) {                    // address not set yet?
        switch (type) {
        case CATCH_THROW:
            addr = proc->lookup_symbol ("__cxa_throw") ;
            break ;
        case CATCH_CATCH:
            addr = proc->lookup_symbol ("__cxa_begin_catch") ;
            break ;
        case CATCH_FORK:
            addr = proc->lookup_symbol ("fork") ;
            break ;
        case CATCH_VFORK:
            addr = proc->lookup_symbol ("vfork") ;
            break ;
        case CATCH_EXEC:
            addr = proc->lookup_symbol ("execve") ;
            break ;
        case CATCH_SIGNAL:
            break ;
        case CATCH_THREAD:
            break ;
        case CATCH_PROCESS:
            break ;
        case CATCH_LOAD:
            break ;
        case CATCH_UNLOAD:
            break ;
        case CATCH_STOP:
            break ;
        }
        if (addr != 0) {
            proc->add_breakpoint (this, true) ;         // just update it
        }
    }
    if (addr != 0) {
        SoftwareBreakpoint::set() ;
    } else {
        throw Exception ("Unable to set catchpoint") ;
    }
}

void Catchpoint::clear() {
    if (addr != 0) {
        SoftwareBreakpoint::clear() ;
    }
}

void Catchpoint::attach() {
    if (addr != 0) {
        SoftwareBreakpoint::attach() ;
    }
}

void Catchpoint::detach() {
    if (addr != 0) {
        SoftwareBreakpoint::detach() ;
    }
}

Breakpoint *Catchpoint::clone() {
    Catchpoint *bp = new Catchpoint (arch, proc, num, type, data) ;
    copy (bp) ;
    return bp ;
}

Catchpoint::~Catchpoint() {
}

Breakpoint_action Catchpoint::hit_active(PStream &os) {
    proc->stop_hook() ;
    if (!silent) {
        os.print ("Catchpoint %d, (%s)\n", num, info()) ;
    }
    switch (disp) {
    case DISP_KEEP:
        return BP_ACTION_STOP ;
    case DISP_DELETE:
        clear() ;
        remove() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        return BP_ACTION_STOP ;
    case DISP_DISABLE:
        disable() ;
        return BP_ACTION_STOP ;
    }
    throw Exception("not reached");
}

void Catchpoint::print(PStream &os) {
    if (!silent) {
        os.print ("Catchpoint %d, (%s)\n", num, info()) ;
    }
}

const char *Catchpoint::info() {
    switch (type) {
    case CATCH_THROW:
        return "exception thrown" ;
        break ;
    case CATCH_CATCH:
        return "exception caught" ;
        break ;
    case CATCH_FORK:
        return "program forked" ;
        break ;
    case CATCH_VFORK:
        return "program vforked" ;
        break ;
    case CATCH_EXEC:
        return "program vforked" ;
        break ;
    case CATCH_SIGNAL:
        return "signal caught" ;
        break ;
    case CATCH_THREAD:
        return "thread event" ;
        break ;
    case CATCH_PROCESS:
        return "process event" ;
        break ;
    case CATCH_LOAD:
        return "load event" ;
        break ;
    case CATCH_UNLOAD:
        return "unload event" ;
        break ;
    case CATCH_STOP:
        return "program stopped" ;
        break ;
    }
    throw Exception("not reached");
}

const char *Catchpoint::get_type() {
    switch (type) {
    case CATCH_THROW:
        return "catch throw" ;
        break ;
    case CATCH_CATCH:
        return "catch catch" ;
        break ;
    case CATCH_FORK:
        return "catch fork" ;
        break ;
    case CATCH_VFORK:
        return "catch vfork" ;
        break ;
    case CATCH_EXEC:
        return "catch exec" ;
        break ;
    case CATCH_SIGNAL:
        return "catch signal" ;
        break ;
    case CATCH_THREAD:
        return "catch thread" ;
        break ;
    case CATCH_PROCESS:
        return "catch process" ;
        break ;
    case CATCH_LOAD:
        return "catch load" ;
        break ;
    case CATCH_UNLOAD:
        return "catch unload" ;
        break ;
    case CATCH_STOP:
        return "catch stop" ;
        break ;
    }
    throw Exception("not reached");
}


TempBreakpoint::TempBreakpoint (Architecture *_arch, Process *_proc, Address _addr, int _num)
    : SoftwareBreakpoint(_arch, _proc, "", _addr, _num)
 {
    set_disposition (DISP_DELETE) ;
}

Breakpoint *TempBreakpoint::clone() {
    TempBreakpoint *bp = new TempBreakpoint (arch, proc, addr, num) ;
    copy (bp) ;
    return bp ;
}

TempBreakpoint::~TempBreakpoint() {
}

Breakpoint_action TempBreakpoint::hit_active(PStream &os) {
    //std::cout << "temp breakpoint hit" << '\n' ;
    clear() ;
    remove() ;
    proc->remove_breakpoint(this) ;
    proc->record_breakpoint_deletion (this) ;
    return BP_ACTION_STOP ;
}

StepBreakpoint::StepBreakpoint (Architecture *_arch, Process *_proc, Address _addr, int _num)
    : SoftwareBreakpoint(_arch, _proc, "", _addr, _num), fp(0)
 {
    fp = proc->get_reg ("fp") ;                 // read the current frame pointer
}

Breakpoint *StepBreakpoint::clone() {
    StepBreakpoint *bp = new StepBreakpoint (arch, proc, addr, num) ;
    bp->fp = fp ;
    copy (bp) ;
    return bp ;
}

StepBreakpoint::~StepBreakpoint() {
}

Breakpoint_action StepBreakpoint::hit_active(PStream &os) {
    Address currframe = proc->get_reg ("fp") ;
    if (currframe < fp) {
        //printf ("step breakpoint ignored because frame is different\n") ;
        return BP_ACTION_CONT ;
    }
    //std::cout << "step breakpoint hit" << '\n' ;
    clear() ;
    remove() ;
    proc->remove_breakpoint(this) ;
    proc->record_breakpoint_deletion (this) ;
    return BP_ACTION_IGNORE ;
}

CascadeBreakpoint::CascadeBreakpoint (Architecture *_arch, Process *_proc, Address _addr, int _num)
    : SoftwareBreakpoint(_arch, _proc, "", _addr, _num)
 {
}

Breakpoint *CascadeBreakpoint::clone() {
    CascadeBreakpoint *bp = new CascadeBreakpoint (arch, proc, addr, num) ;
    copy (bp) ;
    return bp ;
}

CascadeBreakpoint::~CascadeBreakpoint() {
}

Breakpoint_action CascadeBreakpoint::hit_active(PStream &os) {
    //printf ("cascade breakpoint hit\n") ;
    // hit all the children.   If one of the children says to stop the program then
    // this is returned to the caller
    Breakpoint_action action = BP_ACTION_CONT ;
    for (uint i = 0 ; i < children.size() ; i++) {
        Breakpoint_action a = children[i]->hit (os) ;
        if (a == BP_ACTION_STOP) {
            action = a ;
        }
    }

    // now clear the breakpoint
    clear() ;
    remove() ;
    proc->remove_breakpoint(this) ;
    proc->record_breakpoint_deletion (this) ;
    return action ;
}

DynamicLinkBreakpoint::DynamicLinkBreakpoint (Architecture *_arch, Process *_proc, Address _addr, int _num)
    : SoftwareBreakpoint(_arch, _proc, "", _addr, _num),
    phase(1) {


}

Breakpoint *DynamicLinkBreakpoint::clone() {
    DynamicLinkBreakpoint *bp = new DynamicLinkBreakpoint (arch, proc, addr, num) ;
    copy (bp) ;
    bp->phase = phase ;
    return bp ;
}

DynamicLinkBreakpoint::~DynamicLinkBreakpoint() {
}

Breakpoint_action DynamicLinkBreakpoint::hit_active(PStream &os) {
    //std::cout << "breakpoint in dynamic linker hit" << '\n' ;
    if (phase == 1) {                               // phase 1, remove fixup bp and 
        Address returnaddr = proc->get_return_addr() ;// get return address 
        clear() ;                                     // clear this bp
        proc->remove_breakpoint(this) ;                // remove it from the address map
        addr = returnaddr ;                           // and move to return address
        proc->add_breakpoint (this) ;                  // reinsert at new address
        set() ;                                       // set it again at return addres
        phase = 2 ;
    } else {
        clear() ;
        remove() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        proc->resume_stepping() ;
    }
    return BP_ACTION_CONT ;
}

ThreadBreakpoint::ThreadBreakpoint (Architecture *_arch, Process *_proc, Address _addr, int _num, bool _iscreate)
    : SoftwareBreakpoint(_arch, _proc, "", _addr, _num),
    iscreate(_iscreate) {

}

Breakpoint *ThreadBreakpoint::clone() {
    ThreadBreakpoint *bp = new ThreadBreakpoint (arch, proc, addr, num, iscreate) ;
    copy (bp) ;
    return bp ;
}

ThreadBreakpoint::~ThreadBreakpoint() {
}

Breakpoint_action ThreadBreakpoint::hit_active(PStream &os) {
    //std::cout << "thread breakpoint hit" << '\n' ;
    int event = 0 ;
    void * threadhandle = 0 ;
    void * data = 0 ;
    thread_db::get_event_message (proc->thread_agent, event, threadhandle, data) ;
    //std::cout << "event: "  <<  event  <<  " in thread "  <<  threadhandle << '\n' ;
    thread_db::enable_thread_events (proc->thread_agent, threadhandle, 1) ;
    proc->new_thread (threadhandle) ;
#if defined (__linux__)
    return BP_ACTION_CONT ;
#elif defined (__FreeBSD__)
    return BP_ACTION_WAIT;
#endif
}

SharedLibraryBreakpoint::SharedLibraryBreakpoint (Architecture *_arch, Process *_proc, Address _addr, int _num)
    : SoftwareBreakpoint(_arch, _proc, "", _addr, _num)  {

}

Breakpoint *SharedLibraryBreakpoint::clone() {
    SharedLibraryBreakpoint *bp = new SharedLibraryBreakpoint (arch, proc, addr, num) ;
    copy (bp) ;
    return bp ;
}


SharedLibraryBreakpoint::~SharedLibraryBreakpoint() {
}

Breakpoint_action SharedLibraryBreakpoint::hit_active(PStream &os) {
    //std::cout << "shared library breakpoint hit" << '\n' ;
    Address state = proc->get_r_debug_state() ;
    //println ("r_debug state = " + state)
    const char *event = "unknown" ;
    switch (state) {
    case r_debug::RT_CONSISTENT: {             // library is consistent
        LinkMap *lm = proc->get_new_link_map() ;        
        if (lm != NULL) {
            std::string name = lm->get_name() ;
            if (name != "") {
                Address base = lm->get_base() ;
                proc->open_object_file (name, base) ;
            }
        }
        proc->resolve_pending_breakpoints() ;           // try to resolve pending breakpoints
        event = "library has become consistent" ;
        break ;
        }
    case r_debug::RT_ADD:                    // new library is being added
        event = "library is being added" ;
        break ;
    case r_debug::RT_DELETE:                 // library is being removed
        event = "library is being removed" ;
        break ;
    default: ;
       throw Exception ("Unknown r_debug state: ") ;
    }
    if (proc->get_int_opt(PRM_STOP_SL) != 0) {
     
        os.print ("Stopped on shared library event (%s)\n", event) ;
        return BP_ACTION_STOP ;
    }
    return BP_ACTION_CONT ;
}


//
// general watchpoint
//

Watchpoint::Watchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr, Address _addr, int _size, int _num)
    : Breakpoint (_arch, _proc, _expr, _addr, _num), expr(_expr), compiled_expr(cexpr), size(_size), local(false) {

    try {
        fp = proc->get_reg ("fp") ;                 // read the current frame pointer

        Address pc = proc->get_reg ("pc");
        Location loc = proc->lookup_address (pc);
        FunctionLocation* floc = loc.get_funcloc();
        start_pc = floc->get_start_address();
        end_pc = floc->get_end_address();

    } catch (Exception e) {
        fp = 0 ;
        start_pc = 0;
        end_pc = 0;
    }

    bool isauto ;
    language = proc->get_language (isauto) ;
}


Watchpoint::~Watchpoint() {
    delete compiled_expr ;
}

void Watchpoint::print_location(PStream &os) {
//     Address pc = proc->get_reg ("pc") ;
//     Location loc = proc->lookup_address (pc) ;
//     if (loc.get_file() != NULL) {
//         proc->print_loc(loc, proc->get_current_frame(), os, true) ;
//         loc.show_line(os, proc->get_cli()->isemacs()) ;
//     } else {
//         os.print ("at address 0x%llx\n", pc) ;
//     }
}


void Watchpoint::print_short_location (PStream &os) {
    os.print ("%s", expr.c_str()) ;
}

void Watchpoint::print_details (PStream &os) {
    os.print ("%-3d %-14s %-4s %-3s ", num, get_type(), is_temp() ? "del":"keep",
                            is_disabled()?"n":"y") ;
    if (is_pending()) {
        os.print ("<PENDING>          ") ;
    } else {
        os.print ("0x%016llx ", addr) ;
    }
    if (is_hw_breakpoint()) {
        dynamic_cast<HardwareBreakpoint*>(this)->show_location_details (os) ;
    } else {
        os.print (" %s", expr.c_str()) ;
    }
    os.print ("\n") ;
    if (hit_count > 0) {
       os.print ("        breakpoint already hit %d time%s\n", hit_count, hit_count==1?"":"s") ;
    }
    if (condition_expr != "") {
        os.print ("        stop only if %s\n", condition_expr.c_str()) ;
    }
    if (threads.size() > 0) {
        bool comma = false ;
        os.print ("        stop only in thread%s ", threads.size() == 1 ? "" : "s") ;
        for (uint i = 0 ; i < threads.size() ; i++) {
            if (comma) os.print (", ") ;
            comma = true ;
            os.print ("%d", threads[i]) ;
        }
        os.print ("\n") ;
    }
    if (ignore_count > 0) {
        os.print ("        ignore next %d hits\n", ignore_count) ;
    }
    if (silent) {
        os.print ("        silent\n") ;
    }
    for (uint i = 0 ; i < commands.size() ; i++) {
        commands[i]->print (os, 8) ;
    }
}
                                                                                                                                                     
void Watchpoint::copy (Breakpoint *bp) {
    Breakpoint::copy (bp) ;
    Watchpoint *wp = dynamic_cast<Watchpoint*>(bp) ;
    fp = wp->fp ;
    local = wp->local ;
}

// return true if the scope for the watchpoint is valid
bool Watchpoint::check_scope() {
    if (local) {
        if (!proc->stack_contains(fp, start_pc, end_pc)) {
            proc->get_os().print ("Watchpoint %d deleted because the program has "
                 "left the block in which its expression is valid.\n", get_num()) ;
            print_location (proc->get_os()) ;
            clear() ;
            proc->remove_breakpoint(this) ;
            proc->record_breakpoint_deletion (this) ;
            return false ;
        }
    }
    return true ;
}

SoftwareWatchpoint::SoftwareWatchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr,  Address _addr, int _size, int _num)
    : Watchpoint (_arch, _proc, _expr, cexpr, _addr, _size, _num), current_value(0) {
}

Breakpoint *SoftwareWatchpoint::clone() {
    SoftwareWatchpoint *bp = new SoftwareWatchpoint (arch, proc, expr, compiled_expr, addr, size, num) ;
    copy (bp) ;
    bp->current_value = current_value ;
    return bp ;
}

SoftwareWatchpoint::~SoftwareWatchpoint() {
}

void SoftwareWatchpoint::set() {
    if (fp == 0) {
        fp = proc->get_reg ("fp") ;
    }
    if (!disabled && !applied) {
        EvalContext context (proc, proc->get_reg ("fp"), language, proc->get_os()) ;
        current_value = compiled_expr->evaluate (context) ;
        applied = true ;
    }
}

void SoftwareWatchpoint::clear() {
    applied = false ;
}

void SoftwareWatchpoint::attach() {
    set() ;
}

void SoftwareWatchpoint::detach() {
    clear() ;
}


Breakpoint_action SoftwareWatchpoint::hit_active(PStream &os) {
    if (!check_scope()) {       
printf ("out of scope\n") ;
        return BP_ACTION_STOP ;
    }
    EvalContext context (proc, proc->get_reg ("fp"), language, proc->get_os()) ;
    Value newval = compiled_expr->evaluate (context) ;
    if (newval != current_value) {
        if (!silent) {
            show_header (os) ;
            os.print ("%s\n", expr.c_str()) ;
            os.print ("Old value = ") ;

            /* XXX: replace me */
            JunkStream* js;
            js = reinterpret_cast<JunkStream*>(&os);
            js->print (current_value) ;

            os.print ("\nNew value = ") ;

            /* XXX: replace me */
            js = reinterpret_cast<JunkStream*>(&os);
            js->print (newval) ;

            os.print ("\n") ;
            print_location (os) ;
        }
        current_value = newval ;
        switch (disp) {
        case DISP_KEEP:
            return BP_ACTION_STOP ;
        case DISP_DELETE:
            clear() ;
            proc->remove_breakpoint(this) ;
            proc->record_breakpoint_deletion (this) ;
            return BP_ACTION_STOP ;
        case DISP_DISABLE:
            disable() ;
            return BP_ACTION_STOP ;
        }
        return BP_ACTION_STOP ;
    }
    hit_count-- ;                       // decrement hit count because we weren't actually hit
    return BP_ACTION_CONT ;
}

void SoftwareWatchpoint::show_header (PStream &os) {
    os.print ("Watchpoint %d: ", get_num()) ;
}


HardwareWatchpoint::HardwareWatchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr, Address _addr, WatchpointType _type, int _size, int _num)
    : Watchpoint (_arch, _proc, _expr, cexpr, _addr, _size, _num),
      current_value(0),
      type(_type) 
{
    debug_reg = arch->allocate_debug_reg(_addr) ;            // allocate a debug reg
}

Breakpoint *HardwareWatchpoint::clone() {
    HardwareWatchpoint *bp = new HardwareWatchpoint (arch, proc, expr, compiled_expr, addr, type, size, num) ;
    copy (bp) ;
    bp->debug_reg = debug_reg ;
    bp->current_value = current_value ;
    return bp ;
}

HardwareWatchpoint::~HardwareWatchpoint() {
}

void HardwareWatchpoint::set() {
    if (fp == 0) {
        fp = proc->get_reg ("fp") ;
    }
    if (!disabled && !applied) {
        arch->set_watchpoint (proc, debug_reg, type, size) ;
        if (compiled_expr == NULL) {
            current_value = proc->read (addr, size) ;
        } else {
            EvalContext context (proc, proc->get_reg ("fp"), language, proc->get_os()) ;
            current_value = compiled_expr->evaluate (context) ;
            if (compiled_expr->get_type()->is_real()) {
                if (compiled_expr->get_type()->get_real_size(context) == 4) {
                    current_value.real = (double)(*(float*)(void*)&current_value.real) ;
                }
            }
        }
        applied = true ;
    }
}

void HardwareWatchpoint::clear() {
    if (!disabled && applied) {
        arch->clear_watchpoint (proc, debug_reg) ;
        arch->free_debug_reg (debug_reg) ;
        applied = false ;
    }
}

void HardwareWatchpoint::attach() {
    if (!disabled && applied) {
        arch->set_watchpoint (proc, debug_reg,  type, size) ;
    }
}

void HardwareWatchpoint::detach() {
    if (!disabled && applied) {
        arch->clear_watchpoint (proc, debug_reg) ;
    }
}

void HardwareWatchpoint::disable() {
    if (!disabled) {
        if (applied) { 
            Breakpoint::disable() ;             // will also free reg
        } else {        
            arch->free_debug_reg (debug_reg) ;
            disabled = true ;
        }
    }
}

void HardwareWatchpoint::enable(bool now) {
    if (disabled) {
        debug_reg = arch->allocate_debug_reg(addr) ;
        Breakpoint::enable (now) ;
    }
}

Breakpoint_action HardwareWatchpoint::hit_active(PStream &os) {
    if (!check_scope()) {
        return BP_ACTION_STOP ;
    }
    if (!silent) {
        show_header (os) ;
        os.print ("%s\n", expr.c_str()) ;
        print_location(os) ;
    }
    return BP_ACTION_STOP ;
}

void HardwareWatchpoint::show_header (PStream &os) {
    os.print ("Hardware watchpoint %d: ", get_num()) ;
}

//
// watchpoint for a change in value
//

ChangeWatchpoint::ChangeWatchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr, Address _addr, int _size, int _num)
    : HardwareWatchpoint (_arch, _proc, _expr, cexpr, _addr, WP_RW, _size, _num) {
}

Breakpoint *ChangeWatchpoint::clone() {
    ChangeWatchpoint *bp = new ChangeWatchpoint (arch, proc, expr, compiled_expr, addr, size, num) ;
    copy (bp) ;
    return bp ;
}

ChangeWatchpoint::~ChangeWatchpoint() {
}

Breakpoint_action ChangeWatchpoint::hit_active(PStream &os) {
    if (!check_scope()) {
        return BP_ACTION_STOP ;
    }
    Value newval ;
    if (compiled_expr == NULL) {
        newval = proc->read (addr,size) ;
    } else {
        EvalContext context (proc, fp, language, proc->get_os()) ;
        newval = compiled_expr->evaluate (context) ;
        if (compiled_expr->get_type()->is_real()) {
            if (compiled_expr->get_type()->get_real_size(context) == 4) {
                newval.real = (double)(*(float*)(void*)&newval.real) ;
            }
        }
    }

    if (newval != current_value) {
        if (!silent) {
            show_header (os) ;
            os.print ("%s\n", expr.c_str()) ;
            os.print ("Old value = ") ;

            /* XXX: replace me */
            JunkStream* js;
            js = reinterpret_cast<JunkStream*>(&os);
            js->print (current_value) ;

            os.print ("\nNew value = ") ;

            /* XXX: replace me */
            js = reinterpret_cast<JunkStream*>(&os);
            js->print (newval) ;

            os.print ("\n") ;
            print_location(os) ;
        }
        current_value = newval ;
        switch (disp) {
        case DISP_KEEP:
            return BP_ACTION_STOP ;
        case DISP_DELETE:
            clear() ;
            proc->remove_breakpoint(this) ;
            proc->record_breakpoint_deletion (this) ;
            return BP_ACTION_STOP ;
        case DISP_DISABLE:
            disable() ;
            return BP_ACTION_STOP ;
        }
        return BP_ACTION_STOP ;
    }
    hit_count-- ;                       // decrement hit count because we weren't actually hit
    return BP_ACTION_CONT ;
}

//
// watchpoint for a read/write of address
//

ReadWriteWatchpoint::ReadWriteWatchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr, Address _addr, int _size, int _num)
    : HardwareWatchpoint (_arch, _proc, _expr, cexpr, _addr, WP_RW, _size, _num) {
}

Breakpoint *ReadWriteWatchpoint::clone() {
    ReadWriteWatchpoint *bp = new ReadWriteWatchpoint (arch, proc, expr, compiled_expr, addr, size, num) ;
    copy (bp) ;
    return bp ;
}

ReadWriteWatchpoint::~ReadWriteWatchpoint() {
}

Breakpoint_action ReadWriteWatchpoint::hit_active(PStream &os) {
    if (!check_scope()) {
        return BP_ACTION_STOP ;
    }
    Value newval ;
    if (compiled_expr == NULL) {
        newval = proc->read (addr,size) ;
    } else {
        EvalContext context (proc, fp, language, proc->get_os()) ;
        newval = compiled_expr->evaluate (context) ;
        if (compiled_expr->get_type()->is_real()) {
            if (compiled_expr->get_type()->get_real_size(context) == 4) {
                newval.real = (double)(*(float*)(void*)&newval.real) ;
            }
        }
    }

    if (!silent) {
        show_header (os) ;
        os.print ("%s\n", expr.c_str()) ;
        os.print ("Old value = ") ;

        /* XXX: replace me */
        JunkStream* js;
        js = reinterpret_cast<JunkStream*>(&os);
        js->print (current_value) ;

        os.print ("\nNew value = ") ;

        /* XXX: replace me */
        js = reinterpret_cast<JunkStream*>(&os);
        js->print (newval) ;

        os.print ("\n") ;
        print_location(os) ;
    }
    current_value = newval ;
    switch (disp) {
    case DISP_KEEP:
        return BP_ACTION_STOP ;
    case DISP_DELETE:
        clear() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        return BP_ACTION_STOP ;
    case DISP_DISABLE:
        disable() ;
        return BP_ACTION_STOP ;
    }
    return BP_ACTION_STOP ;
}

void ReadWriteWatchpoint::show_header (PStream &os) {
    os.print ("Hardware access (read/write)  watchpoint %d: ", get_num()) ;
}


//
// watchpoint for a read of address
//

// not supported on x86, so we use a r/w watchpoint instead

ReadWatchpoint::ReadWatchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr, Address _addr, int _size, int _num)
    : HardwareWatchpoint (_arch, _proc, _expr, cexpr, _addr, WP_RW, _size, _num) {
}

Breakpoint *ReadWatchpoint::clone() {
    ReadWatchpoint *bp = new ReadWatchpoint (arch, proc, expr, compiled_expr, addr, size, num) ;
    copy (bp) ;
    return bp ;
}

ReadWatchpoint::~ReadWatchpoint() {
}

Breakpoint_action ReadWatchpoint::hit_active(PStream &os) {
    if (!check_scope()) {
        return BP_ACTION_STOP ;
    }
    Value newval ;
    if (compiled_expr == NULL) {
        newval = proc->read (addr,size) ;
    } else {
        EvalContext context (proc, fp, language, proc->get_os()) ;
        newval = compiled_expr->evaluate (context) ;
        if (compiled_expr->get_type()->is_real()) {
            if (compiled_expr->get_type()->get_real_size(context) == 4) {
                newval.real = (double)(*(float*)(void*)&newval.real) ;
            }
        }
    }

    if (!silent) {
        show_header (os) ;
        os.print ("%s\n", expr.c_str()) ;
        os.print ("Value = ") ;

        /* XXX: replace me */
        JunkStream* js;
        js = reinterpret_cast<JunkStream*>(&os);
        js->print (newval) ;

        os.print ("\n") ;
        print_location(os) ;
    }
    switch (disp) {
    case DISP_KEEP:
        return BP_ACTION_STOP ;
    case DISP_DELETE:
        clear() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        return BP_ACTION_STOP ;
    case DISP_DISABLE:
        disable() ;
        return BP_ACTION_STOP ;
    }
    return BP_ACTION_STOP ;
}

void ReadWatchpoint::show_header (PStream &os) {
    os.print ("Hardware read watchpoint %d: ", get_num()) ;
}


//
// watchpoint for a write of address
//

WriteWatchpoint::WriteWatchpoint (Architecture *_arch, Process *_proc, std::string _expr, Node *cexpr, Address _addr, int _size, int _num)
    : HardwareWatchpoint (_arch, _proc, _expr, cexpr, _addr, WP_WRITE, _size, _num) {
}

Breakpoint *WriteWatchpoint::clone() {
    WriteWatchpoint *bp = new WriteWatchpoint (arch, proc, expr, compiled_expr, addr, size, num) ;
    copy (bp) ;
    return bp ;
}

WriteWatchpoint::~WriteWatchpoint() {
}

Breakpoint_action WriteWatchpoint::hit_active(PStream &os) {
    if (!check_scope()) {
        return BP_ACTION_STOP ;
    }
    Value newval ;
    if (compiled_expr == NULL) {
        newval = proc->read (addr,size) ;
    } else {
        EvalContext context (proc, fp, language, proc->get_os()) ;
        newval = compiled_expr->evaluate (context) ;
        if (compiled_expr->get_type()->is_real()) {
            if (compiled_expr->get_type()->get_real_size(context) == 4) {
                newval.real = (double)(*(float*)(void*)&newval.real) ;
            }
        }
    }

    if (!silent) {
        show_header (os) ;
        os.print ("%s\n", expr.c_str()) ;
        os.print ("Old value = ") ;

        /* XXX: replace me */
        JunkStream* js;
        js = reinterpret_cast<JunkStream*>(&os);
        js->print (current_value) ;

        os.print ("\nNew value = ") ;

        /* XXX: replace me */
        js = reinterpret_cast<JunkStream*>(&os);
        js->print (newval) ;

        os.print ("\n") ;
        print_location(os) ;
    }
    current_value = newval ;
    switch (disp) {
    case DISP_KEEP:
        return BP_ACTION_STOP ;
    case DISP_DELETE:
        clear() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        return BP_ACTION_STOP ;
    case DISP_DISABLE:
        disable() ;
        return BP_ACTION_STOP ;
    }
    return BP_ACTION_STOP ;
}

void WriteWatchpoint::show_header (PStream &os) {
    os.print ("Hardware write watchpoint %d: ", get_num()) ;
}


//
// hardware breakpoint
//

HardwareBreakpoint::HardwareBreakpoint (Architecture *_arch, Process *_proc, std::string _text, Address _addr, int _num)
    : HardwareWatchpoint (_arch, _proc, _text, NULL, _addr, WP_EXEC, 1, _num) {         // XXX: size is 1?
    location = proc->lookup_address (addr) ;
}

Breakpoint *HardwareBreakpoint::clone() {
    HardwareBreakpoint *bp = new HardwareBreakpoint (arch, proc, text, addr, num) ;
    copy (bp) ;
    return bp ;
}

HardwareBreakpoint::~HardwareBreakpoint() {
}


// like HardwareWatchpoint::clear() except it doesn't free the debug reg
void HardwareBreakpoint::tempremove() {
    if (!disabled && applied) {
        arch->clear_watchpoint (proc, debug_reg) ;
        applied = false ;
    }
    removed = true ;
}

void HardwareBreakpoint::temprestore() {
    set() ;
    removed = false ;
}

Breakpoint_action HardwareBreakpoint::hit_active(PStream &os) {
    proc->stop_hook() ;
    if (!silent) {
        show_header(os) ;
        print_location(os) ;
    }
    switch (disp) {
    case DISP_KEEP:
        return BP_ACTION_STOP ;
    case DISP_DELETE:
        clear() ;
        proc->remove_breakpoint(this) ;
        proc->record_breakpoint_deletion (this) ;
        return BP_ACTION_STOP ;
    case DISP_DISABLE:
        disable() ;
        return BP_ACTION_STOP ;
    }
    throw Exception("not reached");
}

void HardwareBreakpoint::show_header (PStream &os) {
    os.print ("Hardware breakpoint %d: ", get_num()) ;
}

void HardwareBreakpoint::show_location_details (PStream &os) {
    if (location.get_file() != NULL) {
        os.print ("in %s at %s:%d",
          location.get_symname().c_str(),
          location.get_filename().c_str(),
          location.get_line()) ;
    }
}
