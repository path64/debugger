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

file: process.h
created on: Fri Aug 13 11:02:32 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef process_h_included
#define process_h_included

#include "dbg_types.h"
#include "breakpoint.h"
#include "thread.h"
#include "target.h"
#include "pstream.h"
#include "cli_param.h"
#include "cli.h"
#include "pcm.h"
#include "dis.h"
#include "register_set.h"

// imported classes
class ProcessController ;
class ExpressionHandler ;
class Target ;
class CommandInterpreter ;
class ComplexCommand ;

#include <thread_db.h>
#include <list>

enum State {
       IDLE,            // process has not been started
       READY,           // process is stopped awaiting our command
       RUNNING,         // process is executing
       DISABLED,        // process has stopped and cannot continue
       STEPPING,        // process is stepping and will become ready soon
       EXITED,          // process has exited
       ISTEPPING,        // stepping internally
       CSTEPPING        // continuing to step
} ;

enum CFARuleType {
        CFA_UNDEFINED, CFA_SAME_VALUE, CFA_OFFSET, CFA_REGISTER, CFA_CFA
} ;

class ObjectFile {
public:
    ObjectFile(std::string name, ELF * elf, std::istream & stream, SymbolTable * symtab) ;
    ~ObjectFile() ; 
    std::string name ; 
	// FIXME: Should not be using ELF directly here!  Move ELF-specific stuff
	// into a wrapper class so we can support Mach-o and so on.
    ELF * elf ; 
    std::istream & stream ; 
    SymbolTable * symtab ; 
    void reset() { elf = NULL ; symtab = NULL ; }
} ;

class FDE;
class Frame {
public:
	Frame(Process *proc, Architecture *arch, int n);
	Frame(Process *proc, Architecture *arch, int n, Location &l, Address pc, Address sp, Address fp);
	virtual ~Frame(); 
	void set_loc(Location &loc);
	Location & get_loc();
	FDE* get_fde();
	virtual void print(PStream &os, bool indent, bool current);
	void set_reg(int reg, Address value);
	void set_fpreg(int reg, double value);
	void set_ra(Address value);
	Address get_reg(int reg);
	double get_fpreg(int reg);
	Address get_ra() ;
	void publish_regs(Thread * thr, bool force = false) ;
	void set_valid();
	bool is_valid();
	Address get_pc();
	Address get_sp();
	Address get_fp();
	void set_pc(Address addr);
	void set_sp(Address addr);
	void set_fp(Address addr);
	void set_n (int i) { n = i; }
	RegisterSet *get_regs() { return regs; }

protected:
	void init(Process *proc, Architecture *arch, int n);

	Process *proc;
	Architecture *arch;
	int n;
	Location loc;
	Address return_addr;
	bool valid;
	RegisterSet *regs;
	RegisterSet *fp_regs;
} ;

class Rule {
public:
    Rule(CFARuleType type, int reg, int offset) ;
    ~Rule() ; 
    std::string toString () ;
    CFARuleType type ; 
    int reg ; 
    int offset ; 
} ;

class CFATable {
public:
    CFATable(Architecture *arch, Process *proc, Address loc, int ra_reg) ;
    ~CFATable() ; 
    void set_cfa (int reg, int offset) ;
    void set_cfa_reg (int reg) ;
    void set_cfa_offset (int offset) ;
    int get_cfa_reg () ;
    int get_cfa_offset () ;
    void set_reg (int reg, CFARuleType type, int offset) ;
    void advance_loc (Offset delta) ;
    void set_loc (Address l) ;
    Address get_loc () ;
    void print () ;
    void apply (Frame *from, Frame * to) ;
    int get_ra_reg() { return ra_reg ; }

    void save() ;
    void restore (int reg) ;
protected:
private:
    Architecture *arch ;
    Process *proc ;
    Address loc ; 
    int ra_reg ; 
    Rule *cfa ; 
    Rule **regs ; 
    Rule * ra ; 

    // for DW_CFA_restore
    Rule **saved_regs ;
    Rule *saved_ra ;
} ;

class LinkMap {
public:
    LinkMap(Architecture *arch, Process *proc, Address addr) ;
    ~LinkMap() ; 
    Address get_next () ;
    void print () ;
    std::string get_name () ;
    Address get_base () ;
    Address get_addr () ;
protected:
private:
    Address addr ; 
    int ps ; 
    Address base ; 
    Address nameaddr ; 
    std::string name ; 
    Address ld ; 
    Address next ; 
} ;

enum SymbolValueType {
    SV_NONE, SV_DIEVEC, SV_ADDRESS
} ;

class SymbolValue {
public:
    SymbolValue (): type(SV_NONE), addr(0) {}
    SymbolValue (DIE *die): type(SV_DIEVEC) { dievec.push_back (die);}
    SymbolValue (std::vector<DIE*> &dies): type(SV_DIEVEC), dievec(dies) {}
    SymbolValue (Address addr): type(SV_ADDRESS), addr(addr) {}
    SymbolValueType type ;
    Address addr ;
    std::vector<DIE*> dievec ;

    operator Address() {
        return addr ;
    }

    operator std::vector<DIE*> &() {
        return dievec ;
    }
} ;

// state for the 'display' command

struct Display {
    Display (Process *proc, int n, PStream &os, std::string expr, int start, Format &fmt) ;
    ~Display() ;
    int n ;
    PStream &os ;
    std::string expr ;
    Format fmt ;
    bool disabled ;
    Node *node ;                        // compiled expression node
    Location loc ;                      // location for local expressions
    bool local ;

    void execute(Process *proc) ;
    void disable() { disabled = true ; }
    void enable() { disabled = false ; }
} ;

// manager class for signals received by debuggee
const int SIGACT_STOP = 1 ;
const int SIGACT_PRINT = 2 ;
const int SIGACT_PASS = 4 ;

struct Signal {
    int num ;
    const char *name ;
    const char *description ;
    int action ;
} ;

class SignalManager {
public:
    SignalManager() ;

    int hit (int sig, PStream &os) ;
    void print (int sig, PStream &os) ;          // print signal name and description
    void show (int sig, PStream &os) ;           // show signal action
    void set (int sig, std::vector<std::string> &actions, PStream &os) ;
    int translate_signame (std::string name) ;
private:
    typedef std::map<int, Signal> SignalMap ;
    SignalMap signals ;
} ;

// a known region of memory containing code.  Used to validate code addresses
struct CodeRegion {
    CodeRegion() : start(0), end(0) {}
    CodeRegion (Section *sect, Address base) : start(sect->get_addr() + base), end (start+sect->get_size()) {}
    Address start ;
    Address end ;
} ;

class DebuggerVar;

enum {
   PSTATE_TABULA_RASA =  0x00,
   PSTATE_QUIET_STEP  =  0x01
};

class StateHolder;

class Process {
    typedef std::list<Thread*> ThreadList ;
    typedef std::list<Breakpoint*> BreakpointList ;
    typedef std::map<Address, BreakpointList*> BreakpointMap ;
    typedef std::vector<Frame*> FrameVec ;
    typedef std::vector<ObjectFile *> ObjectFileVec ;
    typedef std::vector<LinkMap *> LinkMapVec ;
    typedef std::list<Display*> DisplayList ;

    friend class LinkMap ;
public :
    /* XXX: this constructor has ridiculous number of arguments */
    Process(ProcessController * pcm, std::string program,
      Architecture * arch, Target *target, PStream &os, AttachType at) ;

    Process (const Process &old) ;                    // copy the details from another process
    ~Process() ; 

    // attachment methods.  These allow attachment to a core file or a live process
    void attach_core() ;                        // attach to a core file
    void attach_process(int pid) ;                     // attach to a live process
    void attach_child(int pid, State parent_state) ;                     // attach to a child process

    Architecture *get_arch() { return arch ; }
    void reset() ;
    int get_pid() { return pid ; }
    std::string get_program() { return program ; }
    const char *get_state() ;

    PStream &get_os() { return os ; }
    void set_target (Target *t) { target = t ; }
    bool is_running() ;                 // is the process running
    bool is_active() ;                 // is the process running or a core file loaded
    void new_thread (void * id) ;
    void open_object_file (std::string name, Address baseaddress, bool reporterror=false) ;

    // expression evaluation and printing
    void print_expression (std::string expr, Format &fmt, bool terse, bool record) ;
    void display_expression (int n, std::string expr, Node *tree, Format &fmt) ;
    Address evaluate_expression (std::string expr, int &end, bool needint=false) ;
    Value evaluate_expression (Node *expr, bool addressonly=false) ;
    Node *compile_expression (std::string expr, int &end, bool single = false) ;
    void print_function_paras (Frame *frame, DIE *func) ;
    void examine (const Format &fmt, Address addr) ;            // memory dump
    void print_type (std::string expr, bool show_contents = true) ;                        // print the type of the expression
    void complete_symbol (std::string name, std::vector<std::string> &result) ;
    void complete_function (std::string name, std::vector<std::string> &result) ;

    void set_value (Value&loc, Value &v, int size) ;
    void print_loc(const Location& loc, Frame *frame, PStream &os, bool showthread=false);

    void info (std::string root, std::string tail) ;

    /* routines to access global parameters */
    int get_int_opt(CliParamId id);
    const char* get_str_opt(CliParamId id);

    // register access
    Address get_reg (std::string name) ;
    Address get_fpreg (std::string name) ;
    Address get_reg (int num) ;
    void set_reg (std::string name, Address value) ;
    void set_fpreg (std::string name, Address value) ;
    void set_reg (int num, Address value) ;
    Address get_debug_reg (int reg) ;
    void set_debug_reg (int reg, Address value) ;
    StateHolder* save_and_reset_state();
    void restore_state(StateHolder *state);


    // breakpoint control
    void add_breakpoint (Breakpoint * bp, bool update=false) ;
    void print_bps() ;
    BreakpointList * find_breakpoint (Address addr) ;
    Breakpoint *find_breakpoint (int bpnum) ;
    void remove_breakpoint (Breakpoint * bp) ;
    void delete_breakpoint (int num) ;
    // breakpoint conditions etc
    void set_breakpoint_condition (int bpnum, std::string cond) ;
    void set_breakpoint_ignore_count (int bpnum, int n) ;
    void set_breakpoint_commands (int bpnum, std::vector<ComplexCommand *>& cmds) ;
    void list_breakpoints (bool all) ;
    void enable_breakpoint (int n) ;
    void set_breakpoint_disposition (int n, Disposition disp) ;
    void disable_breakpoint (int n) ;   
    void attach_breakpoints (int pid) ;         // attach breakpoints to a given pid
    void detach_breakpoints (int pid) ;         // detach breakpoints from a given pid
    void detach_breakpoints () ;                // detach all breakpoints 
    void tempremove_breakpoints (Address addr) ;
    void temprestore_breakpoints (Address addr) ;
    int breakpoint_count() ;
    void reset_bp_num() { bpnum = 1; }

    void clear_breakpoints (Address addr) ;
    void stop_hook() ;
    bool sw_watchpoints_active() ;             // are any software watchpoints active

    // symbol lookup
    void enumerate_functions (std::string name, std::vector<std::string> &results) ;
    SymbolValue find_symbol (std::string name, Address reqpc=0) ;
    DIE *find_scope (std::string name, bool topdown) ;          // find a scope

    DIE *find_compilation_unit (std::string filename) ;
    FDE *find_fde (Address addr) ;
    Address lookup_symbol (std::string name, std::string objectfile="") ;
    Address lookup_function (std::string name, std::string filename="", bool skip_preamble=true) ;
    Location lookup_address (Address addr, bool guess=true) ;
    Address lookup_line (std::string filename, int lineno) ;
    Address lookup_line (int lineno) ;
    Location get_current_location() ;

    // stack trace control
    void up (int n) ;
    void down (int n) ;
    void dump (Address addr, int size) ;
    Frame *get_current_frame() ;


    LinkMap * get_new_link_map () ;
    Address get_r_debug_state () ;

    /* read and write just passes control to target */
    void write (Address addr, long data, int size=4) {
        target->write((*current_thread)->get_pid(), addr, data, size);
    }
    void write_string (Address addr, std::string s) {
        target->write_string((*current_thread)->get_pid(), addr, s);
    } 
    /* XXX: have target hold pid so we can drop get_pid crap */
    /* XXX: write a un-breakpointsize memory function so reads can also go here */


    // debug memory access
    std::string read_string (Address addr) ;
    std::string read_string (Address addr, int len) ;

    void spawn_cli (Address endsp) ;
    // basic control
    bool run (const std::string& args, EnvMap& env) ;
    void interrupt() ;
    void list_symbols () ;
    void list_threads () ;
    void switch_thread (int n) ;
    bool stepping_stops(Address pc);
    bool test_address (Address addr) ;
    Address read (Address addr, int size);
    Address raw_read (Address addr, int size) ;
    Address readptr (Address addr) ;
    Address readelfxword (ELF * elf, Address addr) ;
    bool docont () ;
    bool cont (int sig=0) ;
    void single_step () ;
    void step (bool by_line, bool over, int n) ;
    void until() ;
    void until(Address addr) ;
    bool jump(Address addr) ;
    Address get_return_addr () ;
    void resume_stepping () ;
    bool wait (int status = -1) ;
    int dowait(int &status) ;                      // wait for a thread or process to terminate with no hang
    bool is_child_pid(int pid) ;
    void get_regs(RegisterSet *regs, void *tid) ;
    void set_regs(RegisterSet *regs, void *tid) ;
    void get_fpregs(RegisterSet *regs, void *tid) ;
    void set_fpregs(RegisterSet *regs, void *tid) ;
    Breakpoint * new_breakpoint (BreakpointType type, std::string text, Address addr, bool pending=false) ;
    Watchpoint *new_watchpoint (BreakpointType type, std::string expr, Node *node, Address addr, int size, bool pending=false) ;
    Catchpoint * new_catchpoint (CatchpointType type, std::string data) ;
    void stacktrace (int n) ;
    bool stack_contains(Address fp, Address start_pc, Address end_pc);
    void set_frame (int frameno) ;
    void show_frame () ;                // show the current frame
    void print_regs (bool all) ;
    void print_reg (std::string name) ;
    void record_breakpoint_deletion (Breakpoint *bp) { if (hitbp == bp) { hitbp = NULL ; } }
    void resolve_pending_breakpoints() ;
    void disassemble (Address addr, bool newline=true) ;
    void disassemble (Address start, Address end, bool newline=true) ;
    void disassemble (Address start, int ninsts, bool newline=true) ;
    void return_from_func(Address value) ;              // return from function with value
    void finish() ;                                     // finish execution of current function
    void wait_for_child(pid_t childpid);
    void follow_fork (pid_t childpid, bool is_vfork) ;
    void execute_displays() ;
    void validate_thread (int n) ;
    int get_current_thread() ;

    // source listing
    void list () ;              // list from last line
    void list_back () ;          // list previous lines
    void list (std::string filename, int line) ;      // list from this line
    void list (std::string filename, int sline, int eline) ;      // list from this line to that line
    void list (Address addr, Address endaddr=0) ;              // list this address
    void search (std::string regex) ;           // search for a string

    // 'display' command handling
    int set_display (std::string expr, int start, Format &fmt) ;
    void undisplay (int n) ;
    void enable_display (int n) ;
    void disable_display (int n) ;
    void list_displays() ;

    CommandInterpreter *get_cli() ;
    AttachType get_attach_type() { return attach_type ; }

    void set_signal_actions (std::string name, std::vector<std::string> &actions) ;

    void get_current_subprogram (DIE *&die, int &language) ;
    void print_variable_set (DIE *die, int language, std::vector<DIE*> &vars) ;

    int get_main_language() ;

    void set_current_line (int n) { last_listed_line = n ; }
    std::string realname (std::string nm) ;

    void build_local_map (DIE *die, LocalMap &map) ;              // build map of local vars for disassembler

    Section * lookup_symbol_section (std::string name) ;
    Section * find_section_at_addr (Address addr) ;
    int get_language(bool &isauto) ;
    int get_language() { bool a; return get_language(a); }
    bool is_case_blind() ;
    bool is_multithreaded() { return multithreaded; }
    void show_current_thread() ;
protected:
private:
    std::list<std::istream*> open_streams;

    bool handle_signal(bool&);

    void print_vector (EvalContext &ctx, Value &v, DIE *type) ;
    void print_vector_type (EvalContext &ctx, Value &v, DIE *type) ;
    void list (File *file, int sline, int eline, int currentline)  ;
    void load_dynamic_info (bool set_break) ;
    void sync_threads () ;
    void apply_breakpoints () ;
    void sync () ;
    void invalidate_frame_cache () ;
    void build_frame_cache () ;
    void execute_fde (FDE * fde, Address pc, Frame * from, Frame *to, bool debug) ;
    Address get_fde_return_address (FDE * fde, Address pc, Frame * frame) ;
    void execute_cfa (Architecture *arch, CFATable *table, BVector code, Address pc,
        int caf, int daf, int ra, bool debug);
    LinkMap * find_link_map (Address addr) ;
    void load_link_map (Address addr) ;
    void step_from_breakpoint (Breakpoint * bp) ; // single step one instruction from breakpoint
    bool step_one_instruction () ;                // single step one instruction
    ProcessController * pcm ; 
    std::string program ; 
public:
    Architecture * arch ;               // needed for bug 2671
    td_thragent_t *thread_agent ; 

    int is_64bit() { return objectfiles[0]->elf->is_elf64(); }
    void push_location() { 
       if (!is_running()) return;
       last_loc = lookup_address(get_reg("pc"));
    }

private:
    Target *target ;
    int pid ; 
    State state ; 
    int signalnum ; 

    int proper_state;   // The original state mechanism is extremely bad
                        // it needs to be thrown away and rewritten.  All
                        // new states should be added as bit flags until
                        // that time.


    ThreadList threads ; 
    ThreadList::iterator current_thread ; 
    bool multithreaded ; 
    BreakpointList breakpoints ; // list of breakpoints
    BreakpointList sw_watchpoints ;     // software watchpoints (subset of breakpoints)
    BreakpointMap bpmap ; // map of address vs list of bps
    int bpnum ; 
    int ibpnum ;                // internal breakpoint numbers
    Breakpoint * hitbp ; // the breakpoint that we hit
    bool stepped_onto_breakpoint ;
    Address r_debug ; // address of struct r_debug for debuggee
    bool stepping_lines ; 
    bool stepping_over ; 
    Address main ; // address of main
    FrameVec frame_cache ; // vector of locations when stopped
    bool frame_cache_valid ; 
    bool frame_corrupted;
    int current_frame ; 
    Address plt_start ; // start of the PLT
    Address plt_end ; // end of the PLT
    Address fixup_addr ; // address of 'fixup'
    Address dyn_start ; // start of dynamic linker .text section
    Address dyn_end ; // end of dynamic linker .text section
    ObjectFileVec objectfiles ; // vector of LoadedFile
    LinkMapVec linkmaps ; 
    PStream &os ;                        // output stream
    int current_signal ;                // current signal to send to process

    DisplayList displays ;
    int displaynum ;

    AttachType attach_type ;            // how we are attached

    int last_listed_line ;
    std::string last_search_string ;
    CodeRegion *lastregion ;

    Location last_loc;             // location at last prompt

    bool thread_db_initialized ;        // thread_db has been initialized already
    Address creation_bp ;
    Address death_bp ;

    int mt_wait() ;                     // multithreaded wait

    // code regions for verifying code addresses
    std::vector<CodeRegion> code_regions ;
    bool is_valid_code_address (Address addr) ;         // is the code address valid
    bool check_code_address (Address addr) ;

    Location current_location ;

    SignalManager signal_manager ;
    bool init_phase ;

public:
    DebuggerVar *find_debugger_variable (std::string n) ;
    DebuggerVar *add_debugger_variable (std::string name, Value &val, DIE *type) ;
private:
    void add_debugger_variable (int n, Value &val, DIE *type) ;

// threads
    void show_thread_states() ;
    void resume_threads() ;
    void kill_threads() ;
    void detach_threads() ;
    void stop_threads() ;
    void disable_threads() ;
    void enable_threads() ;
    ThreadList::iterator find_thread (int pid) ;
    void find_bp_threads (std::vector<ThreadList::iterator> &result, std::vector<ThreadList::iterator> &userbps) ;
    void grope_threads() ;
    void reap_threads() ;

    std::vector<DIE*> expression_dies ;         // DIEs created by expressions that are kept (displays etc)

    time_t programtime;  // modification time of executible
} ;

#endif
