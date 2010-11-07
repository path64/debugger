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
#include <climits> 
#include <sys/ptrace.h>
#include "ptrace_target.h"


// this is where other targets can be created when we have some
Target *Target::new_live_target(Architecture *arch) {
	// FIXME: This should be implemented in the file with the host platform's
	// target implementation.
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

void CoreTarget::get_regs(int pid, RegisterSet *regs) {
    //XXX
    //memcpy (regs, &find_thread(pid)->prstatus.pr_reg, sizeof (struct user_regs_struct)) ;
}

void CoreTarget::set_regs(int pid, RegisterSet *regs) {
    //XXX
    //memcpy (&find_thread(pid)->prstatus.pr_reg, regs, sizeof (struct user_regs_struct)) ;
}

void CoreTarget::get_fpregs(int pid, RegisterSet *regs) {
    //XXX
    //memcpy (regs, &find_thread(pid)->fpregset, sizeof (user_fpregs_struct)) ;
}

void CoreTarget::set_fpregs(int pid, RegisterSet *regs) {
    //XXX
    //memcpy (&find_thread(pid)->fpregset, regs, sizeof (user_fpregs_struct)) ;
}

void CoreTarget::get_fpxregs(int pid, RegisterSet *regs) {
	//XXX
// #if __WORDSIZE == 64
// #else
//     memcpy (regs, &find_thread(pid)->fpxregset, sizeof (struct user_fpxregs_struct)) ;
// #endif
}

void CoreTarget::set_fpxregs(int pid, RegisterSet *regs) {
	//XXX
// #if __WORDSIZE == 64
// #else
//     memcpy (&find_thread(pid)->fpxregset, regs, sizeof (struct user_fpxregs_struct)) ;
// #endif
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

void CoreTarget::write_string (int pid, Address addr, std::string s) {
	throw Exception ("Can't write string to a core file") ;
}

void CoreTarget::init_events (int pid) {
	throw Exception ("Can't init events of a core file") ;
}

pid_t CoreTarget::get_fork_pid (pid_t pid) {
	throw Exception ("Can't get fork pid from a core file") ;
}

long CoreTarget::get_debug_reg (int pid, int reg) {
	throw Exception ("Can't get debug reg from a core file") ;
}

void CoreTarget::set_debug_reg (int pid, int reg, long value) {
	throw Exception ("Can't set debug reg to a core file") ;
}
