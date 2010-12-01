/*

Copyright (c) 2004 PathScale, Inc.  All rights reserved.
Unpublished -- rights reserved under the copyright laws of the United
States. USE OF A COPYRIGHT NOTICE DOES NOT IMPLY PUBLICATION OR
DISCLOSURE. THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE
SECRETS OF PATHSCALE, INC. USE, DISCLOSURE, OR REPRODUCTION IS
PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF PATHSCALE,
INC.

U.S. Government Restricted Rights:
The Software is a "commercial item," as that term is defined at 48
C.F.R. 2.101 (OCT 1995), consisting of "commercial computer software"
and "commercial computer software documentation," as such terms are used
in 48 C.F.R. 12.212 (SEPT 1995).  Consistent with 48 C.F.R. 12.212 and
48 C.F.R. 227-7202-1 through 227-7202-4 (JUNE 1995), all U.S. Government
End Users acquire the Software with only those rights set forth in the
accompanying license agreement. PathScale, Inc. 477 N. Mathilda Ave;
Sunnyvale, CA 94085.

file: pathcore.cc
created on: Thu Oct  7 14:22:43 PDT 2004
author: David Allison <dallison@pathscale.com>

*/


// read an ELF core file and print relevent information

#include "dbg_elf.h"
#include <sys/user.h>
#include <sys/procfs.h>
#include <stdlib.h>
#include "bstream.h"
#include <signal.h>
#include <limits.h>

// struct CoreThread {
//     CoreThread() : id(++nextid) {}
//
//     int id ;
//     elf_prstatus prstatus ;             // process status
//
// #if LONG_BIT == 64
//     struct user_fpregs_struct fpregset ;           // floating point register set and extended ones too
// #else
//     struct user_fpregs_struct fpregset ;           // floating point register set
//     struct user_fpxregs_struct fpxregset ;        // extended floating point register set
// #endif
//     static int nextid ;
// } ;

int CoreThread::nextid = 0 ;

std::vector<CoreThread*> threads ;
int current_thread ;
elf_prpsinfo prpsinfo ;             // process info

void new_thread() {
    threads.push_back (new CoreThread()) ;
    current_thread = threads.size() - 1 ;
}

void read_note (ELF *core, ProgramSegment *note, std::istream &s) {
    ByteVec *data = note->get_contents(s) ;
    BStream stream (data, ! core->is_little_endian()) ;
    while (!stream.eof()) {
        int32_t namesize = stream.read4() ;
        int32_t descsize = stream.read4() ;
        int32_t type = stream.read4() ;
        std::string name = "" ;
        for (int i = 0 ; i < namesize ; i++) {
            char ch = (char)stream.get() ;
            if (ch != 0) {
                name += ch ;
            }
        }
                                                                                                                                
        // align to the next 4-byte boundary
        int aligned = (stream.offset() + 3) & ~3 ;
        int diff = aligned - stream.offset() ;
        while (diff-- > 0) {
            stream.get() ;
        }
        char *dest = NULL ;
        // lets assume that the pstatus starts a new thread and that the floating point
        // register set belongs to that thread.  This is not documented anywhere that
        // I can find.
        switch (type) {         // note type
        case NT_PRSTATUS:
            new_thread() ;
            dest = (char*)&threads[current_thread]->prstatus ;
            break ;
        case NT_PRPSINFO:
            dest = (char*)&prpsinfo ;
            break ;
        case NT_FPREGSET:
            dest = (char*)&threads[current_thread]->fpregset ;
            break ;
            break ;
        case NT_PRFPXREG:
#if LONG_BIT == 64
            dest = (char*)&threads[current_thread]->fpregset ;
#else
            dest = (char*)&threads[current_thread]->fpxregset ;
#endif
            break ;
        default:
            //std::cerr << "Unrecognized note type: " << type << "\n" ;
            break ;
        }
        if (dest != NULL) {             // copy descriptor to appropriate structure
           for (int i = 0 ; i < descsize ; i++) {
               *dest++ = stream.get() ;
           }
        } else {                        // just skip the descriptor since we don't recognize it
           for (int i = 0 ; i < descsize ; i++) {
               stream.get() ;
           }
        }
    }
}

ELF *read_core (std::string file) {
    ELF *corefile = new ELF (file) ;
    std::istream *s = corefile->open() ;
    for (int i = 0 ; i < corefile->get_num_segments() ; i++) {
        ProgramSegment *segment = corefile->get_segment (i) ;
        switch (segment->get_type()) {
        case PT_LOAD:
            break ;
        case PT_NOTE:
            read_note (corefile, segment, *s) ;
            break ;
        case PT_DYNAMIC:                // XXX: need to do something with these?
        case PT_GNU_EH_FRAME:
            break ;
        default:
            std::cerr << "Unknown core segment type: " << segment->get_type() << "\n" ;
        }

    }
    return corefile ;
}

const char *regs64[] = {
  "r15",
  "r14",
  "r13",
  "r12",
  "rbp",
  "rbx",
  "r11",
  "r10",
  "r9",
  "r8",
  "rax",
  "rcx",
  "rdx",
  "rsi",
  "rdi",
  "orig_rax",
  "rip",
  "cs",
  "eflags",
  "rsp",
  "ss",
  "fs_base",
  "gs_base",
  "ds",
  "es",
  "fs",
  "gs",
  NULL
} ;

const char *regs32[] = {
  "ebx",
  "ecx",
  "edx",
  "esi",
  "edi",
  "ebp",
  "eax",
  "xds",
  "xes",
  "xfs",
  "xgs",
  "orig_eax",
  "eip",
  "xcs",
  "eflags",
  "esp",
  "xss",
  NULL
} ;

void print_regs (ELF *core, CoreThread *thread, int indent) {
    const char **regset = core->is_elf64() ? regs64: regs32 ;
    int regsize = core->is_elf64() ? 8 : 4 ;

    for (int j = 0 ; j < indent ; j++) {
        printf (" ") ;
    }
    printf ("Register contents:\n") ;
    indent += 4 ;
    for (int i = 0 ; regset[i] != NULL ; i++) {
        int64_t value ;
        memcpy (&value, ((char*)&thread->prstatus.pr_reg) + i*regsize, regsize) ;
        for (int j = 0 ; j < indent ; j++) {
            printf (" ") ;
        }
        printf ("%%%-8s\t0x%016llx %12lld ", regset[i], (unsigned long long)value, (unsigned long long)value) ;
        bool one = false ;
        for (int i = 63 ; i >= 0 ; i--) {
            int64_t v = (value & (1LL << i)) ;
            if (v != 0 || one) {
                printf ("%c", v ? '1' : '0') ;
                one = true ;
            }
        }
        if (!one) {
            printf ("0") ;
        }
        printf ("\n") ;
    }
}

struct Signal {
    int number ;
    const char *name ;
    const char *desc ;
} ;

Signal signals[] = {
    {SIGHUP, "SIGHUP", "Hangup"},
    {SIGINT, "SIGINT", "Interrupt"},
    {SIGQUIT, "SIGQUIT", "Quit"},
    {SIGILL, "SIGILL", "Illegal instruction"},
    {SIGTRAP, "SIGTRAP", "Trace trap"},
    {SIGABRT, "SIGABRT", "Abort"},
    {SIGBUS, "SIGBUS", "BUS error"},
    {SIGFPE, "SIGFPE", "Floating-point exception"},
    {SIGKILL, "SIGKILL", "Kill, unblockable"},
    {SIGUSR1, "SIGUSR1", "User-defined signal 1"},
    {SIGSEGV, "SIGSEGV", "Segmentation violation"},
    {SIGUSR2, "SIGUSR2", "User-defined signal 2"},
    {SIGPIPE, "SIGPIPE", "Broken pipe"},
    {SIGALRM, "SIGALRM", "Alarm clock"},
    {SIGTERM, "SIGTERM", "Termination"},
    {SIGSTKFLT, "SIGSTKFLT", "Stack fault"},
    {SIGCHLD, "SIGCHLD", "Child status has changed"},
    {SIGCONT, "SIGCONT", "Continue"},
    {SIGSTOP, "SIGSTOP", "Stop, unblockable"},
    {SIGTSTP, "SIGTSTP", "Keyboard stop"},
    {SIGTTIN, "SIGTTIN", "Background read from tty"},
    {SIGTTOU, "SIGTTOU", "Background write to tty"},
    {SIGURG, "SIGURG", "Urgent condition on socket"},
    {SIGXCPU, "SIGXCPU", "CPU limit exceeded"},
    {SIGXFSZ, "SIGXFSZ", "File size limit exceeded"},
    {SIGVTALRM, "SIGVTALRM", "Virtual alarm clock"},
    {SIGPROF, "SIGPROF", "Profiling alarm clock"},
    {SIGWINCH, "SIGWINCH", "Window size change"},
    {SIGIO, "SIGIO", "I/O now possible"},
    {SIGPWR, "SIGPWR", "Power failure restart"},
    {0, NULL, NULL}
} ;

void printsignal (int sig) {
    for (int i = 0 ; signals[i].name != NULL ; i++) {
        if (signals[i].number == sig) {
            printf ("%d (%s - %s)", sig, signals[i].name, signals[i].desc) ;
            return ;
        }
    }
    printf ("%d (unkown signal)", sig) ;
}

void print_core(ELF *core, const char *filename) {
    printf ("%d-bit ELF Core file %s generated by command: %s\n", core->is_elf64() ? 64 : 32, filename, prpsinfo.pr_psargs) ;
    printf ("Terminating signal: ") ;
    for (unsigned int i = 0 ; i < threads.size() ; i++) {
        if (threads[i]->prstatus.pr_cursig != 0) {
            printsignal (threads[i]->prstatus.pr_cursig) ;
            break ;
        }
    }
    printf ("\n") ;
    if (threads.size() > 1) {
        printf ("There are %d threads\n", (int)threads.size()) ;
        printf ("Terminating thread was ") ;
        for (unsigned int i = 0 ; i < threads.size() ; i++) {
            if (threads[i]->prstatus.pr_cursig != 0) {
                printf ("%d", threads[i]->prstatus.pr_pid) ;
                break ;
            }
        }
        printf ("\n") ;
        for (unsigned int i = 0 ; i < threads.size() ; i++) {
            printf ("    Thread %d\n", threads[i]->id) ;
            print_regs (core, threads[i], 4) ;
        }
    } else {
        printf ("Process ID: %d\n", threads[0]->prstatus.pr_pid) ;
        print_regs (core, threads[0], 0) ;
    }
}

int main (int argc, char **argv) {
    if (argc != 2) {
        printf ("usage: pathcore corefile\n") ;
        exit (1) ;
    }
    ELF *core = read_core (argv[1]) ;
    print_core (core, argv[1]) ;
    exit (0) ;
}

namespace Utils {

std::string toUpper (std::string s) {
    return s ;
}
}

