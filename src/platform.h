#include "register_set.h"

/**
 * A Platform encapsulates all of the interfaces to the target platform.  This
 * includes the functionality exported by ptrace(), or some equivalent.  
 */
class Platform
{
	protected:
		Platform();
	public:
		Platform* GetPlatformForPid(pid_t pid);
		virtual RegisterSet* getIntegerRegisterSet()=0;
		virtual RegisterSet* getFloatingPointRegisterSet()=0;
		virtual RegisterSet* getRegisterSetForName(const std::string &name)
			{ return 0; };

};
