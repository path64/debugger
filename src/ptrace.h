
#ifndef _PTRACE_H_
#define _PTRACE_H_

#include <sys/ptrace.h>
#include "config.h"


#ifndef HAVE_PTRACE_O_CONSTANTS
#define PTRACE_SETOPTIONS (__ptrace_request)0x4200
#define PTRACE_O_TRACESYSGOOD 0x00000001
#define PTRACE_O_TRACEFORK 0x00000002
#define PTRACE_O_TRACEVFORK 0x00000004
#define PTRACE_O_TRACECLONE 0x00000008
#define PTRACE_O_TRACEEXEC 0x00000010
#define PTRACE_O_TRACEVFORKDONE 0x00000020
#define PTRACE_O_TRACEEXIT 0x00000040
#define PTRACE_O_MASK 0x0000007f
#endif // !HAVE_PTRACE_O_CONSTANTS


#ifndef HAVE_PTRACE_EVENT_CONSTANTS
#define PTRACE_GETEVENTMSG (__ptrace_request)0x4201
#define PTRACE_EVENT_FORK 1
#define PTRACE_EVENT_VFORK 2
#define PTRACE_EVENT_CLONE 3
#define PTRACE_EVENT_EXEC 4
#define PTRACE_EVENT_VFORK_DONE 5
#define PTRACE_EVENT_EXIT 6
#endif // !HAVE_PTRACE_EVENT_CONSTANTS


#endif // _PTRACE_H_

