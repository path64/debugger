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

file: arch.cc
created on: Fri Aug 13 11:07:33 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "arch.h"
#include "process.h"
#include "dbg_thread_db.h"
#include "target.h"

// floating point registers:
//    ptrace has 2 requests for getting/setting floating point registers.  They are GETFPREGS
//    and GETFPXREGS.  The former uses the struct user_fpregs_struct from /usr/include/sys/user.h,
//    the latter struct user_fpxregs_struct.  The GETFPREGS only reads the x87 registers (called
//    plain floating point registers in the header file).  The GETFPXREGS cal read both the
//    x87 registers and the SSE ones.  
//
//    The x87 registers are also mapped as the mmx registers, but these are never used by
//    applications any more.  They are 16 bytes each in the kernel but actually only the
//    bottom 80 bits (10 bytes) are used.


IntelArch::IntelArch (int num_debug_regs) 
    : num_debug_regs(num_debug_regs), regs_valid(false)
     {
     debug_regs = new Address [num_debug_regs] ;
     refcounts = new int [num_debug_regs] ;
     memset (debug_regs, 0, num_debug_regs * sizeof (Address)) ;
     memset (refcounts, 0, num_debug_regs * sizeof (int)) ;
     status = 0 ;
     control = 0 ;

}

IntelArch::~IntelArch() {
    delete [] debug_regs ;
    delete [] refcounts ;
}

int IntelArch::allocate_debug_reg(Address addr) {
    // first see if one is already allocated for the address
    for (int i = 0 ; i < num_debug_regs ; i++) {
        if (refcounts[i] != 0 && debug_regs[i] == addr) {
            refcounts[i]++ ;
            return i ;
        }
    }
    // no address matches, look for a free reg
    for (int i = 0 ; i < num_debug_regs ; i++) {
        if (refcounts[i] == 0) {
            refcounts[i]++ ;
            debug_regs[i] = addr ;
            return i ;
        }
    }
    throw Exception ("No free debug registers") ;
}

void IntelArch::free_debug_reg (int reg) {
    if (refcounts[reg] == 0) {
        throw Exception ("Attempt to free an unallocated debug register") ;
    }
    refcounts[reg]-- ;
    if (refcounts[reg] == 0) {
        debug_regs[reg] = 0 ;
    }
}

// see the AMD manual, volume 2 page 388
void IntelArch::set_watchpoint (Process *proc, int reg, WatchpointType type, int size) {
    if (refcounts[reg] == 0) {
        throw Exception ("Debug register %d is not allocated", reg) ;
    }

    // convert size to len
    int len = 0 ;
    switch (size) {
    case 1:
        len = 0 ;
        break ;
    case 2:
        len = 1 ;
        break ;
    case 4:
        len = 3 ;
        break ;
    case 8:
        len = 4 ;
        break ;
    default:
        throw Exception ("Can't watch region of this size") ;
    }
    // convert type to rw
    int rw = 0 ;
    switch (type) {
    case WP_EXEC:
        rw = 0 ;
        break ;
    case WP_WRITE:
        rw = 1 ;
        break ;
    case WP_RW:
        rw = 3 ;
        break ;
    default:
        throw Exception ("Unknown watchpoint type") ;
    }

    int v = (len << 2) | rw ;                        // form the R/Wx and LENx field
    control &= ~(0x0f << (16 + reg * 4)) ;              // clear the bit field
    control |= v << (16 + reg * 4);                     // set it to the new value
    control |= 1 << (reg * 2) ;                         // set the local enable bit

    // now set them in the processor
    proc->set_debug_reg (reg, debug_regs[reg]) ;
    proc->set_debug_reg (DR_CONTROL, control) ;
}

void IntelArch::clear_watchpoint (Process *proc, int reg) {
    debug_regs[reg] = 0 ;
    // don't free the reg
    control &= ~(1 << reg * 2) ;                // clear the enable bit

    // now set them in the processor
    proc->set_debug_reg (reg, 0) ;
    proc->set_debug_reg (DR_CONTROL, control) ;

}

int IntelArch::get_debug_status(Process *proc) {
    status = proc->get_debug_reg (DR_STATUS) ;          // get the status register
    proc->set_debug_reg (DR_STATUS, 0) ;                // clear the bits (needs to be done by software)
    return status ;                                     // return the value to the caller
}

void IntelArch::clear_all_watchpoints(Process *proc) {
    for (int i = 0 ; i < num_debug_regs ; i++) {
        debug_regs[i] = 0 ;
        refcounts[i] = 0 ;
        proc->set_debug_reg (i, 0) ;
    }
    control = 0 ;
    proc->set_debug_reg (DR_CONTROL, control) ;
    regs_valid = false ;
}

Address IntelArch::get_debug_reg (Process *proc, int reg) {
    if (!regs_valid) {
        for (int i = 0 ; i < num_debug_regs ; i++) {
            debug_regs[i] = proc->get_debug_reg(i) ;
        }
        regs_valid = true ;
    }
    return debug_regs[reg] ;
}

int IntelArch::get_available_debug_regs() {
    int n = 0 ;
    for (int i = 0 ; i < num_debug_regs ; i++) {
        if (refcounts[i] == 0) {
            n++ ;
        }
    }
    return n ;
}


// get the next frame given the pc, fp and sp.  Write the new pc, fp and sp values
// to the args

void IntelArch::guess_frame (Process *proc, Frame* old_f, Frame* new_f) {
    Address old_pc = old_f->get_pc();
    Address old_fp = old_f->get_fp();
    Address old_sp = old_f->get_sp();

    if (inframemaker (proc, old_pc)) {
        int opcode = proc->read(old_pc, 1);
        // need to check if we are on the first or second instruction of the frame maker
        // if we are on the first, then the return address is the top of the stack, otherwise
        // we have pushed %ebp (or %rbp) and the return address is second on the stack, after
        // the frame pointer (which remains the same)
        if (opcode == 0x55) {           // push %ebp
            new_f->set_pc(proc->readptr(old_sp));
            new_f->set_sp(old_sp);
            new_f->set_fp(old_fp);
        } else {
            new_f->set_pc(proc->readptr (old_sp+ptrsize()));
            new_f->set_sp(old_sp + 2*ptrsize());
            new_f->set_fp(old_fp);
        }
    } else {
        Address newpc = proc->readptr (old_fp + ptrsize()) ;          // read return address 
        if (!proc->test_address (newpc)) {              // see if address is valid
            // might be that there is no stack frame for this function, in which case
            // the return address is the top of the stack
            new_f->set_pc(proc->readptr (old_sp));
            new_f->set_sp(old_sp + ptrsize());               // sp moves up the stack
            new_f->set_fp(old_fp);
        } else {
            new_f->set_pc(old_fp + 2*ptrsize());             // previous stack pointer
            new_f->set_fp(proc->readptr (old_fp));           // read previous frame pointer
            new_f->set_pc(newpc);                          
        }
    }
}

// allocate some space on the stack (aligned to 4 bytes)
Address IntelArch::stack_space (Process *proc, int bytes) {
    Address sp = proc->get_reg ("sp") ;
    bytes = (bytes + 3) & ~3 ;                          // align to 4 byte
    sp -= bytes ;
    proc->set_reg ("sp", sp) ;
    return sp ;
}

bool IntelArch::is_little_endian() {
    return true ;
}

// align stack to 16 bytes for the SSE stuff
void IntelArch::align_stack (Process *proc) {
    Address sp = proc->get_reg("sp") ;
    while ((sp & 0xf) != 0) {
       sp-- ;
    }
    proc->set_reg ("sp", sp) ;
}


i386Arch::i386Arch () : IntelArch (4)
 {
    regnames["ebx"] = 0 * sizeof (long int) ;
    regnames["ecx"] = 1 * sizeof (long int) ;
    regnames["edx"] = 2 * sizeof (long int) ;
    regnames["esi"] = 3 * sizeof (long int) ;
    regnames["edi"] = 4 * sizeof (long int) ;
    regnames["ebp"] = 5 * sizeof (long int) ;
    regnames["eax"] = 6 * sizeof (long int) ;
    regnames["xds"] = 7 * sizeof (long int) ;
    regnames["es"] = 8 * sizeof (long int) ;
    regnames["fs"] = 9 * sizeof (long int) ;
    regnames["gs"] = 10 * sizeof (long int) ;
    //regnames["orig_eax"] = 11 * sizeof (long int) ;
    regnames["eip"] = 12 * sizeof (long int) ;
    regnames["cs"] = 13 * sizeof (long int) ;
    regnames["eflags"] = 14 * sizeof (long int) ;
    regnames["esp"] = 15 * sizeof (long int) ;
    regnames["ss"] = 16 * sizeof (long int) ;
                                                                                                                                         
    regnames["pc"] = 12 * sizeof (long int) ;                 // alias for eip
    regnames["sp"] = 15 * sizeof (long int) ;                 // alias for esp
    regnames["fp"] = 5 * sizeof (long int) ;                 // alias for ebp

    // the floating point register buffer contains the user_fpxregs_struct

    // each of the floating point registers is 16 bytes long

    // see /usr/include/sys/user.h for the user_fpxregs_struct definition

    const int st_start = sizeof (unsigned short) * 4 + sizeof(long) * 6 ;

    regnames["st0"] = st_start + 0 * 16 ;
    regnames["st1"] = st_start + 1 * 16 ;
    regnames["st2"] = st_start + 2 * 16 ;
    regnames["st3"] = st_start + 3 * 16 ;
    regnames["st4"] = st_start + 4 * 16 ;
    regnames["st5"] = st_start + 5 * 16 ;
    regnames["st6"] = st_start + 6 * 16 ;
    regnames["st7"] = st_start + 7 * 16 ;

    // MMX registers are the same as the x87 registers
    regnames["mm0"] = st_start + 0 * 16 ;
    regnames["mm1"] = st_start + 1 * 16 ;
    regnames["mm2"] = st_start + 2 * 16 ;
    regnames["mm3"] = st_start + 3 * 16 ;
    regnames["mm4"] = st_start + 4 * 16 ;
    regnames["mm5"] = st_start + 5 * 16 ;
    regnames["mm6"] = st_start + 6 * 16 ;
    regnames["mm7"] = st_start + 7 * 16 ;


    // there are 8 SSE registers
    const int sse_start = sizeof (unsigned short) * 4 + sizeof(long) * 6 + 32 * sizeof(long);

    regnames["xmm0"] = sse_start + 0 * 16 ;
    regnames["xmm1"] = sse_start + 1 * 16 ;
    regnames["xmm2"] = sse_start + 2 * 16 ;
    regnames["xmm3"] = sse_start + 3 * 16 ;
    regnames["xmm4"] = sse_start + 4 * 16 ;
    regnames["xmm5"] = sse_start + 5 * 16 ;
    regnames["xmm6"] = sse_start + 6 * 16 ;
    regnames["xmm7"] = sse_start + 7 * 16 ;

    // store the register names
    for (RegMap::iterator reg = regnames.begin() ; reg != regnames.end() ; reg++) {
        allnames.push_back (reg->first) ;
    }

    commonnames.push_back ("eax") ;
    commonnames.push_back ("ebx") ;
    commonnames.push_back ("ecx") ;
    commonnames.push_back ("edx") ;
    commonnames.push_back ("esi") ;
    commonnames.push_back ("edi") ;
    commonnames.push_back ("ebp") ;
    commonnames.push_back ("esp") ;
    commonnames.push_back ("eip") ;
    commonnames.push_back ("eflags") ;
    commonnames.push_back ("ds") ;
    commonnames.push_back ("es") ;
    commonnames.push_back ("fs") ;
    commonnames.push_back ("gs") ;

    regnums[0] = 6 * sizeof (long int) ;
    regnums[1] = 1 * sizeof (long int) ;
    regnums[2] = 2 * sizeof (long int) ;
    regnums[3] = 0 * sizeof (long int) ;
    regnums[4] = 15 * sizeof (long int) ;
    regnums[5] = 5 * sizeof (long int) ;
    regnums[6] = 3 * sizeof (long int) ;
    regnums[7] = 4 * sizeof (long int) ;
    regnums[8] = 12 * sizeof (long int) ;
    regnums[9] = 14 * sizeof (long int) ;

    // floating point registers
    for (int i = 11 ; i < 19 ; i++) {
       regnums[i] = st_start + (i-11) * 16 ;
    }

    // SSE registers
    for (int i = 21 ; i < 29 ; i++) {
       regnums[i] = sse_start + (i-21) * 16 ;
    }

    // MMX registers (same as floating point registers)
    for (int i = 29 ; i < 37 ; i++) {
       regnums[i] = st_start + (i-29) * 16 ;
    }

    // XXX: and others

    disassembler = new OpteronDisassembler(false) ;
}

i386Arch::~i386Arch() {
}

RegnameVec &i386Arch::get_regnames(bool all) {
    if (all) {
        return allnames ;
    } else {    
        return commonnames ;
    }

}

int i386Arch::bpsize() {
    return 1 ;
}

int i386Arch::get_regbuffer_size() {
    return sizeof (struct user_regs_struct) ;
}

int i386Arch::get_fpregbuffer_size() {
#if __WORDSIZE == 64
    return  0 ;
#else
    return  sizeof (struct user_fpxregs_struct) ;
#endif
}


int i386Arch::get_num_regs() {
    return sizeof (struct user_regs_struct) / sizeof(long int) ;
}

int i386Arch::get_num_fpregs() {
    return 16 ;                 // 8 fp/xmm regs + 8 MMX regsters
}

int i386Arch::get_reg_size() {
    return sizeof(long int) ;
}

int i386Arch::get_fpreg_size() {
    return 16 ;           // XXX: check this
}

int i386Arch::regnum2offset(int rnum) {
    return rnum * sizeof(long int) ;
}

// the registers start at an offset into the user_fpxregs_struct.  (see /usr/include/sys/user.h)
int i386Arch::fpregnum2offset(int rnum) {
    return sizeof(unsigned short) * 4 + sizeof(long) * 6 + rnum * 16 ;
}

bool i386Arch::isfpreg(int dwarfnum) {
    return dwarfnum > 10 ;
}

int i386Arch::translate_regname(std::string name) {
    RegMap::iterator num = regnames.find (name) ;
    if (num == regnames.end()) {
        throw Exception ("No such register ");
    }
    return num->second ;
}

std::string i386Arch::reverse_translate_regnum(int num) {
   for (RegMap::iterator i = regnames.begin() ; i != regnames.end(); i++) {
       if (i->second == num) {
           return "%" + i->first  ;
       }
   }
   return "%unknown" ;
}

int i386Arch::translate_regnum(int dwarfnum) {
    RegnumMap::iterator i = regnums.find (dwarfnum) ;
    if (i == regnums.end()) {
        throw Exception ("No such DWARF register ") ;
    }
    return i->second ;
}

int i386Arch::translate_fpregname(std::string name) {
    throw Exception("not reached");
}

int i386Arch::set_breakpoint(Process * proc, Address addr) {
    int oldvalue = proc->raw_read (addr, 4) ;
    proc->write (addr, (oldvalue & ~0xff) | 0xcc) ;   /* XXX: broken should do one at a time*/
    return oldvalue ;
}

bool i386Arch::breakpoint_present(Process * proc, Address addr) {
    int v = proc->raw_read (addr, 1) ;
    if (v == 0xcc) {
        return true ;
    }
    return false ;
}

int i386Arch::ptrsize() {
    return 4 ;
}

static bool isframeinst (int opcode) {
    if ((opcode & 0xff) == 0x55) {        // push %ebp
        return true ;
    } else if  ((opcode & 0xffff) == 0xe589) {       // mov %esp, %ebp
        return true ;
    }
    return false ;
}

bool i386Arch::inframemaker(Process * proc, Address pc) {
    if (!proc->test_address (pc)) {
        return false ;
    }
    int opcode = proc->raw_read (pc, 4) ;// read 4 bytes
    if ((opcode & 0xff) == 0xcc) {   // breakpoint?
        typedef std::list<Breakpoint*> BreakpointList ;
        BreakpointList *bps = proc->find_breakpoint (pc) ;
        for (BreakpointList::iterator i = bps->begin() ; i != bps->end() ; i++) {
            Breakpoint *bp = *i ;
            if (bp->is_software()) {
                SoftwareBreakpoint *abp = dynamic_cast<SoftwareBreakpoint*>(bp) ;
                int oldvalue = abp->get_old_value() ;
                if (isframeinst (oldvalue)) {
                    return true ;
                }
            }
        }
        return false ;
    }
    return isframeinst (opcode) ;
}

Address i386Arch::skip_preamble (Process *proc, Address addr) {
    while (inframemaker (proc, addr)) {
        addr++ ;
    }
    return addr ;
}

// this is here due to a bug in -m32 code in the compiler.  
int i386Arch::frame_base_reg() {
    return 5 ;
}

int i386Arch::disassemble (PStream &os, Process *proc, Address addr) {
    // mark current address
    Address pc = proc->get_reg("pc") ;
    if (pc == addr) {
        os.print ("* ") ;
    } else {
        os.print ("  ") ;
    }
    os.print ("0x%llx", addr) ;
    Location loc = proc->lookup_address (addr) ;
    static LocalMap locals ;
    static DIE *lastdie ;
    if (loc.get_symname() != "") {
        char offset_str[32] ;
        sprintf(offset_str, "%d", loc.get_offset()) ; 

        if (loc.get_offset() == 0) {
            os.print (" <%s>:        ", loc.get_symname().c_str()) ;
        } else {
            os.print (" <%s+", loc.get_symname().c_str(), loc.get_offset()) ;
            os.print ("%s>: ", offset_str) ;
            for (int i=0; i < 6-(int)strlen(offset_str); i++) {
                os.print (" ");
            }
        }
        if (loc.get_subp_die() != lastdie) {
            locals.clear() ;
            proc->build_local_map (loc.get_subp_die(), locals) ;
        }
        lastdie = loc.get_subp_die();
    } else {
        os.print (": ") ;
    }
    unsigned char buffer[16] ;          // max of 15 bytes in an instruction
    int64_t *p = reinterpret_cast<int64_t*>(buffer) ;
    *p++ = proc->read (addr, 8) ;
    *p++ = proc->read (addr+8, 8) ;

    return disassembler->disassemble (proc, os, addr, buffer, locals) ;
}

RegisterType i386Arch::get_register_type (std::string reg) {
    if (reg == "pc" || reg == "eip" || reg == "fp" || reg == "ebp" ||
         reg == "sp" || reg == "esp") {
        return RT_ADDRESS ;
    }
    return RT_INTEGRAL ;
}

std::string i386Arch::get_return_reg(int n) {
    if (n != 1) {
        throw Exception ("Bad return register for i386") ;
    }
    return "eax" ;
}

std::string i386Arch::get_return_fpreg(int n) {
    if (n != 1) {
        throw Exception ("Bad return register for i386") ;
    }
    return "st0" ;
}

// for the 32 bit ABI, all arguments are pushed onto the stack
void i386Arch::write_call_arg (Process *proc, int argnum, Address value, bool isfp) {
    Address sp = proc->get_reg ("sp") ;
    sp -= 4 ;
    proc->write (sp, value, 4) ;
    proc->set_reg ("sp", sp) ;
}

// pass an arbitrary arg of known size
void i386Arch::write_call_arg (Process *proc, int argnum, const void *value, int size) {
    Address sp = proc->get_reg ("sp") ;
    size = (size + 3) &~3 ;                     // align to 4 byte size
    sp -= size ;

    // write the data, 4 bytes at a time
    const int *cvalue = (const int*)value ;
    for (int i = 0 ; i < size ; i+=4) {
        proc->write (sp+i, *cvalue, 4) ;
        cvalue += 1 ;
    }
    proc->set_reg ("sp", sp) ;
}

// write a call instruction into the start of 'main'.  The address to call is passed
// write the contents of the memory overwritten into the buffer
// the sequence of instructions inserted is:
//    call *addr 
//    int3
//   addr:
//    <64 bit address>
// The debugger resumes execution at the call instruction and gains control again at the
// int3 (via a SIGTRAP)

Address i386Arch::write_call (Process *proc, Address addr, std::string &buffer) {
    Address pc = proc->lookup_symbol ("main") ;
    pc += 1 ;
    buffer = proc->read_string (pc, 16) ;                // read the current contents
    proc->write (pc+0, 0xff, 1) ;                 // group 5 call with modrm 
    proc->write (pc+1, 0x15, 1) ;               // mod:0 reg: 2 rm: 5
    proc->write (pc+2, pc+7, 4) ;                    // address of address
    proc->write (pc+6, 0xcc, 1) ;                      // int3
    proc->write (pc+7, addr, 4) ;
    return pc ;
}

bool i386Arch::in_sigtramp (Process *proc, std::string name) {
    return name == "__restore_rt" ||
           name == "__restore";
}

// the sigcontext (see <bits/sigcontext.h>) is at sp + 40

// this array contains the offsets of the registers inside the destination array.
// the index into this array is the index into the sigcontext structure.
int i386_sigcontext_regs[] = {
    10,         // gs
    9,         // fs
    8,         // es
    7,         // ds
    4,         // edi
    3,         // esi
    5,          // ebp
    15,          // esp
    0,         // ebx
    2,         // edx
    1,         // ecx
    6,         // eax
    -2,         // not used
    -2,         // not used
    12,         // eip
    13,         // cs
    14,         // eflags
    -1
} ;

void i386Arch::get_sigcontext_frame (Process *proc, Address sp, unsigned char *regs) {
    Address ctx = sp+ 40 ;
    //proc->dump (ctx, sizeof (i386_sigcontext_regs) * 4) ;
    for (int i = 0 ; i386_sigcontext_regs[i] != -1 ; i++) {
        if (i386_sigcontext_regs[i] == -2) {
            continue ;
        }
        int v = proc->read (ctx + i*4, 4) ;
        memcpy (regs + i386_sigcontext_regs[i]*4, &v, 4) ;
    }
}

void i386Arch::get_fpregs (void *agent, void * tid, int pid, Target *target, unsigned char *regs) {
    if (agent != NULL && tid != NULL) {
        thread_db::read_thread_fpxregisters (reinterpret_cast<td_thragent_t*>(agent), tid, regs) ;
    } else {
        target->get_fpxregs (pid, regs) ;
    }
}

void i386Arch::set_fpregs (void *agent, void * tid, int pid, Target *target, unsigned char *regs) {
    if (agent != NULL && tid != NULL) {
        thread_db::write_thread_fpxregisters (reinterpret_cast<td_thragent_t*>(agent), tid, regs) ;
    } else {
        target->set_fpxregs (pid, regs) ;
    }
}

int i386Arch::classify_struct (EvalContext &ctx, DIE *s) {
    return X8664_AC_MEMORY;
}

bool i386Arch::is_call(Process * proc, Address addr) {
    int opcode = proc->read (addr, 4) ;
    if ((opcode & 0xff) == 0xe8) {
       return true ;
    }
    if ((opcode & 0xff) == 0xff) {      // group 5 calls
        int modrm = (opcode & 0xff00) >> 8 ;
        int reg = (modrm >> 3) & 7 ;
        if (reg == 2 || reg == 3) {
            return true ;
        }
    }
    // any other possibilities?
    return false ;
}

int i386Arch::call_size(Process * proc, Address addr) {
    int opcode = proc->read (addr, 4) ;// read 4 bytes
    if ((opcode & 0xff) == 0xe8) {
       return 5 ;
    }
    if ((opcode & 0xff) == 0xff) {      // group 5 calls
        int modrm = (opcode & 0xff00) >> 8 ;
        int mod = (modrm >> 6) & 3 ;
        int rm = modrm & 7 ;
        if (mod == 3) {                 // direct register
           return 2 ;
        }
        switch (mod) {
        case 0: 
            if (rm == 5) {              // disp32 follows
                return 6 ;
            }
            if (rm == 4) {              // SIB follows
                return 3 ;
            }
            return 2 ;
            break ;
        case 1:                         // disp8 follows
            if (rm == 4) {              // SIB
                return 4 ;
            }
            return 3 ;
            break ;
        case 2:                         // disp32 follows
            if (rm == 4) {              // SIB
                return 7 ;
            }
            return 6 ;
            break ;
        }
    }
    throw Exception ("Unable to determine size of call instruction") ;
}


//
// AMD x86_64 architecture
//

static const char *x86_64_32_regnames[] = {
    "r15d",
    "r14d",
    "r13d",
    "r12d",
    "ebp",
    "ebx",
    "r11d",
    "r10d",
    "r9d",
    "r8d",
    "eax",
    "ecx",
    "edx",
    "esi",
    "edi",
    "orig_eax",         // orig_eax
    "eip",
    "cs",
    "eflags",
    "esp",
    "ss",
    "fs_base",         // fs_base
    "gs_base",         // 
    "ds",
    "es",
    "fs",
    "gs"
} ;

static const char *x86_64_64_regnames[] = {
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
    "orig_rax",         // orig_eax
    "rip",
    "cs",
    "eflags",
    "rsp",
    "ss",
    "fs_base",         // fs_base
    "gs_base",         // 
    "ds",
    "es",
    "fs",
    "gs"
} ;

x86_64Arch::x86_64Arch (int mode) : IntelArch (4), mode(mode)
 {
    // in 32 bit mode, the reg names are the 32 bit ones
    const char **r = mode == 32 ? x86_64_32_regnames : x86_64_64_regnames ;

    regnames[*r++] = 0 * sizeof (long int) ;
    regnames[*r++] = 1 * sizeof (long int) ;
    regnames[*r++] = 2 * sizeof (long int) ;
    regnames[*r++] = 3 * sizeof (long int) ;
    regnames[*r++] = 4 * sizeof (long int) ;
    regnames[*r++] = 5 * sizeof (long int) ;
    regnames[*r++] = 6 * sizeof (long int) ;
    regnames[*r++] = 7 * sizeof (long int) ;
    regnames[*r++] = 8 * sizeof (long int) ;
    regnames[*r++] = 9 * sizeof (long int) ;
    regnames[*r++] = 10 * sizeof (long int) ;
    regnames[*r++] = 11 * sizeof (long int) ;
    regnames[*r++] = 12 * sizeof (long int) ;
    regnames[*r++] = 13 * sizeof (long int) ;
    regnames[*r++] = 14 * sizeof (long int) ;
    /*regnames[*r++] = 15 * sizeof (long int) ; */ r++ ;        // orig_rax
    regnames[*r++] = 16 * sizeof (long int) ;
    regnames[*r++] = 17 * sizeof (long int) ;
    regnames[*r++] = 18 * sizeof (long int) ;
    regnames[*r++] = 19 * sizeof (long int) ;
    regnames[*r++] = 20 * sizeof (long int) ;
    /*regnames[*r++] = 21 * sizeof (long int) ;*/  r++ ;        // fs_base
    /*regnames[*r++] = 22 * sizeof (long int) ;*/  r++ ;        // gs_base
    regnames[*r++] = 23 * sizeof (long int) ;
    regnames[*r++] = 24 * sizeof (long int) ;
    regnames[*r++] = 25 * sizeof (long int) ;
    regnames[*r++] = 26 * sizeof (long int) ;
                                                                                                                                         
    regnames["pc"] = 16 * sizeof (long int) ;                 // alias for rip
    regnames["sp"] = 19 * sizeof (long int) ;                 // alias for rsp
    regnames["fp"] = 4  * sizeof (long int) ;                 // alias for rbp

    const int st_start = sizeof (unsigned short) * 4 + sizeof(long) * 2 + sizeof(int) * 2 ;

    regnames["st0"] = st_start + 0 * 16 ;
    regnames["st1"] = st_start + 1 * 16 ;
    regnames["st2"] = st_start + 2 * 16 ;
    regnames["st3"] = st_start + 3 * 16 ;
    regnames["st4"] = st_start + 4 * 16 ;
    regnames["st5"] = st_start + 5 * 16 ;
    regnames["st6"] = st_start + 6 * 16 ;
    regnames["st7"] = st_start + 7 * 16 ;

    // MMX registers are the same as the x87 registers
    regnames["mm0"] = st_start + 0 * 16 ;
    regnames["mm1"] = st_start + 1 * 16 ;
    regnames["mm2"] = st_start + 2 * 16 ;
    regnames["mm3"] = st_start + 3 * 16 ;
    regnames["mm4"] = st_start + 4 * 16 ;
    regnames["mm5"] = st_start + 5 * 16 ;
    regnames["mm6"] = st_start + 6 * 16 ;
    regnames["mm7"] = st_start + 7 * 16 ;


    // there are 8 SSE registers
    const int sse_start = st_start + sizeof(int) * 32 ;

    regnames["xmm0"] = sse_start + 0 * 16 ;
    regnames["xmm1"] = sse_start + 1 * 16 ;
    regnames["xmm2"] = sse_start + 2 * 16 ;
    regnames["xmm3"] = sse_start + 3 * 16 ;
    regnames["xmm4"] = sse_start + 4 * 16 ;
    regnames["xmm5"] = sse_start + 5 * 16 ;
    regnames["xmm6"] = sse_start + 6 * 16 ;
    regnames["xmm7"] = sse_start + 7 * 16 ;
/* these appear not to be in the structures
    regnames["xmm8"] = sse_start + 0 * 16 ;
    regnames["xmm9"] = sse_start + 1 * 16 ;
    regnames["xmm10"] = sse_start + 2 * 16 ;
    regnames["xmm11"] = sse_start + 3 * 16 ;
    regnames["xmm12"] = sse_start + 4 * 16 ;
    regnames["xmm13"] = sse_start + 5 * 16 ;
    regnames["xmm14"] = sse_start + 6 * 16 ;
    regnames["xmm15"] = sse_start + 7 * 16 ;
*/

    // store the register names
    for (RegMap::iterator reg = regnames.begin() ; reg != regnames.end() ; reg++) {
        allnames.push_back (reg->first) ;
    }

    if (mode == 32) {
        commonnames.push_back ("eax") ;
        commonnames.push_back ("ebx") ;
        commonnames.push_back ("ecx") ;
        commonnames.push_back ("edx") ;
        commonnames.push_back ("esi") ;
        commonnames.push_back ("edi") ;
        commonnames.push_back ("ebp") ;
        commonnames.push_back ("esp") ;
        commonnames.push_back ("eip") ;
        commonnames.push_back ("eflags") ;
        commonnames.push_back ("ds") ;
        commonnames.push_back ("es") ;
        commonnames.push_back ("fs") ;
        commonnames.push_back ("gs") ;
    } else {
        commonnames.push_back ("rax") ;
        commonnames.push_back ("rbx") ;
        commonnames.push_back ("rcx") ;
        commonnames.push_back ("rdx") ;
        commonnames.push_back ("rsi") ;
        commonnames.push_back ("rdi") ;
        commonnames.push_back ("rbp") ;
        commonnames.push_back ("rsp") ;
        commonnames.push_back ("r8") ;
        commonnames.push_back ("r9") ;
        commonnames.push_back ("r10") ;
        commonnames.push_back ("r11") ;
        commonnames.push_back ("r12") ;
        commonnames.push_back ("r13") ;
        commonnames.push_back ("r14") ;
        commonnames.push_back ("r15") ;
        commonnames.push_back ("rip") ;
        commonnames.push_back ("eflags") ;
        commonnames.push_back ("ds") ;
        commonnames.push_back ("es") ;
        commonnames.push_back ("fs") ;
        commonnames.push_back ("gs") ;
    }


    if (mode == 64) {
        // DWARF mapping.  see ABI page 37
        // Note: the version I have says that reg 1 is %rbx and reg 3 is %rdx.  Apparently this is
        // wrong and it should be the other way around.
        regnums[0] = 10 * sizeof (long int) ;           // rax
        regnums[1] = 12 * sizeof (long int) ;            // rdx
        regnums[2] = 11 * sizeof (long int) ;           // rcx
        regnums[3] = 5 * sizeof (long int) ;           // rbx
        regnums[4] = 13 * sizeof (long int) ;           // rsi
        regnums[5] = 14 * sizeof (long int) ;           // rdi
        regnums[6] = 4 * sizeof (long int) ;            // rbp
        regnums[7] = 19 * sizeof (long int) ;           // rsp
        regnums[8] = 9 * sizeof (long int) ;            // r8
        regnums[9] = 8 * sizeof (long int) ;            // r9
        regnums[10] = 7 * sizeof (long int) ;            // r10
        regnums[11] = 6 * sizeof (long int) ;            // r11
        regnums[12] = 3 * sizeof (long int) ;            // r12
        regnums[13] = 2 * sizeof (long int) ;            // r13
        regnums[14] = 1 * sizeof (long int) ;            // r14
        regnums[15] = 0 * sizeof (long int) ;            // r15
        regnums[16] = 16 * sizeof (long int) ;            // rip

        // floating point registers
        for (int i = 33 ; i < 41 ; i++) {
           regnums[i] = st_start + (i-33) * 16 ;
        }

        // SSE registers
        for (int i = 17 ; i < 25 ; i++) {
           regnums[i] = sse_start + (i-17) * 16 ;
        }

        // MMX registers (same as floating point registers)
        for (int i = 41 ; i < 49 ; i++) {
           regnums[i] = st_start + (i-41) * 16 ;
        }
    } else {
        regnums[0] = 10 * sizeof (long int) ;           // eax
        regnums[1] = 11 * sizeof (long int) ;           // ecx
        regnums[2] = 12 * sizeof (long int) ;           // edx
        regnums[3] = 5 * sizeof (long int) ;            // ebx
        regnums[4] = 19 * sizeof (long int) ;           // esp
        regnums[5] = 4 * sizeof (long int) ;            // ebp
        regnums[6] = 13 * sizeof (long int) ;           // esi
        regnums[7] = 14 * sizeof (long int) ;           // edi
        regnums[8] = 16 * sizeof (long int) ;           // eip          
        regnums[9] = 18 * sizeof (long int) ;           // eflags

        // floating point registers
        for (int i = 11 ; i < 19 ; i++) {
           regnums[i] = st_start + (i-11) * 16 ;
        }

        // SSE registers
        for (int i = 21 ; i < 29 ; i++) {
           regnums[i] = sse_start + (i-21) * 16 ;
        }

        // MMX registers (same as floating point registers)
        for (int i = 29 ; i < 37 ; i++) {
           regnums[i] = st_start + (i-29) * 16 ;
        }
    }

    // XXX: and others

    disassembler = new OpteronDisassembler(mode == 64) ;
}

x86_64Arch::~x86_64Arch() {
}

RegnameVec &x86_64Arch::get_regnames(bool all) {
    if (all) {
        return allnames ;
    } else {    
        return commonnames ;
    }
}

int x86_64Arch::bpsize() {
    return 1 ;
}

int x86_64Arch::get_regbuffer_size() {
    return sizeof (struct user_regs_struct) ;
}

int x86_64Arch::get_fpregbuffer_size() {
#if __WORDSIZE == 64
    return sizeof (struct user_fpregs_struct) ;
#else
    return sizeof (struct user_fpxregs_struct) ;
#endif
}

int x86_64Arch::get_num_regs() {
    return sizeof (struct user_regs_struct) / sizeof(long int) ;
}

int x86_64Arch::get_num_fpregs() {
    return 16 ;
}

int x86_64Arch::get_reg_size() {
    return 8 ;
}

int x86_64Arch::get_fpreg_size() {
    return 16 ; 
}

int x86_64Arch::regnum2offset(int rnum) {
    return rnum * 8 ;
}

int x86_64Arch::fpregnum2offset(int rnum) {
    return  sizeof (unsigned short) * 4 + sizeof(long) * 2 + sizeof(int) * 2 + sizeof(int) * 32 + rnum * 16 ;
}

bool x86_64Arch::isfpreg(int dwarfnum) {
    return dwarfnum > 16 ;
}

int x86_64Arch::translate_regname(std::string name) {
    RegMap::iterator num = regnames.find (name) ;
    if (num == regnames.end()) {
        throw Exception ("No such register ");
    }
    return num->second ;
}

std::string x86_64Arch::reverse_translate_regnum(int num) {
   for (RegMap::iterator i = regnames.begin() ; i != regnames.end(); i++) {
       if (i->second == num) {
           return "%" + i->first  ;
       }
   }
   return "%unknown" ;
}

int x86_64Arch::translate_regnum(int dwarfnum) {
    RegnumMap::iterator i = regnums.find (dwarfnum) ;
    if (i == regnums.end()) {
        throw Exception ("No such DWARF register ") ;
    }
    return i->second ;
}

int x86_64Arch::translate_fpregname(std::string name) {
    throw Exception("not reached");
}

int x86_64Arch::set_breakpoint(Process * proc, Address addr) {
    int oldvalue = proc->raw_read (addr, 4) ;
    proc->write (addr, (oldvalue & ~0xff) | 0xcc, 4) ; /* XXX: broken should do one byte */
    return oldvalue ;
}

bool x86_64Arch::breakpoint_present(Process * proc, Address addr) {
    int v = proc->raw_read (addr, 1) ;
    if (v == 0xcc) {
        return true ;
    }
    return false ;
}

int x86_64Arch::ptrsize() {
    return mode == 32 ? 4 : 8 ;
}

static bool is64bitframeinst (int opcode) {
    if ((opcode & 0xff) == 0x55) {        // push %ebp
        return true ;
    } else if  ((opcode & 0xffffff) == 0xe58948) {       // mov %esp, %ebp
        return true ;
    }
    return false ;
}

bool x86_64Arch::inframemaker(Process * proc, Address pc) {
    int opcode = proc->raw_read (pc, 4) ;// read 4 bytes
    if ((opcode & 0xff) == 0xcc) {   // breakpoint?
        typedef std::list<Breakpoint*> BreakpointList ;
        BreakpointList *bps = proc->find_breakpoint (pc) ;
        for (BreakpointList::iterator i = bps->begin() ; i != bps->end() ; i++) {
            Breakpoint *bp = *i ;
            if (bp->is_software()) {
                SoftwareBreakpoint *abp = dynamic_cast<SoftwareBreakpoint*>(bp) ;
                int oldvalue = abp->get_old_value() ;
                if (is64bitframeinst (oldvalue)) {
                    return true ;
                }
            }
        }
        return false ;
    }
    return isframeinst (opcode) ;
}

Address x86_64Arch::skip_preamble (Process *proc, Address addr) {
    while (inframemaker (proc, addr)) {
        addr++ ;
    }
    return addr ;
}

// this is here due to a bug in -m32 code in the compiler.  
int x86_64Arch::frame_base_reg() {
    return 5 ;
}

int x86_64Arch::disassemble (PStream &os, Process *proc, Address addr) {
    // mark current address
    Address pc = proc->get_reg("pc") ;
    if (pc == addr) {
        os.print ("* ") ;
    } else {
        os.print ("  ") ;
    }
    os.print ("0x%llx", addr) ;
    Location loc = proc->lookup_address (addr) ;
    static LocalMap locals ;
    static DIE *lastdie ;
    if (loc.get_symname() != "") {
        char offset_str[32] ;
        sprintf(offset_str, "%d", loc.get_offset()) ; 

        if (loc.get_offset() == 0) {
            os.print (" <%s>:        ", loc.get_symname().c_str()) ;
        } else {
            os.print (" <%s+", loc.get_symname().c_str(), loc.get_offset()) ;
            os.print ("%s>: ", offset_str) ;
            for (int i=0; i < 6-(int)strlen(offset_str); i++) {
                os.print (" ");
            }
        }
        if (loc.get_funcloc() != NULL) {
            if (loc.get_subp_die() != lastdie) {
                locals.clear() ;
                proc->build_local_map (loc.get_subp_die(), locals) ;
            }
            lastdie = loc.get_subp_die();
       }
    } else {
        os.print (": ") ;
    }
    unsigned char buffer[16] ;          // max of 15 bytes in an instruction
    int64_t *p = reinterpret_cast<int64_t*>(buffer) ;
    *p++ = proc->read (addr, 8) ;
    *p++ = proc->read (addr+8, 8) ;

    return disassembler->disassemble (proc, os, addr, buffer, locals) ;
}

RegisterType x86_64Arch::get_register_type (std::string reg) {
    if (reg == "pc" || reg == "rip" || reg == "fp" || reg == "rbp" ||
         reg == "sp" || reg == "rsp" || reg == "eip" || reg == "ebp" ||
         reg == "esp") {
        return RT_ADDRESS ;
    }
    return RT_INTEGRAL ;
}


std::string x86_64Arch::get_return_reg(int n) {
    if (mode == 32) {
        if (n == 1) {
            return "eax" ;
        }
        return "edx" ;
    } else {
        if (n == 1) {
            return "rax" ;
        }
        return "rdx" ;
    }
}

std::string x86_64Arch::get_return_fpreg(int n) {
    return "xmm0" ;
}

// From the Sys V ABI page 17:
// Args are passed in registers in the order: %rdi, %rsi, %rdx, %rcx, %r8, %r9
// any further args are passed on the stack
// this must be called with the last argument first as this will be pushed onto
// the stack immediately

static const char *argregs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9"} ;
static const char *fpargregs[] = { "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"} ;

void x86_64Arch::write_call_arg (Process *proc, int argnum, Address value, bool isfp) {
    if (mode == 32) {
        Address sp = proc->get_reg ("sp") ;
        sp -= 4 ;
        proc->write (sp, value, 4) ;
        proc->set_reg ("sp", sp) ;
    } else {
        if (isfp) {
            if (argnum > 7) {
                Address sp = proc->get_reg ("sp") ;
                sp -= 8 ;
                proc->write (sp, value, 8) ;
                proc->set_reg ("sp", sp) ;
            } else {
                proc->set_fpreg (fpargregs[argnum], value) ;
            }
        } else {
            if (argnum > 5) {           // push onto stack?
                Address sp = proc->get_reg ("sp") ;
                sp -= 8 ;
                proc->write (sp, value, 8) ;
                proc->set_reg ("sp", sp) ;
            } else {
                proc->set_reg (argregs[argnum], value) ;
            }
        }
    }
}

// pass an arbitrary arg of known size

void x86_64Arch::write_call_arg (Process *proc, int argnum, const void *value, int size) {
    Address sp = proc->get_reg ("sp") ;
    if (mode == 32) {
        size = (size + 3) &~3 ;                     // align to 4 byte size
        sp -= size ;

        // write the data, 4 bytes at a time
        const int *cvalue = (const int*)value ;
        for (int i = 0 ; i < size ; i+=4) {
            proc->write (sp+i, *cvalue, 4) ;
            cvalue += 1 ;
        }
    } else {
        size = (size + 7) &~7 ;                     // align to 8 byte size
        sp -= size ;

        // write the data, 8 bytes at a time
        const int64_t *cvalue = (const int64_t*)value ;
        for (int i = 0 ; i < size ; i+=8) {
            proc->write (sp+i, *cvalue, 8) ;
            cvalue += 1 ;
        }
    }
    proc->set_reg ("sp", sp) ;
}

// write a call instruction into the start of 'main'.  The address to call is passed
// write the contents of the memory overwritten into the buffer
// the sequence of instructions inserted is:
//    call 1(%rip)
//    int3
//    <64 bit address>
// The debugger resumes execution at the call instruction and gains control again at the
// int3 (via a SIGTRAP)

// see the i386Arch::write_call() function for the 32 bit code

Address x86_64Arch::write_call (Process *proc, Address addr, std::string &buffer) {
    Address pc = proc->lookup_symbol ("main") ;
    pc += 1 ;
    buffer = proc->read_string (pc, 16) ;                // read the current contents
    if (mode == 32) {
        proc->write (pc+0, 0xff, 1) ;                 // group 5 call with modrm 
        proc->write (pc+1, 0x15, 1) ;               // mod:0 reg: 2 rm: 5
        proc->write (pc+2, pc+7, 4) ;                    // address of address
        proc->write (pc+6, 0xcc, 1) ;                      // int3
        proc->write (pc+7, addr, 8) ;
    } else {
        proc->write (pc, 0xff, 1) ;                 // group 5 call with modrm 
        proc->write (pc+1, 0x15, 1) ;               // mod:0 reg: 2 rm: 5
        proc->write (pc+2, 1, 4) ;                    // disp32 = 1
        proc->write (pc+6, 0xcc, 1) ;                      // int3
        proc->write (pc+7, addr, 8) ;
    }
    return pc ;
}

#if 0
// this is the hard way to do it.  Check if the pc points to an instruction
// sequence.  This does actually happen.  Here's a real case:

/*
pathdb> where
#0  0x00000000004006c3 in signalhandler (sig=10) at t.c:11
#1  0x0000002a9579dd40 in __restore_rt ()

pathdb> disass
0x2a9579dd40:  mov 0xf, %rax
0x2a9579dd47:  syscall 

pathdb> x/10xb 0x2a9579dd40
0x9579dd40:   0x48  0xc7  0xc0  0xf  0x0  0x0  0x0  0xf
0x9579dd48:   0x5  0x66
*/

static unsigned char x86_64_sigtramp[] = {
    0x48, 0xc7, 0xc0, 0x0f, 0x00, 0x00, 0x00,           // mov 0xf, %rax
    0x0f, 0x05                                          // syscall
} ;

bool x86_64Arch::in_sigtramp (Process *proc, std::string name) {
    Address pc = proc->get_reg ("pc") ;
    for (int i = 0 ; i < sizeof (x86_64_sigtramp) ; i++) {
        unsigned char byte = proc->read (pc+i, 1) ;
        if (byte != x86_64_sigtramp[i]) {
            return false ;
        }
    }
    return true ;
}
#else

// for linux the name of the signal return is always the same.
bool x86_64Arch::in_sigtramp (Process *proc, std::string name) {
    return name == "__restore_rt" ||
           name == "__restore";
}

#endif

// the sigcontext (see <bits/sigcontext.h>) is at sp + 40

// this array contains the offsets of the registers inside the destination array.
// the index into this array is the index into the sigcontext structure.
int x86_64_sigcontext_regs[] = {
    9,          // r8
    8,          // r9
    7,          // r10
    6,          // r11
    3,          // r12
    2,          // r13
    1,          // r14
    0,          // r15
    14,         // rdi
    13,         // rsi
    4,          // rbp
    5,          // rbx
    12,         // rdx
    10,         // rax
    11,         // rcx
    19,         // rsp
    16,         // rip
    18,         // eflags
    17,         // cs
    26,         // gs
    25,         // fs
    -1
} ;

void x86_64Arch::get_sigcontext_frame (Process *proc, Address sp, unsigned char *regs) {
    Address ctx = sp+ 40 ;
    //proc->dump (ctx, sizeof (x86_64_sigcontext_regs) * sizeof(long)) ;
    for (int i = 0 ; x86_64_sigcontext_regs[i] != -1 ; i++) {
        Address v = proc->read (ctx + i*sizeof(long), 8) ;
        memcpy (regs + x86_64_sigcontext_regs[i]*sizeof(long), &v, 8) ;
    }
}

void x86_64Arch::get_fpregs (void *agent, void * tid, int pid, Target *target, unsigned char *regs) {
    if (agent != NULL && tid != NULL) {
        thread_db::read_thread_fpregisters (reinterpret_cast<td_thragent_t*>(agent), tid, regs) ;
    } else {
        target->get_fpregs (pid, regs) ;
    }
}


void x86_64Arch::set_fpregs (void *agent, void * tid, int pid, Target *target, unsigned char *regs) {
    if (agent != NULL && tid != NULL) {
        thread_db::write_thread_fpregisters (reinterpret_cast<td_thragent_t*>(agent), tid, regs) ;
    } else {
        target->set_fpregs (pid, regs) ;
    }
}

// classify the small struct according to the rules on the ABI (page 16)
int x86_64Arch::classify_struct (EvalContext &ctx, DIE *s) {
    if (mode == 32) {
        return X8664_AC_MEMORY ;
    }
    if (s->get_real_size(ctx) > 16) {
        return X8664_AC_MEMORY ;
    }
    x86_64ArgClass cls1 = X8664_AC_NO_CLASS ;
    x86_64ArgClass cls2 = X8664_AC_NO_CLASS ;
 
    int offset = 0 ;
    std::vector<DIE*> &children = s->getChildren();
    for (int i = 0 ; i < (int)children.size() ; i++) {
        x86_64ArgClass *cls = offset < 8 ? &cls1 : &cls2 ;
        DIE *child = children[i];
        if (child->get_tag() == DW_TAG_member) {
            DIE *type = child->get_type() ;
            if (type->is_real()) {
                if (*cls != X8664_AC_MEMORY && *cls != X8664_AC_INTEGER) {
                    *cls = X8664_AC_SSE ;
                }
            } else if (type->is_struct()) {
                x86_64ArgClass tmp = (x86_64ArgClass)classify_struct (ctx, child) ;
                if (*cls != X8664_AC_MEMORY && *cls != X8664_AC_INTEGER) {
                    *cls = tmp ;
                }
            } else if (type->is_scalar() || type->is_array()) {
                if (*cls != X8664_AC_MEMORY) {
                    *cls = X8664_AC_INTEGER ;
                }
            } else {
                if (*cls != X8664_AC_MEMORY && *cls != X8664_AC_INTEGER) {
                    *cls = X8664_AC_SSE ;
                }
            }
            offset += type->get_real_size(ctx) ;
        }
    }
    if (offset < 8) {
        return cls1 ;
    }
    if (cls1 == cls2) {
        return cls1 ;
    }
    if (cls1 == X8664_AC_NO_CLASS) {
        return cls2 ;
    }
    if (cls1 == X8664_AC_MEMORY || cls2 == X8664_AC_MEMORY) {
        return X8664_AC_MEMORY ;
    }
    if (cls1 == X8664_AC_INTEGER || cls2 == X8664_AC_INTEGER) {
        return X8664_AC_INTEGER ;
    }
    return X8664_AC_SSE ;
}


bool x86_64Arch::is_call(Process * proc, Address addr) {
    int64_t opcode = proc->read (addr, 8) ;
    for (;;) {
        if ((opcode & 0xffLL) == 0xe8) {
           return true ;
        }
        if ((opcode & 0xffLL) == 0xff) {      // group 5 calls
            int modrm = (opcode & 0xff00LL) >> 8 ;
            int reg = (modrm >> 3) & 7 ;
            if (reg == 2 || reg == 3) {
                return true ;
            }
        }
        int op = opcode & 0xffLL ;
        // XXX: REX instruction
        if (op >= 0x40 && op <= 0x4f) {
            opcode >>= 8 ;                   // remove prefix
            continue ;
        }
        break ;
    }
    // any other possibilities?
    return false ;
}

int x86_64Arch::call_size(Process * proc, Address addr) {
    int64_t opcode = proc->read (addr, 8) ;// read 8 bytes
    int call_size = 0;
    for (;;) {
        if ((opcode & 0xff) == 0xe8) {
           call_size += 5 ;
           return call_size ;
        }
        if ((opcode & 0xff) == 0xff) {      // group 5 calls
            int modrm = (opcode & 0xff00) >> 8 ;
            int mod = (modrm >> 6) & 3 ;
            int rm = modrm & 7 ;
            if (mod == 3) {                 // direct register
               call_size += 2 ;
               return call_size ;
            }
            switch (mod) {
            case 0: 
                if (rm == 5) {              // disp32 follows
                    call_size += 6 ;
                    return call_size ;
                }
                if (rm == 4) {              // SIB follows
                    call_size += 3 ;
                    return call_size ;
                }
                call_size += 2 ;
                return call_size ;
                break ;
            case 1:                         // disp8 follows
                if (rm == 4) {              // SIB
                    call_size += 4 ;
                    return call_size ;
                }
                call_size += 3 ;
                return call_size ;
            case 2:                         // disp32 follows
                if (rm == 4) {              // SIB
                    call_size += 7 ;
                    return call_size ;
                }
                call_size += 6 ;
                return call_size ;
            }
        }
        int op = opcode & 0xffLL ;
        if (op >= 0x40 && op <= 0x4f) {
            call_size++ ;                    // prefix size
            opcode >>= 8 ;                   // remove prefix
            continue ;
        }
        break ;
    }
    throw Exception ("Unable to determine size of call instruction") ;
}


