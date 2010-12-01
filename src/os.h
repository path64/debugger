#ifndef _OS_H_
#define _OS_H_

#include "dbg_except.h"
#include "register_set.h"

class OS {
public:
	int regset_size;
	int fpregset_size;
	virtual void char2regset(char *, RegisterSet *) {throw Exception ("Not support")  ;};
	virtual void regset2char(char *, RegisterSet *) {throw Exception ("Not support")  ;};
	virtual void char2fpregset(char *, RegisterSet *) {throw Exception ("Not support")  ;};
	virtual void fpregset2char(char *, RegisterSet *) {throw Exception ("Not support")  ;};
};

class x86_linux_os : public OS
{
private:
	int	bit;
public:
	x86_linux_os(int bit);
	void char2regset(char *, RegisterSet *);
	void regset2char(char *, RegisterSet *);
	void char2fpregset(char *, RegisterSet *);
	void fpregset2char(char *, RegisterSet *);
};

class x86_freebsd_os : public OS
{
private:
	int	bit;
public:
	x86_freebsd_os(int bit);
	void char2regset(char *, RegisterSet *);
	void regset2char(char *, RegisterSet *);
};

#endif
