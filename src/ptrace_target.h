#include "target.h"

/**
 * A live target that uses ptrace to control the process.  The ptrace() call
 * exists across most UNIX-like systems, but has significant variations in the
 * parameters that it accepts.  This superclass provides some generic
 * functionality that is common across systems.  Subclasses implement
 * platform-specific behaviour.
 */
class PtraceTarget : public LiveTarget
{
public:
    PtraceTarget (Architecture *arch) ;
    ~PtraceTarget() {}

    int attach (const char* prog, const char* args, EnvMap&);      // attach to a file
    int attach (std::string fn, int pid) ;                         // attach to a process
    int attach (int pid) ;                                         // attach to a process id

    void detach(int pid, bool kill = false) ;                                 // detach from target

    void interrupt(int pid) ;
    bool test_address (int pid, Address addr) ;                  // check if address is good
    Address read (int pid, Address addr, int size=4) ;           // read a number of words
    Address readptr (int pid, Address addr)  ;
    void write (int pid, Address addr, Address data, int size) ;    // write a word
    virtual void get_regs(int pid, RegisterSet *regs);               // get register set
    virtual void set_regs(int pid, RegisterSet *regs);               // set register set
    virtual void get_fpregs(int pid, RegisterSet *regs);               // get floating point register set
    virtual void set_fpregs(int pid, RegisterSet *regs);               // set floating point register set
    virtual void get_fpxregs(int pid, RegisterSet *regs);               // get floating point extended register set
    virtual void set_fpxregs(int pid, RegisterSet *regs); // set floating point extended register set
    long get_debug_reg (int pid, int reg) ;
    void set_debug_reg (int pid, int reg, long value) ;

    void step(int pid)  ;                                           // single step
    void cont (int pid, int signal)  ;                              // continue execution
    void init_events (int pid) ;
    pid_t get_fork_pid (pid_t pid) ;

protected:

    pid_t pid;
    bool is_attached ;
};
