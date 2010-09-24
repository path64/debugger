#include "platform.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>


class Platform_FreeBSD : Platform
{
	protected:
		pid_t pid;
	public:

		Platform_FreeBSD(pid_t aPid) : Platform(), pid(aPid)
		{
			ptrace(PT_ATTACH, pid, 0, 0);
		};

		~Platform_FreeBSD()
		{
			ptrace(PT_DETACH, pid, 0, 0);
		};

};

#define SET_REG(x) set->set_register(#x, regs.r_ ## x)

typedef long double x87_register_t;

class Platform_FreeBSD_X86 : Platform_FreeBSD
{
		virtual RegisterSet* getIntegerRegisterSet()
		{
			struct reg regs;
			ptrace(PT_GETREGS, pid, (caddr_t)&regs, 0);
			TypedRegisterSet<int> *set = new TypedRegisterSet<int>(X86RegisterProperties);
			// It would be faster to set these all by number, but the ptrace
			// call is likely to be the biggest overhead in this function, and
			// this provides a little bit of sanity checking to make sure that
			// we didn't do it completely wrong.
			SET_REG(eax);
			SET_REG(ecx);
			SET_REG(edx);
			SET_REG(ebx);
			SET_REG(esp);
			SET_REG(ebp);
			SET_REG(esi);
			SET_REG(edi);
			SET_REG(eip);
			SET_REG(eflags);
			SET_REG(es);
			SET_REG(cs);
			SET_REG(ss);
			SET_REG(ds);
			SET_REG(fs);
			SET_REG(gs);
			SET_REG(isp);
			SET_REG(err);
			return set;
		};
		virtual RegisterSet* getFloatingPointRegisterSet()
		{
				struct fpreg regs;
				ptrace(PT_GETXMMREGS, pid, (caddr_t)&regs, 0);
				TypedRegisterSet<unsigned char[10]> *set =
					new TypedRegisterSet<unsigned char[10]>(X86RegisterProperties);
				set->setAllRegisters(&regs.fpr_acc[0]);
				return set;
		};
		virtual RegisterSet* getRegisterSetForName(const std::string &name)
		{
			if ("sse" == name)
			{
				struct xmmreg regs;
				ptrace(PT_GETXMMREGS, pid, (caddr_t)&regs, 0);
				TypedRegisterSet<unsigned char[16]> *set =
					new TypedRegisterSet<unsigned char[16]>(SSERegisterProperties);
				set->setAllRegisters(&regs.xmm_reg[0]);
				return set;
			}
			return 0;
		};
};
