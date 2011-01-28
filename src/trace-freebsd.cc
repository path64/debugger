#ifdef __FreeBSD__

#include "trace.h"
#include <sys/endian.h>
#include <machine/reg.h>
#include <machine/sysarch.h>

int Trace::get_regs (pid_t pid, void *regs) {
    return ptrace (PT_GETREGS, pid, (caddr_t)regs, 0) ;
}

int Trace::get_fpregs (pid_t pid, void *fpregs) {
    return ptrace (PT_GETFPREGS, pid, (caddr_t)fpregs, 0) ;
}

int Trace::get_fpxregs (pid_t pid, void *fpxregs) {
    return get_fpregs (pid, fpxregs) ; // XXX: this right?
}

unsigned long Trace::get_dbgreg (pid_t pid, int idx) {
    struct dbreg regs ;
    ptrace (PT_GETDBREGS, pid, (caddr_t)&regs, 0) ;
    return regs.dr[idx] ;
}

int Trace::set_regs (pid_t pid, void *regs) {
    return ptrace (PT_SETREGS, pid, (caddr_t)regs, 0) ;
}

int Trace::set_fpregs (pid_t pid, void *fpregs) {
    return ptrace (PT_SETFPREGS, pid, (caddr_t)fpregs, 0) ;
}

int Trace::set_fpxregs (pid_t pid, void *fpxregs) {
    return set_fpregs (pid, fpxregs) ; // XXX: this right?
}

int Trace::set_dbgreg (pid_t pid, int idx, unsigned long val) {
    struct dbreg regs ;
    ptrace (PT_GETDBREGS, pid, (caddr_t)&regs, 0) ;
    regs.dr[idx] = val ;
    return ptrace (PT_SETDBREGS, pid, (caddr_t)&regs, 0) ;
}

unsigned long Trace::read_data (pid_t pid, void *addr) {
    unsigned long read_word = 0 ;
    int n_reads = int(sizeof (long) / sizeof (int)) ;

    for (int i = 0 ; i < n_reads ; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
        void *int_addr = (int *)addr + i ;
#else
        void *int_addr = (int *)addr + (n_reads - 1 - i) ;
#endif
        unsigned int read_int = (unsigned int)ptrace (PT_READ_D, pid, (caddr_t)int_addr, 0) ;
        read_word |= ((unsigned long)read_int << (i * 32)) ;
    }

    return read_word ;
}

int Trace::write_data (pid_t pid, void *addr, unsigned long data) {
    int ret_val = 0 ;
    int n_writes = int(sizeof (long) / sizeof (int)) ;

    for (int i = 0 ; i < n_writes ; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
        void *int_addr = (int *)addr + i ;
#else
        void *int_addr = (int *)addr + (n_writes - 1 - i) ;
#endif
        unsigned int write_int = (unsigned int)(data >> (i * 32)) ;
        ret_val |= ptrace (PT_WRITE_D, pid, (caddr_t)int_addr, write_int) ;
    }

    return ret_val ;
}

int Trace::set_options (pid_t pid, long opts) {
    return -1 ;
}

int Trace::get_fork_pid (pid_t parent_pid, pid_t *fork_pid) {
    return -1 ;
}

int Trace::get_thread_area (pid_t pid, int idx, void *dst) {
    return -1 ;
}

int Trace::suspend (void *tid) {
    return (int)ptrace (PT_SUSPEND, (intptr_t)tid, 0, 0) ;
}

#endif
