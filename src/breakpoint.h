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

file: breakpoint.h
created on: Fri Aug 13 11:02:26 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef breakpoint_h_included
#define breakpoint_h_included

#include "dbg_types.h"
#include "pstream.h"
#include "gen_loc.h"
#include "exp_value.h"

// NOTE:  The name 'Breakpoint' is the root class of all types of what are really
// 'event points'.  This name is chosen because of the traditional use of it.  

class Architecture ;
class Process ;
class ComplexCommand ;

enum BreakpointType {
    BP_USER,            // user-set breakpoint, permanent
    BP_TEMP,            // temporary bp set by the debugger
    BP_DSO,             // Shared library activity breakpoint
    BP_DYNLINK,         // step breakpoint at 'fixup' in dynamic linker
    BP_STEP,            // step breakpoint
    BP_THR_CREATE,      // thread creation
    BP_THR_DEATH,       // thread death
    BP_WWATCH,          // watchpoint on write
    BP_HBREAK,          // hardware breakpoint
    BP_RWWATCH,         // watchpoint on read/write
    BP_RWATCH,          // watchpoint on read
    BP_CHWATCH,         // watchpoint on change
    BP_CASCADE,         // cascade breakpoint
    BP_SWWATCH          // software watchpoint
} ;

enum Breakpoint_action {
   BP_ACTION_NONE,              // no action
   BP_ACTION_CONT,              // continue
   BP_ACTION_IGNORE,            // ignore breakpoint
   BP_ACTION_STOP               // stop
} ;

enum WatchpointType {
    WP_EXEC,    // instruction execution
    WP_WRITE,   // data write
    WP_RW       // data read or write
} ;


enum Disposition {
    DISP_KEEP = 1,
    DISP_DELETE,
    DISP_DISABLE
} ;

enum CatchpointType {
    CATCH_THROW,
    CATCH_CATCH,
    CATCH_FORK,
    CATCH_VFORK,
    CATCH_EXEC,
    CATCH_SIGNAL,
    CATCH_THREAD,
    CATCH_PROCESS,
    CATCH_LOAD,
    CATCH_UNLOAD,
    CATCH_STOP
} ;

class Node;

class Breakpoint {
public:
    Breakpoint (Architecture * arch, Process * proc, std::string text, Address addr, int num) ;

    virtual ~Breakpoint() ;

    virtual void set () {} ;
    virtual void clear () {} ;
    virtual void attach () {} ;
    virtual void detach () {} ;
    virtual Address get_address () { return addr ; }
    virtual void set_address (Address a) { addr = a ; }

    std::string get_text() { return text ; }

    virtual bool is_software() { return false ; }
    // printing stuff
    virtual void print (PStream &os)  {} ;
    virtual void print_location (PStream &os) = 0 ;
    virtual void print_short_location (PStream &os)  = 0 ;
    virtual void print_details (PStream &os)  = 0 ;

    virtual Breakpoint *clone() = 0 ;

    // disable and enable
    virtual void disable () ;
    virtual void enable (bool now=true) ;
    bool is_disabled () ;
    virtual bool show_address() { return true ; }

    virtual bool is_watchpoint() { return false ; }
    void reset (Process *newproc) ;
    int get_num () { return num ; }

    Breakpoint_action hit(PStream &os) ;                 // breakpoint hit

    virtual bool is_user() { return false ; }
    virtual const char *get_type() = 0 ;
    virtual bool is_temp() { return false ; }

    virtual void tempremove() = 0 ;         // remove temporarily
    virtual void temprestore() = 0 ;        // restore after temporary removal
    bool is_removed() { return removed ; }
    bool is_applied() { return applied ; }
    virtual bool is_sw_watchpoint() { return false ; }
    virtual bool is_hw_watchpoint() { return false ; }
    virtual bool is_hw_breakpoint() { return false ; }
    bool is_pending() { return pending ; }
    void set_pending (bool v) { pending = v ; }

    virtual bool requires_backup() { return false ; }   // does this breakpoint require backing pc up
    void set_disposition (Disposition d) { disp = d ; }
    const char *get_disposition() ;

    // conditions etc
    void set_condition (std::string cond) ;
    void set_ignore_count (int n) ;
    void set_commands (std::vector<ComplexCommand *> &commands) ;

    void set_thread (int n) ;
protected:
    Address addr ;
    bool disabled ; 
    Architecture * arch ; 
    Process * proc ; 
    int num ; 
    std::string text ;

    // conditions, ignore and commands
    std::string condition_expr ;
    Node *condition ;
    int ignore_count ;
    int hit_count ;
    std::vector<ComplexCommand *> commands ;
    bool applied ; 
    bool removed ;              // has been removed temporarily
    bool silent ;               // breakpoint is silent
    bool pending ;              // breakpoint is pending
    Disposition disp ;
    virtual Breakpoint_action hit_active (PStream &os) = 0 ;
    std::vector<int> threads ;

    virtual void copy (Breakpoint *dest) ;              // copy innards to dest
} ;

class SoftwareBreakpoint : public Breakpoint {
public:
    SoftwareBreakpoint(Architecture * arch, Process * proc, std::string text, Address addr, int num) ;
    ~SoftwareBreakpoint() ;
    void set () ;
    void clear () ;
    void attach () ;
    void detach () ;
    bool is_software() { return true ; }
    void set_address (Address a) ;

    void print_location (PStream &os) ;
    void print_short_location (PStream &os) ;
    void print_details (PStream &os) ;

    void tempremove() ;         // remove temporarily
    void temprestore() ;        // restore after temporary removal
    void remove () ;

    int get_old_value() { return oldvalue ; }

    virtual const char *get_type() { return "breakpoint" ; }
 
    bool requires_backup() { return true; }
protected:
    int oldvalue ; 
    bool deleted ; 
    Location location ; 
} ;

class UserBreakpoint: public SoftwareBreakpoint {
public:
    UserBreakpoint(Architecture * arch, Process * proc, std::string text, Address addr, int num) ;
    ~UserBreakpoint() ; 
    virtual void print (PStream &os) ;
    bool is_user() { return true ; }
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
private:
} ;

class CascadeBreakpoint: public SoftwareBreakpoint {
public:
    CascadeBreakpoint(Architecture * arch, Process * proc, Address addr, int num) ;
    ~CascadeBreakpoint() ; 
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void add_child (Breakpoint *child) { children.push_back (child) ; }
protected:
private:
    std::vector<Breakpoint*> children ;
} ;

class TempBreakpoint: public SoftwareBreakpoint {
public:
    TempBreakpoint(Architecture * arch, Process * proc, Address addr, int num) ;
    ~TempBreakpoint() ; 
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
private:
} ;

// a step breakpoint is active only in the same frame.
class StepBreakpoint: public SoftwareBreakpoint {
public:
    StepBreakpoint(Architecture * arch, Process * proc, Address addr, int num) ;
    ~StepBreakpoint() ; 
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
private:
    Address fp ;
} ;

class DynamicLinkBreakpoint: public SoftwareBreakpoint {
public:
    DynamicLinkBreakpoint(Architecture * arch, Process * proc, Address a, int num) ;
    ~DynamicLinkBreakpoint() ; 
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
private:
    int phase ; 
} ;

class ThreadBreakpoint: public SoftwareBreakpoint {
public:
    ThreadBreakpoint(Architecture * arch, Process * proc, Address addr, int num, bool iscreate) ;
    ~ThreadBreakpoint() ; 
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
private:
    bool iscreate ; 
} ;

class SharedLibraryBreakpoint: public SoftwareBreakpoint {
public:
    SharedLibraryBreakpoint(Architecture * arch, Process * proc, Address addr, int num) ;
    ~SharedLibraryBreakpoint() ; 
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
private:
} ;


//
// catchpoints
//

class Catchpoint : public SoftwareBreakpoint {
public:
    Catchpoint(Architecture * arch, Process * proc, int num, CatchpointType type, std::string data) ;
    ~Catchpoint() ; 
    void set () ;
    void clear () ;
    void attach () ;
    void detach () ;
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    const char *get_type() ;
    void print (PStream &os) ;
    bool is_user() { return true ; }
    bool show_address() { return false ; }
protected:
private:
    CatchpointType type ;
    std::string data ;
    const char *info() ;
} ;

// 
// watchpoints
//

class Watchpoint : public Breakpoint {
public:
    Watchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, int size, int num) ;
    ~Watchpoint() ;
    void tempremove() {}         
    void temprestore() {}        

    void print_location (PStream &os) ;
    void print_short_location (PStream &os) ;
    void print_details (PStream &os) ;

    void set_local() { local = true ; }
    bool is_user() { return true ; }

    virtual void show_header (PStream &os) = 0 ;
    virtual bool is_watchpoint() { return true ; }
protected:
    void copy (Breakpoint *dest) ;              // copy innards to dest
    std::string expr ;
    Node *compiled_expr ;
    int size ;
    Address fp ;
    Address start_pc;
    Address end_pc;
    bool local ;
    int language ;

    bool check_scope() ;
} ;

// a software watchpoint is a watch on an address that requires software to single step and check it
// on every instruction

class SoftwareWatchpoint : public Watchpoint {
public:
    SoftwareWatchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, int size, int num) ;
    ~SoftwareWatchpoint() ;
    void set() ;
    void clear() ;
    void attach () ;
    void detach() ;
    const char *get_type() { return "watchpoint" ; }
    bool is_sw_watchpoint() { return true ; }

    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void show_header (PStream &os) ;
protected:
    Value current_value ;
} ;

// a hardware watchpoint uses the debug registers of the hardware to give us a signal when an address is
// hit

class HardwareWatchpoint : public Watchpoint {
public:
    HardwareWatchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, WatchpointType type, int size, int num) ;
    ~HardwareWatchpoint() ;
    void set() ;
    void clear() ;
    void attach () ;
    void detach() ;
    void disable() ;
    void enable (bool now=true) ;
    const char *get_type() { return "hw watchpoint" ; }

    bool is_hw_watchpoint() { return true; }

    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void show_header (PStream &os) ;
protected:
    Value current_value ;
    WatchpointType type ;
    int debug_reg ;             // debug register allocated
} ;

// a change watchpoint is used to detect change of the memory at a specific address

class ChangeWatchpoint : public HardwareWatchpoint {
public:
    ChangeWatchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, int size, int num) ;
    ~ChangeWatchpoint() ;

    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
protected:
} ;

// a read/write watchpoint is used to detect a read or write of the memory at a specific address

class ReadWriteWatchpoint : public HardwareWatchpoint {
public:
    ReadWriteWatchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, int size, int num) ;
    ~ReadWriteWatchpoint() ;

    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void show_header (PStream &os) ;
    const char *get_type() { return "acc watchpoint" ; }
private:
} ;

// a read watchpoint is used to detect a read of the memory at a specific address

class ReadWatchpoint : public HardwareWatchpoint {
public:
    ReadWatchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, int size, int num) ;
    ~ReadWatchpoint() ;

    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void show_header (PStream &os) ;
    const char *get_type() { return "read watchpoint" ; }
private:
} ;

// a write watchpoint is used to detect a write of the memory at a specific address

class WriteWatchpoint : public HardwareWatchpoint {
public:
    WriteWatchpoint (Architecture * arch, Process * proc, std::string expr, Node *cexpr, Address addr, int size, int num) ;
    ~WriteWatchpoint() ;

    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void show_header (PStream &os) ;
    const char *get_type() { return "write watchpoint" ; }
private:
} ;

// a hardware breakpoint is a watch (type = WP_EXEC) on an address containing an instruction

class HardwareBreakpoint : public HardwareWatchpoint {
public:
    HardwareBreakpoint (Architecture * arch, Process * proc, std::string text, Address addr, int num) ;
    ~HardwareBreakpoint() ;
    const char *get_type() { return "hw breakpoint" ; }

    void tempremove() ;         // remove temporarily
    void temprestore() ;        // restore after temporary removal
    Breakpoint_action hit_active (PStream &os) ;
    Breakpoint *clone() ;
    void show_header (PStream &os) ;
    bool is_hw_breakpoint() { return true ; }
    void show_location_details (PStream &os) ;
private:
    Location location ; 
} ;

#endif
