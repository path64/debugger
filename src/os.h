#ifndef _OS_H_
#define _OS_H_

#include "register_set.h"

class OS {
public:
	virtual void char2regset(char *, RegisterSet *) {};
	virtual void char2fpregset(char *, RegisterSet *) {};
};

class x86_linux_os : public OS
{
private:
	int	bit;
public:
	x86_linux_os(int bit);
	void char2regset(char *, RegisterSet *);
	void char2fpregset(char *, RegisterSet *);
};

#endif
