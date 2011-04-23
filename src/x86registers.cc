#include "register_set.h"
#include "dbg_except.h"
//XXX
// #include "x86registers.h"
// #include "x87_register_set.h"

namespace
{

class X86RegisterSetProperties : public RegisterSetProperties
{
public:
	X86RegisterSetProperties() {
		int count = number_of_registers();
		const char **names = register_names();
		for (int i=0 ; i<count ; i++)
		{
			registerNumbers[names[i]] = i;
		}
	};
	virtual const char *name() const { return "main"; };

	virtual int number_of_registers() const { return 18; };

	virtual size_t size_of_register() const { return 4; }

	virtual bool is_integer(int) const { return true; }
	virtual bool is_address(int) const { return true; }

	virtual int register_number_for_dwarf_number(int n) const
	{
		if (n < 0 || n > 9)
			throw Exception("The dwarf reg %d is not support.", n);
		return n;
	}
	virtual int dwarf_number_for_register_number(int n) const
	{ throw Exception("Not support convent reg to dwarf reg."); };
	//XXX
	//virtual RegisterSet* new_empty_register_set(void) { return NULL;};
private:
	virtual const char** register_names() const
	{
		static const char *names[] = { 
			// GPRs
			"eax", "ecx", "edx", "ebx", 
			// stack pointer / base
			"sp", "fp",
			// String operation source / destination
			"esi", "edi", 
			// Instruction pointer, 
			"pc", "eflags",
			// Segment registers
			"es", "cs", "ss", "ds", "fs", "gs",
			"isp", "err", 
			};
		return names;
	}
};

class X87RegisterSetProperties : public RegisterSetProperties
{
public:
	X87RegisterSetProperties(){
		int count = number_of_registers();
		const char **names = register_names();
		for (int i=0 ; i<count ; i++)
		{
			registerNumbers[names[i]] = i;
		}
	};
	virtual const char *name() const { return "x87"; };

	virtual int number_of_registers() const { return 8; };

	virtual size_t size_of_register() const { return 10; }

	virtual bool is_integer(int) const { return true; }
	virtual bool is_address(int) const { return true; }

	virtual bool is_integer() const { return false; }

	virtual int register_number_for_dwarf_number(int n) const
	{
		if (n < 33 || n > 40)
			throw Exception("The dwarf reg %d is not support.", n);
		return n - 33;
	}
	virtual int dwarf_number_for_register_number(int n) const
	{ throw Exception("Not support convent reg to dwarf reg."); };
	//XXX
	//virtual RegisterSet* new_empty_register_set(void) { return NULL;};
private:
	virtual const char** register_names() const
	{
		static const char *names[] = { "st0", "st1", "st2", "st3", "st4", "st5",
			"st6", "st7" };
		return names;
	}
};

class X86_64RegisterSetProperties : public RegisterSetProperties
{
public:
	X86_64RegisterSetProperties(){
		int count = number_of_registers();
		const char **names = register_names();
		for (int i=0 ; i<count ; i++)
		{
			registerNumbers[names[i]] = i;
		}
	};
	virtual const char *name() const { return "x86-64"; };

	virtual int number_of_registers() const { return 24; };

	virtual size_t size_of_register() const { return 4; }

	virtual bool is_integer(int) const { return true; }
	virtual bool is_address(int) const { return true; }

	virtual int register_number_for_dwarf_number(int n) const
	{
		switch (n) {
		case 0:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			return n;
			break;
		case 1:
			return 2;
			break;
		case 2:
			return 1;
			break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			return n + 8;
			break;
		case 16:
			return 8;
			break;
		case 41:
			return 15;
			break;
		case 42:
			return 12;
			break;
		case 43:
			return 9;
			break;
		case 44:
			return 10;
			break;
		case 45:
			return 11;
			break;
		case 46:
			return 13;
			break;
		case 47:
			return 14;
			break;
		}

		throw Exception("The dwarf reg %d is not support.", n);
	}
	virtual int dwarf_number_for_register_number(int n) const
		{ throw Exception("Not support convent reg to dwarf reg."); };
	//XXX
	//virtual RegisterSet* new_empty_register_set(void) { return NULL;};

private:
	virtual const char** register_names() const
	{
		// FIXME:
		static const char *names[] = {
			"rax", "rcx", "rdx", "rbx", "rsi", "rdi", "fp",
			"sp", "pc", "cs", "ss", "ds", "es", "fs", "gs",
			"eflags", "r8", "r9", "r10", "r11", "r12", "r13",
			"r14", "r15",
		};
		return names;
	}
};

}; // end anonymous namespace


static X86RegisterSetProperties x86;
RegisterSetProperties *X86RegisterProperties = &x86;

static X87RegisterSetProperties x87;
RegisterSetProperties *X87RegisterProperties = &x87;

static X86_64RegisterSetProperties x86_64;
RegisterSetProperties *X86_64RegisterProperties = &x86_64;
