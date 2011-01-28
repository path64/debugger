#ifdef __linux__

#include "trace.h"
#include <endian.h>
#include <sys/user.h>

/* find the offset of X into struct user (from sys/user.h) */
#define STRUCT_USER_OFFSET(X) (&(((struct user*)0)->X))

int Trace::get_regs (pid_t pid, void *regs) {
    return int (ptrace (PTRACE_GETREGS, pid, 0, regs)) ;
}

int Trace::get_fpregs (pid_t pid, void *fpregs) {
    return int (ptrace (PTRACE_GETFPREGS, pid, 0, fpregs)) ;
}

int Trace::get_fpxregs (pid_t pid, void *fpxregs) {
    return int (ptrace (PTRACE_GETFPXREGS, pid, 0, fpxregs)) ;
}

unsigned long Trace::get_dbgreg (pid_t pid, int idx) {
    /* find offset and access struct user */
    void *addr = STRUCT_USER_OFFSET (u_debugreg[idx]) ;
    return ptrace (PTRACE_PEEKUSER, pid, addr, 0) ;
}

int Trace::set_regs (pid_t pid, void *regs) {
    return int (ptrace (PTRACE_SETREGS, pid, 0, regs)) ;
}

int Trace::set_fpregs (pid_t pid, void *fpregs) {
    return int (ptrace (PTRACE_SETFPREGS, pid, 0, fpregs)) ;
}

int Trace::set_fpxregs (pid_t pid, void *fpxregs) {
    return int (ptrace (PTRACE_SETFPXREGS, pid, 0, fpxregs)) ;
}

int Trace::set_dbgreg (pid_t pid, int idx, unsigned long val) {
    /* find offset and access struct user */
    void *addr = STRUCT_USER_OFFSET (u_debugreg[idx]) ;
    return ptrace (PTRACE_POKEUSER, pid, addr, val) ;
}

unsigned long Trace::read_data (pid_t pid, void *addr) {
    return ptrace (PTRACE_PEEKDATA, pid, addr, 0) ;
}

int Trace::write_data (pid_t pid, void *addr, unsigned long data) {
    return ptrace (PTRACE_POKEDATA, pid, addr, (void *)data) ;
}

int Trace::set_options (pid_t pid, long opts) {
    return ptrace(PTRACE_SETOPTIONS, pid, (void*)0, opts) ;
}

int Trace::get_fork_pid (pid_t parent_pid, pid_t *fork_pid) {
	long opt = 0;
	int ret = ptrace (PTRACE_GETEVENTMSG, parent_pid, (void*)0, &opt);
	*fork_pid = (pid_t)opt;
	return  ret;
}

int Trace::get_thread_area (pid_t pid, int idx, void *dst) {
#define PTRACE_GET_THREAD_AREA (__ptrace_request)25
    return ptrace (PTRACE_GET_THREAD_AREA, pid, (void *)idx, dst) ;
}

#endif
