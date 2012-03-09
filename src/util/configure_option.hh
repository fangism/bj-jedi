// "configure_option.hh"

#ifndef	__UTIL_CONFIGURE_OPTION_HH__
#define	__UTIL_CONFIGURE_OPTION_HH__

#include <iostream>
#include <map>

#include "util/string.tcc"	// for string_to_num
#include "util/tokenize.hh"	// for string_list
#include "util/command.hh"

namespace util {
using strings::string_to_num;
using std::ostream;
using std::map;
using std::cout;
using std::cerr;
using std::endl;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static
inline
const char*
yn(const bool y) {
        return y ? "yes" : "no";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class T>
int
configure_value(T& v, const string_list& args,
		const char* name, bool T::*mem) {
switch (args.size()) {
case 1: cout << name << " = " << yn(v.*mem) << endl; break;
default: {
	if (string_to_num(args.back(), v.*mem)) {
		cerr << "Error: invalid boolean value, expecting 0 or 1" << endl;
		return CommandStatus::BADARG;
	}
}
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class T>
int
configure_value(T& v, const string_list& args,
		const char* name, size_t T::*mem) {
switch (args.size()) {
case 1: cout << name << " = " << v.*mem << endl; break;
default: {
	if (string_to_num(args.back(), v.*mem)) {
		cerr << "Error: invalid integer value" << endl;
		return CommandStatus::BADARG;
	}
}
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class T>
static
int
configure_value(T& v, const string_list& args,
		const char* name, double T::*mem) {
switch (args.size()) {
case 1: cout << name << " = " << v.*mem << endl; break;
default: {
	double t;
	if (string_to_num(args.back(), t)) {
		cerr << "Error: invalid real value." << endl;
		return CommandStatus::BADARG;
	}
	if (t < 0.0) {
		cerr << "Error: value must be >= 0.0" << endl;
		return CommandStatus::BADARG;
	}
	v.*mem = t;
}
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class T>
struct printer_fun_ptr {
	typedef void (*type)(ostream&, const T&);
	typedef	map<string, type>		map_type;
};

#define	DECLARE_PRINTER_MAP(T)						\
typedef	util::printer_fun_ptr<T>::map_type	T ## _printer_map_type;	\
static									\
T ## _printer_map_type							\
T ## _member_var_names;							\
static									\
int									\
T ## _add_var_name(const string& s,					\
		const util::printer_fun_ptr<T>::type p) {		\
	T ## _member_var_names[s] = p;					\
	return T ## _member_var_names.size();				\
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define	DEFINE_GENERIC_OPTION_MEMBER_COMMAND(T, mem, str, desc)		\
DECLARE_AND_INITIALIZE_COMMAND_CLASS(T, mem, str, desc)			\
int									\
mem::main(T& v, const string_list& args) {				\
	return util::configure_value(v, args, name, &T::mem);		\
}									\
static void								\
__printer_ ## mem (ostream& o, const T& v) {				\
	o << v.mem;							\
}									\
static									\
int __init_name_ ## mem = T ## _add_var_name(str, & __printer_ ## mem );
// __ATTRIBUTE_UNUSED__

//=============================================================================
}	// end namespace util

#endif	// __UTIL_CONFIGURE_OPTION_HH__

