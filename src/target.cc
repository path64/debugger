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

file: target.cc
created on: Fri Aug 13 11:07:47 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "target.h"
#include "pstream.h"
#include <errno.h>
#include <sys/ptrace.h>
#include "dbg_thread_db.h"
#include <thread_db.h>
#include <unistd.h>
#include <sys/wait.h>
#include "arch.h"
#include "dbg_elf.h"
#include "bstream.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

/* find the offset of X into struct user (from sys/user.h) */
/* XXX: change long to Address, after Address is reset to long */
#define STRUCT_USER_OFFSET(X) ((long)&(((struct user*)0)->X))


// this is where other targets can be created when we have some
Target *Target::new_live_target(Architecture *arch) {
    return new PtraceTarget (arch);
}

Target::Target (Architecture *arch) : arch(arch) {}

//
// functions common to all targets
//
void Target::dump(PStream &os, int pid, Address addr, int size) {
    unsigned char linebuf [16] ;
    while (size > 0) {
        os.print ("%08llX ", addr) ;
        for (int i = 0 ; i < 16 ; i += 4) { ;
             int tmp = read (pid, addr) ;// 4 bytes, little endian
             linebuf[i] = tmp & 0xff ;
             linebuf[i+1] = (tmp >> 8) & 0xff ;
             linebuf[i+2] = (tmp >> 16) & 0xff ;
             linebuf[i+3] = (tmp >> 24) & 0xff ;
             addr += 4 ;
        }
        for (int ch = 0 ; ch < 16; ch++) {
            os.print ("%02X ", linebuf[ch]) ;
        }
        os.print (" ") ;
        for (int i = 0 ; i < 16; i++) {
            unsigned char ch = linebuf[i] ;
            if (ch < 32 || ch >= 127) {
                os.print (".") ;
            } else {
                os.print ("%c", ch) ;
            }
        }
        os.print ("\n") ;
        size -= 16 ;
    }
}

std::string Target::read_string(int pid, Address addr) {
    std::string s = "";
    for (;;) {
        int w = read(pid, addr, 1);
        char ch = w & 0xff;
        if (ch == 0) {
           return s;
        }
        s.push_back(ch);
        addr++;
    }
}
                                                                                                                                           
std::string Target::read_string(int pid, Address addr, int len) {
    std::string s = "";
    for (;;) {
        int w = read(pid, addr, 1);
        char ch = w & 0xff;
        if (len-- == 0) {
           return s;
        }
        s.push_back(ch);
        addr++;
    }
}

//
// LiveTarget functions
//

void LiveTarget::write_string (int pid, Address addr, std::string s) {
    for (unsigned int i = 0 ; i < s.size(); i++) {
       write(pid, addr, s[i], 1); 
       addr++;
    }
}


PtraceTarget::PtraceTarget (Architecture *arch) : LiveTarget(arch), is_attached(false) {}


///////////////////////////////////////////////////////////////////////////////
// Fork a process and attach for debugging
///////////////////////////////////////////////////////////////////////////////
int PtraceTarget::attach (const char* path, const char* args, EnvMap& env)
{
   // This function forks a process and returns the pid
   // of the child.  It returns -1 on failure.  We have
   // to the fork ourselves as system() will cause SIGINT
   // and SIGQUIT to be ignored.

   int pid = fork();
   if (pid == -1) {
      // Fork failed 
      return -1;
   }

   if (pid == 0) { /* Child executes */

      // Absorb stored environmental variables 
      EnvMap::iterator j;
      for (j = env.begin(); j!=env.end(); j++) {
         std::string m = j->first + "=" + j->second;
         putenv(strdup(m.c_str()));
      }

      // Initiatiate process tracing
      ptrace (PTRACE_TRACEME, 0, (void*)0, 0) ;

      // Prepare command string
      std::string cmd_str;
      cmd_str = "exec ";
      cmd_str += path;
      cmd_str += " ";
      cmd_str += args;

      // Prepare child arguments
      const char* argv[5];
      unsigned i = 0;

      argv[i++] = "/bin/sh";
      argv[i++] = "-c";
      argv[i++] = cmd_str.c_str();
      argv[i++] = NULL;

      // Exec into the shell
      execv(argv[0], (char* const*)argv);

      printf("Unable to execute program\n");
      exit(1);
   }

   return pid;
}



int PtraceTarget::attach (std::string filename, int pid) {
    int e = ptrace (PTRACE_ATTACH, pid, (void*)0, 0) ;
    if (e != 0) {
        throw Exception ("Unable to attach to process") ;
    }
    is_attached = true ;
    return pid ;
}

int PtraceTarget::attach (int pid) {
    int e = ptrace (PTRACE_ATTACH, pid, (void*)0, 0) ;
    if (e != 0) {
        throw Exception ("Unable to attach to process") ;
    }
    is_attached = true ;
    return pid ;
}


// detach from the live target.  We need to kill the process and then wait for it to
// die.  The only safe way to do it is to send a SIGKILL to it.
void PtraceTarget::detach(int pid, bool kill) {
    if (is_attached || !kill) {
        int e = ptrace (PTRACE_DETACH, pid, (void*)0, 0) ;
        if (e != 0) {
            throw Exception ("Unable to detach from process") ;
        }
    } else {
        int e = ptrace (PTRACE_KILL,pid, (void*)0, 0) ;
        if (e != 0) {
            perror ("kill") ;
            throw Exception ("Unable to kill the target process") ;
        }
        int status = 0 ;
        e = waitpid (pid, &status, 0) ;             //XXX: WUNTRACED?
        if (e == -1) {
            perror ("wait") ;
            throw Exception ("Unable to reap child process") ;
        }
        // target should be dead now
    }
}

void PtraceTarget::interrupt(int pid) {
    kill (pid, SIGINT) ;
}

void PtraceTarget::cont (int pid, int signal) {
    long e = ptrace (PTRACE_CONT, pid, (void*)0, reinterpret_cast<void*>(signal)) ;
    if (e < 0) {
       perror ("cont") ;
       throw Exception ("Unable to continue")  ;
    }
}

void PtraceTarget::step (int pid) {
    long e = ptrace (PTRACE_SINGLESTEP, pid, (void*)0, 0);
    if (e < 0) {
       perror ("cont");
       throw Exception("Unable to step a single instruction");
    }
}

void PtraceTarget::init_events (int pid) {
    long opts = 0;

    /* construct options */
    opts |= PTRACE_O_TRACEFORK;
    opts |= PTRACE_O_TRACEVFORK;
    opts |= PTRACE_O_TRACEEXEC;
    opts |= PTRACE_O_TRACEVFORKDONE; 

    /* tell ptrace to catch multiprocessing events */ 
    long e = ptrace(PTRACE_SETOPTIONS, pid, (void*)0, opts);
    if (e != 0) {
        printf("Warning: unable to enable ptrace events\n");
    }
}

pid_t PtraceTarget::get_fork_pid(pid_t pid) {
    pid_t fpid;
    long e = ptrace (PTRACE_GETEVENTMSG, pid, (void*)0, &fpid);
    if (e != 0) {
        printf("Warning: unable to get ptrace fork events\n"); 
    }
    return fpid;
}

void PtraceTarget::write(int pid, Address addr, Address data, int size) {
/*  This function writes a number of bytes to an arbitrary address in
 *  the user process's memory.  Since the interface is word-based but
 *  the address may abut against a segment boundary, we have to read
 *  and write to the lowest common word-aligned address.  With a bunch
 *  of masks and bitwise operations we modify only some some bytes.
 *  Note: we use word size from the host not the client.  Also the
 *  code for big endian is absolutely, completely untested.
 */

    int psize = sizeof(long int);
    Address val, hi_mask, lo_mask;
    int offset = 0;

    /* require a little sanity */
    if (size < 1 || size > (int)sizeof(Address)) {
       throw Exception ("Unable write memory with size %d", size);
    }

    /* find lowest word boundary */
    if ( size != psize ) {
       addr -= offset = addr & (psize-1);
    }

    /* for large writes loop through data */
    if ( size > psize - offset ) {
       int lo_size = psize - offset;

       if (arch->is_little_endian()) {
          addr += offset;
          while (size > 0) {
             write(pid, addr, data, lo_size);
             data >>= (CHAR_BIT*lo_size);
             addr += lo_size;
             size -= lo_size;

             lo_size = size < psize ? size : psize;
          }
       } else {
          addr += offset + size - lo_size;
          while (size > 0) {
             write(pid, addr, data, lo_size);
             data >>= (CHAR_BIT*lo_size);
             size -= lo_size;

             lo_size = size < psize ? size : psize;
             addr -= lo_size;
          }
       }

       return;
    }

    /* prepare mask of the lower bits */
    if (size < (int)sizeof(Address)) {
       lo_mask = ~((~(Address)0) << (CHAR_BIT*size));
    } else {
       /* excessive shift is impl-defined */
       lo_mask = ~(Address)0;
    }


    /* flip the offset for big endian */
    if ( !arch->is_little_endian() ) {
       offset = psize - size - offset;
    }

    /* prepare mask for now-shifted bits */
    hi_mask = lo_mask << (CHAR_BIT*offset);

    /* read in from the shifted address */
    val = read(pid, addr, psize);

    /* mask out the unwanted bits */
    val = val & ~hi_mask;

    /* mask in the desired data bits */
    val |= (data&lo_mask) << (CHAR_BIT*offset);

    /* write to now-shifted address */
    errno = 0;
    void* caddr = reinterpret_cast<void*>(addr);
    val = ptrace (PTRACE_POKETEXT, pid, caddr, val);

    /* bad things! bad things! */
    if (errno != 0) {
       switch (errno) {
       case EPERM: throw Exception("Process %d cannot be traced", pid);
       case ESRCH: throw Exception("Process %d does not exist", pid);
       default: throw Exception ("Unable to write memory at 0x%llx", addr);
       }
    }
}

bool PtraceTarget::test_address(int pid, Address addr) {
    try {
       read(pid, addr, 1);
       return 1;
    } catch (...) {
       return 0;
    }
}
                                                                                                                                           
Address PtraceTarget::read(int pid, Address addr, int size) {
/*  This function reads and returns a number of bytes from an arbitrary
 *  address in the user process's memory.  Since the interface is word-
 *  based but the address may abut against a segment boundary, we have
 *  to read from the lowest common word-aligned address.  Then through
 *  a bunch of shifts and bitwise ops we reconsitute the value. Note:
 *  we use word size from the host not the client.  Also, the code for
 *  big endian is absolutely, completely untested.
 */

    int psize = sizeof(long int);
    Address val, hi_mask, lo_mask;
    int offset = 0;

    /* require a little sanity */
    if (size < 1 || size > (int)sizeof(Address)) {
       throw Exception ("Unable read memory with size %d", size);
    }

    /* find lowest word boundary */
    if ( size != psize ) {
       addr -= offset = addr & (psize-1);
    }

    /* for large reads loop through data */
    if ( size > psize - offset ) {
       int lo_size = psize - offset;
       unsigned num = 0;

       if (arch->is_little_endian()) {
          val=0; addr += offset;
          while (size > 0) {
             Address lo = read(pid, addr, lo_size);
             val |= lo << (CHAR_BIT*num);
             num += lo_size;
             addr += lo_size;
             size -= lo_size;
 
             lo_size = size < psize ? size : psize; 
          }
       } else {
          val=0; addr += offset + size - lo_size;
          while (size > 0) {
             Address lo = read(pid, addr, lo_size);
             val |= lo << (CHAR_BIT*num);
             num += lo_size;
             size -= lo_size;

             lo_size = size < psize ? size : psize; 
             addr -= lo_size;
          }
       }

       return val;
    }

    /* prepare mask of the lower bits */
    if (size < (int)sizeof(Address)) {
       lo_mask = ~((~(Address)0) << (CHAR_BIT*size));
    } else {
       /* excessive shift is impl-defined */
       lo_mask = ~(Address)0;
    }

    /* flip over offset for big endian */
    if ( !arch->is_little_endian() ) {
       offset = psize - offset - size;
    }

    /* prepare mask for now-shifted bits */
    hi_mask = lo_mask << (CHAR_BIT*offset);

    /* read in from now-shifted address */
    errno = 0;
    void* caddr = reinterpret_cast<void*>(addr);
    val = ptrace (PTRACE_PEEKTEXT, pid, caddr, 0);

    /* mask out our bits, and shift into place */
    val = (val & hi_mask) >> (CHAR_BIT*offset);

    /* never do sign extension */
    val = val & lo_mask;

    /* bad things! bad things! */
    if (errno != 0) {
       switch (errno) {
       case EPERM: throw Exception("Process %d cannot be traced", pid);
       case ESRCH: throw Exception("Process %d does not exist", pid);
       default: throw Exception ("Unable to read memory at 0x%llx", addr);
       }
    }

    return val;
}

Address PtraceTarget::readptr (int pid, Address addr) {
    return read(pid, addr, arch->ptrsize());
}

void PtraceTarget::get_regs(int pid, unsigned char *regs) {
    long e = ptrace (PTRACE_GETREGS, pid, (void*)0, regs) ;
    if (e < 0) {
        throw Exception ("Unable to read registers")  ;
    }
}

void PtraceTarget::set_regs(int pid, unsigned char *regs) {
    long e = ptrace (PTRACE_SETREGS, pid, (void*)0, regs) ;
    if (e < 0) {
        throw Exception ("Unable to write registers")  ;
    }
}

void PtraceTarget::get_fpregs(int pid, unsigned char *regs) {
    long e = ptrace (PTRACE_GETFPREGS, pid, (void*)0, regs) ;
    if (e < 0) {
        throw Exception ("Unable to read floating point registers")  ;
    }
}

void PtraceTarget::set_fpregs(int pid, unsigned char *regs) {
    long e = ptrace (PTRACE_SETFPREGS, pid, (void*)0, regs) ;
    if (e < 0) {
        throw Exception ("Unable to write floating point registers")  ;
    }
}

void PtraceTarget::get_fpxregs(int pid, unsigned char *regs) {
    long e = ptrace (PTRACE_GETFPXREGS, pid, (void*)0, regs) ;
    if (e < 0) {
        throw Exception ("Unable to read floating point registers")  ;
    }
}

void PtraceTarget::set_fpxregs(int pid, unsigned char *regs) {
    long e = ptrace (PTRACE_SETFPXREGS, pid, (void*)0, regs) ;
    if (e < 0) {
        throw Exception ("Unable to write floating point registers")  ;
    }
}

long PtraceTarget::get_debug_reg (int pid, int reg) {
    /* find offset and access struct user */
    Address addr = STRUCT_USER_OFFSET(u_debugreg[reg]);
    long e = ptrace (PTRACE_PEEKUSER, pid, addr, 0);
    if (errno != 0) {
        throw Exception ("Unable to read debug register");
    }

    /* cast to Address */
    return e;
}

void PtraceTarget::set_debug_reg (int pid, int reg, long value) {
    /* find offset and write struct user */
    Address addr = STRUCT_USER_OFFSET(u_debugreg[reg]);
    long e = ptrace (PTRACE_POKEUSER, pid, addr, value) ;
    if (e < 0) {
        throw Exception ("Unable to write debug register")  ;
    }
}


// core target

int CoreThread::nextid = 1 ;

CoreTarget::CoreTarget (Architecture *arch, std::string corefile) : Target(arch), corefile(corefile) {
    fd = open (corefile.c_str(), O_RDONLY) ;
    if (fd == -1) {
       throw Exception ("Unable to open core file") ;
    }
    core = new ELF (corefile) ;
    std::istream *s = core->open() ;
    
    int nsegs = core->get_num_segments() ;
    for (int i = 0 ; i < nsegs ; i++) {
        ProgramSegment *segment = core->get_segment (i) ;
        switch (segment->get_type()) {
        case PT_LOAD:
            load_segment (segment) ;
            break ;
        case PT_NOTE:
            read_note (segment, *s) ;
            break ;
        case PT_DYNAMIC:                // XXX: need to do something with these?
        case PT_GNU_EH_FRAME:
            break ;
        default:
            std::cerr << "Unknown core segment type: " << segment->get_type() << "\n" ;
        }
    }
    // XXX: close the elf stream
}

CoreTarget::~CoreTarget() {
    detach(0) ;
}

int CoreTarget::attach (const char* prog, const char* args, EnvMap& env)
{
    return -1;
}


// map all the segments that are in the given file.  This is used to map in the 
// main program before reading the dynamic information.  The dynamic information may
// refer to an address within the main program.
void CoreTarget::map_file (ELF *file) {
    int fd = open (file->get_name().c_str(), O_RDONLY) ;
    if (fd < 0) {
        throw Exception ("Cannot open ELF file") ;
    }
    open_files.push_back (fd) ;
    std::vector<SegmentList::iterator> found_segs ;
    for (SegmentList::iterator i = pending_segments.begin() ; i != pending_segments.end() ; i++) {
        Address addr = (*i)->get_start() ;
        ProgramSegment *seg = file->find_segment (addr) ;
        if (seg != NULL) {
            void *vaddr = seg->map (fd) ;
            if (vaddr == NULL) {
                printf ("warning: failed to map segment for address 0x%llx\n", (unsigned long long)addr) ;
                continue ;
            }

            regions.add(seg->get_start(),
                         seg->get_end(),
                         (char*) vaddr);

            found_segs.push_back (i) ;
        }
    }

    // now erase the found segments from the pending list
    for (unsigned int i = 0 ; i < found_segs.size() ; i++) {
        pending_segments.erase (found_segs[i]) ;
    }
}

void CoreTarget::map_code (std::vector<ELF*> &files) {
    for (SegmentList::iterator i = pending_segments.begin() ; i != pending_segments.end() ; i++) {
        Address addr = (*i)->get_start() ;
        //printf ("mapping code segment 0x%llx\n", addr) ;
        for (unsigned int j = 0 ; j < files.size() ; j++) {
             ProgramSegment *seg = files[j]->find_segment (addr) ;
             if (seg != NULL) {
                 int fd = open (files[j]->get_name().c_str(), O_RDONLY) ;
                 if (fd < 0) {
                     throw Exception ("Cannot open ELF file") ;
                 }
                 open_files.push_back (fd) ;
                 void *vaddr = seg->map (fd) ;
                 if (vaddr == NULL) {
                     printf ("warning: failed to map segment for address 0x%llx\n", (unsigned long long)addr) ;
                     continue ;
                 }

                 regions.add(seg->get_start(),
                              seg->get_end(),
                              (char*) vaddr);
                 break ;
             }
        }
    }
}


void CoreTarget::read_note (ProgramSegment *note, std::istream &s) {
    BVector data = note->get_contents(s) ;
    BStream stream (data, ! core->is_little_endian()) ;
    while (!stream.eof()) {
        int32_t namesize = stream.read4u() ;
        int32_t descsize = stream.read4u() ;
        int32_t type = stream.read4u() ;
        std::string name = "" ;
        for (int i = 0 ; i < namesize ; i++) {
            char ch = (char)stream.read1u() ;
            if (ch != 0) {
                name += ch ;
            }
        }

        // align to the next 4-byte boundary
        int aligned = (stream.offset() + 3) & ~3 ;
        int diff = aligned - stream.offset() ;
        while (diff-- > 0) {
            stream.read1u() ;
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
        case NT_PRFPXREG:
#if __WORDSIZE == 64
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
               *dest++ = stream.read1u() ;
           }
        } else {                        // just skip the descriptor since we don't recognize it
           for (int i = 0 ; i < descsize ; i++) {
               stream.read1u() ;
           }
        }
    }
}

void CoreTarget::load_segment (ProgramSegment *seg) {
    void *addr = seg->map (fd) ;
    if (addr == NULL) {
        pending_segments.push_back (seg) ;
        return ;
    }

    regions.add(seg->get_start(),
                 seg->get_end(),
                 (char*) addr);
}

int CoreTarget::attach (std::string filename, int pid) {
    throw Exception ("Cannot attach to a core file with a pid") ;
}

int CoreTarget::attach (int pid) {
    throw Exception ("Cannot attach to a core file with a pid") ;
}

// detach from core by unmapping all the regions and closing the files
void CoreTarget::detach(int pid, bool kill) {
    Map_Range<Address,char*>::iterator i;
    for (i=regions.begin(); i!=regions.end(); ++i) {
       munmap(i->val, i->hi - i->lo + 1);
    }

    for (unsigned int i = 0 ; i < open_files.size() ; i++) {
        close (open_files[i]) ;
    }
    delete core ;
    close (fd) ;
}

void CoreTarget::interrupt(int pid) {
}


void CoreTarget::write(int pid, Address addr, Address data, int size) {
    throw Exception ("Can't write to a core file") ;
}

Address CoreTarget::read(int pid, Address addr, int size) {
    Address v = 0 ;

    Map_Range<Address,char*>::iterator i;
    i = regions.find(addr);

    if (i == regions.end() || i->hi <= addr+size) {
       throw Exception ("Unable to read memory at address 0x%llx", addr) ;
    }

    // XXX: big endian won't work
    memcpy (&v, addr - i->lo + i->val, size) ;
    return v ;
}

Address CoreTarget::readptr (int pid, Address addr) {
    return read (pid, addr, arch->ptrsize()) ;
}

void CoreTarget::get_regs(int pid, unsigned char *regs) {
    memcpy (regs, &find_thread(pid)->prstatus.pr_reg, sizeof (struct user_regs_struct)) ;
}
                                                                                                                                           
void CoreTarget::set_regs(int pid, unsigned char *regs) {
    memcpy (&find_thread(pid)->prstatus.pr_reg, regs, sizeof (struct user_regs_struct)) ;
}

void CoreTarget::get_fpregs(int pid, unsigned char *regs) {
    memcpy (regs, &find_thread(pid)->fpregset, sizeof (user_fpregs_struct)) ;
}

void CoreTarget::set_fpregs(int pid, unsigned char *regs) {
    memcpy (&find_thread(pid)->fpregset, regs, sizeof (user_fpregs_struct)) ;
}

void CoreTarget::get_fpxregs(int pid, unsigned char *regs) {
#if __WORDSIZE == 64
#else
    memcpy (regs, &find_thread(pid)->fpxregset, sizeof (struct user_fpxregs_struct)) ;
#endif
}

void CoreTarget::set_fpxregs(int pid, unsigned char *regs) {
#if __WORDSIZE == 64
#else
    memcpy (&find_thread(pid)->fpxregset, regs, sizeof (struct user_fpxregs_struct)) ;
#endif
}

int CoreTarget::get_signal() {
    for (unsigned int i = 0 ; i < threads.size() ; i++) {
        if (threads[i]->prstatus.pr_cursig != 0) {
            return threads[i]->prstatus.pr_cursig ;
        }
    }
    return 0 ;
}

// return the pid of the terminating thread
int CoreTarget::get_terminating_thread() {
    for (unsigned int i = 0 ; i < threads.size() ; i++) {
        if (threads[i]->prstatus.pr_cursig != 0) {
            return threads[i]->prstatus.pr_pid ;
        }
    }
    return 0 ;
}

std::string CoreTarget::get_program() {
    return prpsinfo.pr_fname ;
}

void CoreTarget::cont (int pid, int signal) {
    throw Exception ("Can't continue execution on a core file") ;
}

void CoreTarget::step (int pid) {
    throw Exception ("Can't single step a core file") ;
}


int CoreTarget::get_num_threads() {
    return (int)threads.size() ;
}

void CoreTarget::new_thread() {
    threads.push_back (new CoreThread()) ;
    current_thread = get_num_threads() - 1 ;
}

int CoreTarget::get_thread_pid(int n) {
    return threads[n]->prstatus.pr_pid ;
}

void *CoreTarget::get_thread_tid(int n) {
    return threads[n] ;
}

CoreThread *CoreTarget::find_thread (int pid) {
    for (unsigned int i = 0 ; i < threads.size() ; i++) {
        if (threads[i]->prstatus.pr_pid == pid) {
            return threads[i] ;
        }
    }
    throw Exception ("No thread for process id %d", pid) ;
}
