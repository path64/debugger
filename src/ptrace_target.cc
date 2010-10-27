#include <sys/ptrace.h>

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
		   // FIXME: Throwing these generic exceptions is silly.  We can't
		   // catch them and find anything useful from them.
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
