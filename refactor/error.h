/* 
 * (C) Copyright 2001 Diomidis Spinellis.
 *
 * Error reporting
 *
 * Include synopsis:
 * #include <iostream>
 * #include <string>
 * #include "tokid.h"
 *
 * $Id: error.h,v 1.1 2001/08/20 15:32:58 dds Exp $
 */

#ifndef ERROR_
#define ERROR_

enum e_error_level {
	E_WARN,
	E_ERR,
	E_INTERNAL,
	E_FATAL
};

class Error {
private:
	static int num_errors;
	static int num_warnings;
public:
	static void error(enum e_error_level level, string msg);
	static int get_num_errors();
	static int get_num_warnings();
};

#endif // ERROR_