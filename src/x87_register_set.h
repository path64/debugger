
#include "register_set.h"

/**
 * X87RegisterSet encapsulates the floating point register stack in an x87 FPU.
 * This contains 8 10-bit floating point registers.
 */
class X87RegisterSet : RegisterSet
{
	protected:
		char values[10][8];
	public:
		X87RegisterSet();
		X87RegisterSet(RegisterSetProperties *p);
		/**
		 * Returns the value of the register, interpreted as a 64-bit integer.
		 */
		virtual int64_t get_register_as_integer(const int num);
		/**
		 * Gets the value of the register as a double.
		 */
		virtual double get_register_as_double(const int num);
		/**
		 * Returns the value of the register as a vector of bytes.  The
		 * representation of this is undefined in the general case - it will be
		 * the internal representation of registers in this register set.
		 */
		virtual std::vector<unsigned char> get_register_as_bytes(const int num);
		/**
		 * Sets the value of the indicated register from an integer.
		 */
		virtual void set_register(const int num, int64_t v);
		/**
		 * Sets the value of the register 
		 */
		virtual void set_register(const int num, double v);
		/**
		 * Sets the value of the register from a vector of bytes in target byte
		 * order.
		 */
		virtual void set_register(const int num, 
		                          const std::vector<unsigned char> &v);
};
