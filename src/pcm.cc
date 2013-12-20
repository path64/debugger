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

file: pcm.cc
created on: Fri Aug 13 11:07:43 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

//FIXME: Build system bug work-around - remove when the build system is fixed.

#define NO_LICENSE

#ifndef PSC_BUILD_DATE
#define PSC_BUILD_DATE ""
#endif

#include "pcm.h"

#include "version_bk.h"
#include "symtab.h"
#include "process.h"
#include "arch.h"
#include "cli.h"
#include "target.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysctl.h>

static bool file_exists (std::string filename) {
    struct stat st ;
    int e = stat (filename.c_str(), &st) ;
    if (e == 0) {
        if (!S_ISREG(st.st_mode)) {
            return false ;
        }
        return true ;           // XXX: symbolic links?
    } else {
       return false ;
    }
}

static std::string find_program (std::string program) {
    uint ch = 0 ;
    while (ch < program.size() && isspace (program[ch])) ch++ ;
    if (ch == program.size()) {
        throw Exception ("Unable to find program %s", program.c_str()) ;
    }
    if (file_exists (program)) {
        if (program[0] != '/') {
            char *cwd = getcwd (NULL, 0) ;
            return std::string(cwd) + '/' + program ;                        // look for plain name first
        } else {
            return program ;
        }
    }
    // if the program was a rooted file and we failed to find it above then it 
    // doesn't exist
    if (program[ch] == '/') {
        throw Exception ("Unable to find program %s", program.c_str()) ;
    }
    char *path = getenv ("PATH") ;
    char *s = path ;
    while (*s != 0) {
        char *e = s ;
        std::string p ;
        while (*e != 0 && *e != ':') {          // skip to end of path segment
            e++ ;
        }
        // copy path segment to temporary
        while (s != e) {
            p += *s++ ;
        }
        // make full path
        std::string pathname = p + "/" + program ;
        if (file_exists (pathname)) {
            return pathname ;
        }
        if (*e == 0) {                  // end of path?
            break ;
        }
        s = e + 1 ;
    }
    throw Exception ("Unable to find program %s", program.c_str()) ;
}


// static Architecture *new_arch (std::string filename) {
// 	// FIXME: This is wrong - you should be able to debug a 64-bit core on a
// 	// 32-bit machine, and you should be able to do remote debugging of a
// 	// 64-bit process from a 32-bit one.  We should also check the e_machine
// 	// flag in the header and see if it corresponds to the current architecture.
//     Architecture *arch ;
//     if (is_elf64 (filename.c_str())) {
//          if (sizeof(char*) == 4) {
//              throw Exception ("Cannot debug a 64-bit executable using a 32-bit debugger") ;
//          }
//          arch = new x86_64Arch(64) ;
//     } else {
//          if (sizeof(char*) == 8) {              // 64 bit debugger
//              arch = new x86_64Arch(32) ;                // 32 bit x86_64
//          } else {
//              arch = new i386Arch() ;
//          }
//     }
//     return arch ;
//
// }

ProcessController::ProcessController (CommandInterpreter *_cli, bool _subverbose)
    : arch(NULL),
      file_present(false),
      current_process(NULL),
      cli(_cli),
      os (_cli->os),
      target(NULL),
      dirlist(_cli->dirlist), subverbose(_subverbose) {
    // create dummy process
    Process *proc = new Process (this, "", NULL, NULL, os, ATTACH_NONE) ;
    current_process = proc ;
    add_process (proc) ;
}

void ProcessController::get_license() {
#ifdef TIMEBOMB
     time_t now = time(NULL) ;
     if (now > TIMEBOMB) {
        fprintf (stderr, "This version of the debugger has expired.  Please contact support@pathscale.com for an upgrade.\n") ;
        exit (1) ;
     }
#endif

#ifdef NO_LICENSE
    return ;
#endif
    std::string exename ;
    char *client = getenv ("PATHSCALE_SUBSCRIPTION_CLIENT") ;
    if (client == NULL) {
        char link[256] ;
        int e, end=0 ;
#if defined (__linux__)
        e = readlink ("/proc/self/exe", link, sizeof(link)-1) ;
#elif defined (__FreeBSD__)
        int exe_path_mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, getpid() } ;
        size_t exe_path_size = sizeof (link) ;
        e = sysctl (exe_path_mib, 4, link, &exe_path_size, NULL, 0) ;

        if (e != -1)
            e = strlen (link) ;
#endif
        if (e == -1) {
            fprintf (stderr, "Unable to determine directory for licensing, terminating\n") ;
            exit(1) ;
        }
        if (e > 1) {
            // remove the final executable name from the path
            // first iteration converts length to last index
            do {
                e-- ;
            } while (e != 0 && link[e] != '/');
            end = e-- ;
        }
        link[end] = 0 ;
        exename = std::string(link) + "/subclient" ;
    } else {
        exename = client ;
    }

    if (subverbose) {
        printf ("looking for %s\n", exename.c_str()) ;
    }
    if (!file_exists(exename)) {
        fprintf (stderr, "Unable to obtain license - could not locate the license client program\n") ;
        exit (1) ;
    }

    int language = get_main_language() ;
    if (language == DW_LANG_C) {                        // default for not finding the language
        int MAIN__ = lookup_symbol ("MAIN__", "") ;         // only fortran programs will have this
        if (MAIN__ != 0) {
            language = DW_LANG_Fortran90 ;
        }
    }
    const char *lang = "" ;
    switch (language) {
    case DW_LANG_C:
    case DW_LANG_C89:
        lang = "C" ;
        break ;
    case DW_LANG_C_plus_plus:
        lang = "CC" ;
        break ;
    case DW_LANG_Fortran77:
        lang = "FORTRAN77" ;
        break ;
    case DW_LANG_Fortran90:
        lang = "FORTRAN90" ;
        break ;
    case DW_LANG_Fortran95:
        lang = "FORTRAN95" ;
        break ;
    }
    if (subverbose) {
        printf ("Attempting to get license for language: %s\n", lang) ;
    }
    int pid = fork() ;
    if (pid == 0) {
        const char *argvec[8] ;
                                                                                                                                                      
        argvec[0] = exename.c_str() ;
        argvec[1] = "Compiler" ;
        argvec[2] = lang ;
        argvec[3] = PSC_BUILD_DATE ;
        argvec[4] = "" ;
        argvec[5] = "pathdb" ;
        argvec[6] = NULL ;
        if (subverbose) {
            argvec[6] = "--v" ;
            argvec[7] = NULL ;
        }
        execv (exename.c_str(), (char*const*)argvec) ;
        fprintf (stderr, "Unable to obtain license - could not execute the client program\n") ;
        exit(1) ;
    } else {
        int statloc ;
        waitpid (pid, &statloc, 0) ;
                                                                                                                                                      
        // if we were not able to get a license due to missing subclient executable, tell caller
        if (WIFEXITED(statloc)) {
            if (WEXITSTATUS (statloc) == 6) {           // no subclient program?
                exit (1) ;
            } else if (WEXITSTATUS (statloc) == 7) {    // hard stop?
                fprintf (stderr, "Debugger terminated\n") ;
                exit (1) ;
            } else if (WEXITSTATUS (statloc) != 0) {            // license client failed, can't rely on output
                fprintf (stderr, "Subscription client exited with error status\n") ;
                exit (1) ;
            }
        }

    }
}

// debug a new file
void ProcessController::attach (std::string filename, bool replace) {
    if (filename == "") {
         bool ok = cli->confirm ("No executable file now.", "Discard symbol table") ;
         if (ok) {
            // create dummy process   XXX: 'replace' parameter?
            Process *proc = new Process (this, "", NULL, NULL, os, ATTACH_NONE) ;
            remove_process (current_process);
            delete current_process ;
            add_process (proc) ;
            current_process = proc;
            program = "" ;
            printf ("No symbol file now.\n") ;
            return ;
         }
    }

    program = find_program (filename) ;
    ELF * elf = new ELF (program) ;
    std::istream *s = elf->open() ;
    arch = elf->new_arch () ;
    delete elf;
    delete s;
    target = Target::new_live_target (arch) ;

    if (replace) {
        Process *oldp = current_process ;
        remove_process (oldp) ;
        delete oldp ;
    }
    

    Process *proc = new Process (this, program, arch, target, os, ATTACH_NONE) ;
    add_process (proc) ;
    current_process = proc;
    file_present = true ;
    get_license() ;
}

// attach a new process to a process
void ProcessController::attach (int pid, bool replace) {
    if (program != "") {
        printf ("Attaching to program %s, process %d.\n", program.c_str(), pid) ;
    } else {
        printf ("Attaching to process %d.\n", pid) ;
    }
    if (pid == 0) {                     // 0 is meaningful to kill, but it doesn't exist
        printf ("attach: No such process\n") ;
        return ;
    }
    int e = ::kill (pid, 0) ;
    if (e != 0) {
        perror ("attach") ;
        return ;
    }
    if (program == "") {
        char procbuf[256] ;
        char link[256] ;
        snprintf (procbuf, sizeof(procbuf), "/proc/%d/exe", pid) ;
        e = readlink (procbuf, link, sizeof(link)-1) ;
        if (e == -1) {
            perror ("readlink") ;
            return ;
        }
        link[e] = 0 ;  // append NUL char (readlink doesn't)
        printf ("program is %s\n", link) ;
        program = link ;
    }

    char pidbuf[100] ;
    sprintf (pidbuf, "%d", pid) ;
    ELF * elf = new ELF (program) ;
    std::istream *s = elf->open() ;
    arch = elf->new_arch () ;
    delete elf;
    delete s;
    target = Target::new_live_target (arch) ;

    if (replace) {
        Process *oldp = current_process ;
        remove_process (oldp) ;
        delete oldp ;
    }
    Process *proc = new Process (this, program, arch, target, os, ATTACH_PROCESS) ;
    add_process (proc) ;
    current_process = proc;

    proc->attach_process (pid) ;
    file_present = true ;
    get_license() ;
}

// attach a new process to a core file
void ProcessController::attach_core (std::string corefile, bool replace) {
    ELF * elf = new ELF (program) ;
    std::istream *s = elf->open() ;
    arch = elf->new_arch () ;
    delete elf;
    delete s;

    target = new CoreTarget (arch, corefile) ;

    if (replace) {
        Process *oldp = current_process ;
        remove_process (oldp) ;
        delete oldp ;
    }
    Process *proc = new Process (this, program, arch, target, os, ATTACH_CORE) ;
    add_process (proc) ;
    current_process = proc;

    proc->attach_core() ;
    file_present = true ;
    get_license() ;
}

// attach a new process to a core file
void ProcessController::attach_core (std::string filename, std::string corefile, bool replace) {
    if (!file_exists (corefile)) {
        throw Exception ("Unable to open core file %s", corefile.c_str()) ;
    }
    program = find_program (filename) ;
    struct stat corest ;
    struct stat progst ;
    if (stat (program.c_str(), &progst) == 0) {
        if (stat (corefile.c_str(), &corest) == 0) {
            if (corest.st_mtime < progst.st_mtime) {
                printf ("Warning: executable %s has been modified more recently than core file %s\n", program.c_str(), corefile.c_str()) ;
            }
        }
    }

    ELF * elf = new ELF (program) ;
    std::istream *s = elf->open() ;
    arch = elf->new_arch () ;
    delete elf;
    delete s;

    target = new CoreTarget (arch, corefile) ;

    if (replace) {
        Process *oldp = current_process ;
        remove_process (oldp) ;
        delete oldp ;
    } 
    Process *proc = new Process (this, program, arch, target, os, ATTACH_CORE) ;
    add_process (proc) ;
    current_process = proc;

    proc->attach_core() ;
    file_present = true ;
    get_license() ;
}

void ProcessController::detach() {
    if (program != "") {
        printf ("Detaching from program: %s\n", program.c_str()) ;
        // create dummy process
        Process *proc = new Process (this, "", NULL, NULL, os, ATTACH_NONE) ;
        remove_process (current_process);
        delete current_process ;
        add_process (proc) ;
        current_process = proc;
    }
}

void ProcessController::detach_all() {
    // blow away all old processes
    std::vector<Process*>::iterator i ; 
    for (i = processes.begin(); i != processes.end(); i++) {
        //if ((*i)->is_running()) {
           delete *i ;
        //}
    }
    processes.clear();

    // setup nil process as current
    Process *proc = new Process (this, "", NULL, NULL, os, ATTACH_NONE) ;
    add_process(proc);
    current_process = proc;
}

ProcessController::~ProcessController() {
}

int ProcessController::add_process (Process *proc) {
    processes.push_back (proc) ;
    return processes.size() - 1 ;
}

void ProcessController::remove_process (Process *proc) {
#if 0
    int index = -1 ;
    for (unsigned int i = 0 ; i < processes.size() ; i++) {
        if (processes[i] == proc) {
            index = (int)i ;
            break ;
        }
    }
    if (index == -1) {
        throw Exception ("No such process") ;
    }
    // shift all processes above proc down one slot
    for (uint i = index ; i < processes.size() - 1 ; i++) {
        processes[index] = processes[index+1] ;
    }
    processes.resize (processes.size() - 1) ;
#endif
	std::vector<Process*>::iterator i ;

	for (i = processes.begin(); i != processes.end(); i++) {
		if (*i == proc) {
			processes.erase (i);
			return;
		}
	}

	throw Exception ("No such process") ;
}

void ProcessController::select_process (int i) {
    if (i < 0 || i >= (int) processes.size()) {
        throw Exception ("No such process") ;
    }
    current_process = processes[i];
}

#if 0
void ProcessController::list_processes () {
    os.print ("Index    PID      State      Command\n") ;
    for (uint i = 0 ; i < processes.size() ; i++) {
        os.print ("%-8d %-8d %-10s %s\n", i, processes[i]->get_pid(), processes[i]->get_state(), processes[i]->get_program().c_str()) ;
    }
}
#endif

int ProcessController::get_current_process() {
    for (uint i = 0 ; i < processes.size() ; i++) {
        if (processes[i] == current_process) {
            return i ;
        }
    }
    throw Exception ("Get id of current process error") ;
}

                                                                                                                          

bool ProcessController::is_running(int n) {
    return processes[n]->is_running() ;
}

void ProcessController::run(const std::string& args, EnvMap& env) {
	push_location();

    Process *proc = current_process ;
    
    AttachType at = proc->get_attach_type() ;
    switch (at) {
    case ATTACH_NONE:                 
    case ATTACH_PROCESS:
        if (proc->is_running()) {
            bool yes = cli->confirm("The program being debugged has been started already",
                                    "Start it from the beginning") ;
            if (yes) {
                Process *newproc = new Process (*proc) ;   // copy the process data
                remove_process (proc);
		current_process = newproc ;           // overwrite the old process
		add_process (newproc) ;
                delete proc ;                                    // delete the old process
            } else {
                throw Exception ("Program not restarted.") ;
            }
        } else {
            current_process->reset() ;
        }
        break ;

    case ATTACH_CORE:
        // going to run from a core state.  We need to create a new LiveTarget and delete the
        // old CoreTarget
        //delete target ;                       // XXX: seems to cause a problem
        target = Target::new_live_target (arch) ;
        Process *newproc = new Process (*proc) ;   // copy the process data
        newproc->set_target (target) ;
        current_process = newproc ;           // overwrite the old process
        delete proc ;                                    // delete the old process
        break ;
    }

	if (current_process->run (args, env))
		ready_wait() ;
}

void ProcessController::cont(int sig) {
	push_location ();

    if (current_process->cont(sig))
	ready_wait() ;
}

void ProcessController::single_step() {
	push_location();

    current_process->single_step() ;
}

void ProcessController::kill() {
    Process *proc = current_process ;
    
    AttachType at = proc->get_attach_type() ;
    switch (at) {
    case ATTACH_NONE:                 
    case ATTACH_PROCESS:
        if (proc->is_running()) {
            bool yes = cli->confirm(NULL, "Kill the program being debugged") ;
            if (yes) {
                Process *newproc = new Process (*proc) ;   // copy the process data
                remove_process (proc);
		current_process = newproc ;           // overwrite the old process
		add_process (newproc) ;
                delete proc ;                                    // delete the old process
            } else {
                os.print ("Program not killed.\n") ;
                return ;
            }
        } else {
            os.print ("The program is not being run.\n") ;
        }
        break ;

    case ATTACH_CORE:
        // going to run from a core state.  We need to create a new LiveTarget and delete the
        // old CoreTarget
        //delete target ;                       // XXX: seems to cause a problem
        target = Target::new_live_target (arch) ;
        Process *newproc = new Process (*proc) ;   // copy the process data
        newproc->set_target (target) ;
        current_process = newproc ;           // overwrite the old process
        delete proc ;                                    // delete the old process
        break ;
    }
}

void ProcessController::wait() {
    current_process->wait() ;
}

void ProcessController::interrupt() {
    current_process->interrupt() ;
}

Breakpoint * ProcessController::new_breakpoint(BreakpointType type, std::string text, Address addr, bool pending) {
    return current_process->new_breakpoint (type, text, addr, pending) ;
}

Watchpoint *ProcessController::new_watchpoint (BreakpointType type, std::string expr, Node *node, Address addr, int size, bool pending) {
    return current_process->new_watchpoint (type, expr, node, addr, size, pending) ;
}

Catchpoint *ProcessController::new_catchpoint (CatchpointType type, std::string data) {
    return current_process->new_catchpoint (type, data) ;
}

Address ProcessController::lookup_symbol(std::string name, std::string objectfile) {
    return current_process->lookup_symbol (name, objectfile) ;
}

Location ProcessController::lookup_address (Address addr) {
    return current_process->lookup_address (addr) ;
}

void ProcessController::push_location () {
    return current_process->push_location() ;
}

Address ProcessController::lookup_function(std::string name, std::string filename, bool skip_preamble) {
    return current_process->lookup_function (name, filename, skip_preamble) ;
}

void ProcessController::list_breakpoints() {
    current_process->list_breakpoints(false) ;
}

void ProcessController::list_symbols() {
    current_process->list_symbols() ;
}

void ProcessController::list_threads() {
    current_process->list_threads() ;
}

void ProcessController::switch_thread(int n) {
    current_process->switch_thread(n) ;
}

void ProcessController::dump(Address addr, int size) {
    current_process->dump(addr, size) ;
}

void ProcessController::stacktrace(int n) {
    current_process->stacktrace(n) ;
}

// void ProcessController::up(int n) {
//     current_process->up(n) ;
// }
//
// void ProcessController::down(int n) {
//     current_process->down(n) ;
// }

void ProcessController::set_frame(int n) {
    current_process->set_frame(n) ;
}

void ProcessController::print_regs() {
    current_process->print_regs(true) ;
}

void ProcessController::show_frame() {
    current_process->show_frame() ;
}

void ProcessController::print_expression(std::string expr, Format &fmt, bool terse, bool record) {
    current_process->print_expression(expr, fmt, terse, record) ;
}

void ProcessController::print_type(std::string expr, bool show_contents) {
    current_process->print_type(expr, show_contents) ;
}

Address ProcessController::evaluate_expression(std::string expr, int &end, bool needint) {
    return current_process->evaluate_expression (expr, end, needint) ;
}

Value ProcessController::evaluate_expression(Node *expr, bool addronly) {
    return current_process->evaluate_expression (expr, addronly) ;
}

Node *ProcessController::compile_expression(std::string expr, int &end, bool single) {
    return current_process->compile_expression (expr, end, single) ;
}

void ProcessController::step(bool by_line, bool over, int n) {
	push_location();

    current_process->step (by_line, over, n) ;
}

void ProcessController::until() {
	push_location();

    current_process->until() ;
}

void ProcessController::until(Address addr) {
	push_location();

    current_process->until(addr) ;
}

void ProcessController::jump(Address addr) {
	push_location ();

	if (current_process->jump(addr))
		ready_wait() ;
}

void ProcessController::ready_wait() {
    for (;;) {
        int status ;
        int pid = 0 ;
        for (uint i = 0 ; i < processes.size() ; i++) {
            pid = processes[i]->dowait(status) ;          // wait for any child to stop
            if (pid > 0) {
                break ;
            }
        }

        // find the process that stopped and call it's wait() function to process the status
        for (uint i = 0 ; i < processes.size() ; i++) {
            if (processes[i]->is_child_pid(pid)) {
                bool keep_waiting = processes[i]->wait (status) ;
                processes[i]->execute_displays() ;
                if (!keep_waiting) {
                    if (processes[i] != current_process) {
                        os.print ("Current process is now %d.\n", i) ;
                        current_process = processes[i];
                    }
                    return ;
                }
            }
        }
    }
}


void ProcessController::disassemble (Address addr) {
    current_process->disassemble (addr) ;
}

void ProcessController::disassemble (Address start, Address end) {
    current_process->disassemble (start, end) ;
}

int ProcessController::set_display (std::string expr, int start, Format &fmt) {
    return current_process->set_display (expr, start, fmt) ;
}

void ProcessController::undisplay (int n)  {
    current_process->undisplay (n) ;
}

void ProcessController::enable_display (int n)  {
    current_process->enable_display (n) ;
}

void ProcessController::disable_display (int n)  {
    current_process->disable_display (n) ;
}

void ProcessController::list_displays() {
    current_process->list_displays() ;

}

void ProcessController::delete_breakpoint(int n) {
    current_process->delete_breakpoint(n) ;

	if (n == 0)
		reset_bp_num();
}

void ProcessController::disable_breakpoint(int n) {
    current_process->disable_breakpoint(n) ;
}

void ProcessController::enable_breakpoint(int n) {
    current_process->enable_breakpoint(n) ;
}

void ProcessController::set_breakpoint_disposition(int n, Disposition disp) {
    current_process->set_breakpoint_disposition(n, disp) ;
}

void ProcessController::set_breakpoint_condition (int bpnum, std::string cond) {
    current_process->set_breakpoint_condition (bpnum, cond) ;
}
                                                                                                                                                   
void ProcessController::set_breakpoint_ignore_count (int bpnum, int n) {
    current_process->set_breakpoint_ignore_count (bpnum, n) ;
}
                                                                                                                                                   
void ProcessController::set_breakpoint_commands (int bpnum, std::vector<ComplexCommand*>& cmds) {
    current_process->set_breakpoint_commands (bpnum, cmds) ;
}

Address ProcessController::lookup_line (std::string filename, int lineno) {
    return current_process->lookup_line (filename, lineno) ;
    
}

Address ProcessController::lookup_line (int lineno) {
    return current_process->lookup_line (lineno) ;
}

void ProcessController::enumerate_functions (std::string name, std::vector<std::string> &results) {
    current_process->enumerate_functions (name, results) ;
}

std::string ProcessController::realname (std::string mangled_name) {
    const char *rn = aliases.find_alias (mangled_name.c_str()) ;
    if (rn == NULL) {
        return mangled_name ;
    }
    return rn ;
}

#if 0

void ProcessController::list () {              // list from last line
    current_process->list() ;
}

void ProcessController::list_back () {              // list from last line
    current_process->list_back() ;
}

void ProcessController::list (std::string filename, int line) {      // list from this line
    current_process->list(filename, line) ;
}

void ProcessController::list (std::string filename, int sline, int eline) {      // list from this line to that line
    current_process->list(filename, sline, eline) ;
}

void ProcessController::list (Address addr, Address endaddr) {              // list this address
    current_process->list(addr, endaddr) ;
}

#endif

void ProcessController::return_from_func(Address value) {              // return from function with value
    current_process->return_from_func(value) ;
}

void ProcessController::finish() {                                     // finish execution of current function
	push_location();

    current_process->finish() ;
}

void ProcessController::examine (const Format &fmt, Address addr) {            // memory dump
    current_process->examine (fmt, addr) ;
}


void ProcessController::info (std::string root, std::string tail) {
    for (uint i = 0 ; i < processes.size() ; i++) {
        processes[i]->info (root, tail) ;
    }
}

void ProcessController::set_signal_actions (std::string name, std::vector<std::string> &actions) {
    current_process->set_signal_actions (name, actions) ;
}


Location ProcessController::get_current_location() {
    return current_process->get_current_location() ;
}

int ProcessController::breakpoint_count() {
    return current_process->breakpoint_count() ;
}

void ProcessController::reset_bp_num() {
    return current_process->reset_bp_num() ;
}

void ProcessController::clear_breakpoints(Address addr) {
    current_process->clear_breakpoints (addr) ;
}

Address ProcessController::get_return_addr() {
    return current_process->get_return_addr() ;
}

int ProcessController::get_main_language() {
    return current_process->get_main_language() ;
}

void ProcessController::complete_symbol (std::string name, std::vector<std::string> &result) {
    current_process->complete_symbol(name, result) ;
}

void ProcessController::complete_function (std::string name, std::vector<std::string> &result) {
    current_process->complete_function(name, result) ;
}

#if 0
void ProcessController::search(std::string text) {
    current_process->search (text) ;
}
#endif

bool ProcessController::test_address (Address addr) {
    return current_process->test_address(addr) ;
}

Address
ProcessController::get_frame_pc (int pid, int tid, int fid)
{
	if (pid < 0 || pid >= (int) processes.size())
		throw Exception ("No such process") ;

	return processes[pid]->get_frame_pc (tid, fid);
}

int
ProcessController::get_frame()
{
	return current_process->get_frame() ;
}

int
ProcessController::get_frame_size()
{
	return current_process->get_frame_size() ;
}

int
ProcessController::get_current_thread()
{
	return current_process->get_current_thread() ;
}

void
ProcessController::print_function_paras (int fid, DIE *die)
{
	return current_process->print_function_paras(fid, die) ;
}

bool
ProcessController::in_sigtramp (std::string name)
{
	return get_arch()->in_sigtramp (current_process, name);
}

File *
ProcessController::find_file(std::string name)
{
	return current_process->find_file(name) ;
}

DIE *
ProcessController::new_int_type()
{
	return current_process->new_int_type() ;
}

RegisterSet *
ProcessController::get_frame_reg()
{
	if (!current_process->is_running())
		throw Exception ("Program not restarted.") ;

	return current_process->get_frame_reg();
}

bool
ProcessController::file_ok ()
{
	return current_process->file_ok();
}

int
ProcessController::get_pid (int n)
{
	if (n < 0 || n >= (int) processes.size())
		throw Exception ("No such process") ;

	return processes[n]->get_pid();
}

const char *
ProcessController::get_state(int n)
{
	if (n < 0 || n >= (int) processes.size())
		throw Exception ("No such process") ;

	return processes[n]->get_state();
}

std::string
ProcessController::get_program (int n)
{
	if (n < 0 || n >= (int) processes.size())
		throw Exception ("No such process") ;

	return processes[n]->get_program();
}

Architecture *
ProcessController::get_arch()
{
	return current_process->get_arch();
}

int
ProcessController::get_threads_number ()
{
	return current_process->get_threads_number();
}

int
ProcessController::get_thread_pid (int n)
{
	return current_process->get_thread_pid(n);
}
