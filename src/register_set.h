#ifndef __PATHDB_REG_SET_INCLUDED__
#define __PATHDB_REG_SET_INCLUDED__

#include <map>
#include <list>
#include <string>
#include <vector>
#include <stdint.h>
#include <assert.h>

class RegisterSet;
class PStream;
/**
 * RegisterSetProperties encapsulate metadata about register sets.  Each
 * platform provides a singleton subclass of this for each set of registers in
 * the target CPU.
 */
class RegisterSetProperties
{
	private:
		/**
		 * Returns the names of all of the registers in this set, in an array
		 * of C strings.
		 */
		virtual const char** register_names() const=0;
	public:
		std::map<std::string, int> registerNumbers;

		static const int invalid_register = -1;
 		RegisterSetProperties();
		/**
		 * Returns the name of the register set.
		 */
		virtual const char *name() const=0;
		/**
		 * Returns the number of registers in this register set.
		 */
		virtual int number_of_registers() const=0;
		/**
		 * Returns the size of registers in this set, in bytes.
		 */
		virtual size_t size_of_register() const=0;
		/**
		 * Returns true for integer registers.
		 */
		virtual bool is_integer(int num) const=0;
		/**
		 * Returns true if this register stores an integer value.
		 */
		virtual bool is_address(int num) const=0;
		/**
		 * Returns the register number within this register set for a specific
		 * DWARF register number.  
		 */
		virtual int register_number_for_dwarf_number(int) const=0;
		/**
		 * Returns the DWARF number for a register number in this set.
		 */
		virtual int dwarf_number_for_register_number(int) const=0;
		/**
		 * Returns a new register set instance using these properties.
		 */
		virtual RegisterSet* new_empty_register_set(void);
		/**
		 * Returns the register number for a named register.
		 */
		int register_number_for_name(const std::string &_name)
		{
			std::map<std::string, int>::iterator i = registerNumbers.find(_name);
			return (registerNumbers.end() == i) ? -1 : i->second;
		}
		/**
		 * Returns he name for a specific register number.  The returned string
		 * is statically allocated and does not need to be free()'d.
		 */
		const char* name_for_register_number(int num) const
		{
			assert(num < number_of_registers());
			return register_names()[num];
		}
};

/**
 * RegisterSet is an abstract class defining a set of registers.  Each
 * architecture exposes one or more register sets.  
 *
 * Concrete subclasses of this provide accessors.  
 */
class RegisterSet 
{
	protected:
		/**
		 * The properties of this register set.
		 */
		RegisterSetProperties *properties;
		/**
		 * Flag indicating that this register set has been modified since it
		 * was created (or since the flag was explicitly cleared).
		 */
		bool isDirty;
	public:
		RegisterSet(RegisterSetProperties *p);
		virtual ~RegisterSet();
		/**
		 * Returns the properties of this register set.
		 */
		RegisterSetProperties *get_properties(){ return properties; };

		/**
		 * Returns the value of the register, interpreted as a 64-bit integer.
		 */
		virtual int64_t get_register_as_integer(const int num) = 0;
		/**
		 * Gets the value of the register as a double.
		 */
		virtual double get_register_as_double(const int num) = 0;
		/**
		 * Returns the value of the register as a vector of bytes.  The
		 * representation of this is undefined in the general case - it will be
		 * the internal representation of registers in this register set.
		 */
		virtual std::vector<unsigned char> get_register_as_bytes(const int num) = 0;
		/**
		 * Sets the value of the indicated register from an integer.
		 */
		virtual void set_register(const int num, int64_t v) = 0;
		/**
		 * Sets the value of the register 
		 */
		virtual void set_register(const int num, double v) = 0;
		/**
		 * Sets the value of the register from a vector of bytes in target byte
		 * order.
		 */
		virtual void set_register(const int num, 
		                          const std::vector<unsigned char> &v) = 0;
		/**
		 * Takes the values from another register set.  Both sets must have
		 * been created with the same set of properties.
		 */
		virtual void take_values_from(RegisterSet* set) = 0;
		/**
		 * Returns the value of the register, interpreted as a 64-bit integer.
		 */
		int64_t get_register_as_integer(const std::string &name)
		{
			int num = properties->register_number_for_name(name);
			assert(num != RegisterSetProperties::invalid_register);
			return get_register_as_integer(num);
		}
		/**
		 * Gets the value of the register as a double.
		 */
		double get_register_as_double(const std::string &name)
		{
			int num = properties->register_number_for_name(name);
			assert(num != RegisterSetProperties::invalid_register);
			return get_register_as_double(num);
		}
		/**
		 * Returns the value of the register as a vector of bytes.  The
		 * representation of this is undefined in the general case - it will be
		 * the internal representation of registers in this register set.
		 */
		std::vector<unsigned char> get_register_as_bytes(const std::string &name)
		{
			int num = properties->register_number_for_name(name);
			assert(num != RegisterSetProperties::invalid_register);
			return get_register_as_bytes(num);
		}
		/**
		 * Sets the value of the indicated register from an integer.
		 */
		void set_register(const std::string &name, int64_t v)
		{
			int num = properties->register_number_for_name(name);
			assert(num != RegisterSetProperties::invalid_register);
			return set_register(num, v);
		}
		/**
		 * Sets the value of the register 
		 */
		void set_register(const std::string &name, double v)
		{
			int num = properties->register_number_for_name(name);
			assert(num != RegisterSetProperties::invalid_register);
			return set_register(num, v);
		}
		/**
		 * Sets the value of the register from a vector of bytes in target byte
		 * order.
		 */
		void set_register(const std::string &name,
		                  const std::vector<unsigned char> &v)
		{
			int num = properties->register_number_for_name(name);
			assert(num != RegisterSetProperties::invalid_register);
			return set_register(num, v);
		}
		/**
		 * Returns whether any registers in this register set have been
		 * modified since it was created.
		 */
		bool is_dirty() { return isDirty; };
		/**
		 * Clears the dirty flag.  is_dirty() will return false until the next
		 * time that a register in this set is modified.
		 */
		void clear_dirty_flag() { isDirty = false; }

		/**
		 * Prints the register set to the specified stream.  May be overridden
		 * in subclasses to provide other formatting, for example for vector
		 * registers.
		 */
		virtual void print(PStream &os);
};

/**
 * Concrete subclass encapsulating floating point register sets.  This is
 * exposed solely to make it easy to construct floating point register sets in
 * platform-specific code.  You should never cast a RegisterSet to this class -
 * individual architectures are free to provide their own register set classes
 * that use different internal representations.
 */
class FloatRegisterSet : public RegisterSet
{
	protected:
		double *values;
	public:
		FloatRegisterSet(RegisterSetProperties *r);
		virtual ~FloatRegisterSet();
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
		/**
		 * Takes the values from another register set.  Both sets must have
		 * been created with the same set of properties.
		 */
		virtual void take_values_from(RegisterSet* set);
};

/**
 * Concrete subclass encapsulating integer register sets.  This is exposed
 * solely to make it easy to construct integer register sets in
 * platform-specific code.  You should never cast a RegisterSet to this class -
 * individual architectures are free to provide their own register set classes
 * that use different internal representations.
 */
class IntegerRegisterSet : public RegisterSet
{
	protected:

		int64_t *values;
	public:
		IntegerRegisterSet(RegisterSetProperties *r);
		virtual ~IntegerRegisterSet();
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
		/**
		 * Takes the values from another register set.  Both sets must have
		 * been created with the same set of properties.
		 */
		virtual void take_values_from(RegisterSet* set);
};


extern RegisterSetProperties *X86RegisterProperties;
extern RegisterSetProperties *X87RegisterProperties;
extern RegisterSetProperties *SSERegisterProperties;
extern RegisterSetProperties *X86_64RegisterProperties;

#endif
