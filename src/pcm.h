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

file: pcm.h
created on: Fri Aug 13 11:02:31 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef pcm_h_included
#define pcm_h_included

#include "dbg_types.h"
#include "target.h"
#include "pstream.h"
#include "breakpoint.h"
#include "symtab.h"

class Process ;
class Architecture ;
class Breakpoint ;
class Format ;
class Target ;
class CommandInterpreter ;
//class AliasManager ;
class ProcessController ;
class ComplexCommand ;

enum AttachType {
    ATTACH_NONE,                // don't attach to anything
    ATTACH_CORE,                // attach to a core file
    ATTACH_PROCESS              // attach to a live process
} ;

class ProcessController {
public:
    ProcessController(CommandInterpreter *cli, DirectoryTable &dirlist, bool subverbose) ;
    ~ProcessController() ;

    void attach (std::string filename, bool replace) ;
    void attach (int pid, bool replace) ;
    void attach_core (std::string corefile, bool replace) ;
    void attach_core (std::string filename, std::string corefile, bool replace) ;
    void detach() ;
    void detach_all() ;

    int get_current_thread();

    Address get_frame_pc (int pid, int tid, int fid);
    int get_frame();
    int get_frame_size();
    void print_function_paras (int fid, DIE *die);

    bool in_sigtramp (std::string name);

    void get_license() ;

    bool file_ok() { return file_present ; }

    // process list control
    int add_process (Process *proc) ;
    void remove_process (Process *proc) ;
    void select_process (int i) ;
    void list_processes () ;
    int get_current_process() ;

    void set_signal_actions (std::string name, std::vector<std::string> &actions) ;

    bool run (const std::string& args, EnvMap& env);
    bool cont (int sig = 0) ;
    void single_step () ;
    void wait () ;
    void interrupt() ;
    void ready_wait() ;
    void until() ;
    void until (Address addr) ;
    void kill() ;
    bool jump (Address addr) ;

    bool test_address (Address addr) ;
    Address get_return_addr() ;
    Location lookup_address (Address addr) ;
    void push_location();

    int breakpoint_count() ;
    void reset_bp_num();
    Breakpoint * new_breakpoint (BreakpointType type, std::string text, Address addr, bool pending) ;
    Watchpoint * new_watchpoint (BreakpointType type, std::string expr, Node *node, Address addr, int size, bool pending) ;
    Catchpoint * new_catchpoint (CatchpointType type, std::string data) ;
    Address lookup_symbol (std::string name, std::string objectfile="") ;
    Address lookup_function (std::string name, std::string filename="", bool skip_preamble=true) ;
    void enumerate_functions (std::string name, std::vector<std::string> &results) ;
    std::string realname (std::string mangled_name) ;
    void list_breakpoints () ;
    void delete_breakpoint (int n) ;
    void enable_breakpoint (int n) ;
    void disable_breakpoint (int n) ;
    void set_breakpoint_disposition (int n, Disposition dis) ;
    void list_symbols () ;
    void list_threads () ;
    void switch_thread (int n) ;
    void dump (Address addr, int size) ;
    void stacktrace (int n) ;
    void up (int n) ;
    void down (int n) ;
    void print_regs () ;
    Location get_current_location() ;
    void complete_symbol (std::string name, std::vector<std::string> &result) ;
    void complete_function (std::string name, std::vector<std::string> &result) ;
    void info(std::string root, std::string tail) ;
    void print_expression (std::string expr, Format &fmt, bool terse, bool record) ;
    void print_type (std::string expr, bool show_contents = true) ;
    Address evaluate_expression (std::string expr, int &end, bool needint=false) ;
    Value evaluate_expression (Node *tree, bool addressonly=false) ;
    Node *compile_expression (std::string expr, int &end, bool single = false) ;
    void step (bool by_line, bool over, int n) ;
    void disassemble (Address addr) ;
    void disassemble (Address start, Address end) ;
    void set_frame (int n) ;
    void show_frame() ;
    int set_display (std::string expr, int start, Format &fmt) ;
    void enable_display (int n) ;
    void disable_display (int n) ;
    void undisplay (int n) ;
    void list_displays() ;
    Address lookup_line (std::string filename, int lineno) ;
    Address lookup_line (int lineno) ;
    void return_from_func(Address value) ;              // return from function with value
    void finish() ;                                     // finish execution of current function
    void examine (const Format &fmt, Address addr) ;            // memory dump

    int get_main_language() ;

    // listing
    void list () ;              // list from last line
    void list (std::string filename, int line) ;      // list from this line
    void list (std::string filename, int sline, int eline) ;      // list from this line to that line
    void list (Address addr, Address endaddr= 0) ;              // list this address
    void list_back() ;          // the previous lines

    void search (std::string text) ;

    Architecture *get_arch() { return arch ; }
    // breakpoint conditions etc
    void set_breakpoint_condition (int bpnum, std::string cond) ;
    void set_breakpoint_ignore_count (int bpnum, int n) ;
    void set_breakpoint_commands (int bpnum, std::vector<ComplexCommand *>& cmds) ;
    void clear_breakpoints (Address addr) ;

    void set_cli (CommandInterpreter *cli) { this->cli = cli ; }
    CommandInterpreter *get_cli() { return cli ; }
    bool is_running() ;
    AliasManager *get_aliases() { return &aliases ; }
    DirectoryTable &get_dirlist() { return dirlist ; }
protected:
private:
    std::string program ;
    Architecture * arch ;
    Target *target ;
    PStream &os ;
    std::vector<Process *> processes ;          // all processes

    bool file_present ;
    Process *current_process ;
    CommandInterpreter *cli ;
    AliasManager aliases ;
    DirectoryTable &dirlist ;
    bool subverbose ;
} ;

#endif
