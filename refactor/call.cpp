/*
 * (C) Copyright 2003 Diomidis Spinellis.
 *
 * For documentation read the corresponding .h file
 *
 * $Id: call.cpp,v 1.7 2005/06/12 09:39:07 dds Exp $
 */

#include <map>
#include <string>
#include <deque>
#include <vector>
#include <stack>
#include <iterator>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <cassert>

#include "cpp.h"
#include "debug.h"
#include "attr.h"
#include "metrics.h"
#include "fileid.h"
#include "tokid.h"
#include "token.h"
#include "ytab.h"
#include "ptoken.h"
#include "fchar.h"
#include "error.h"
#include "pltoken.h"
#include "macro.h"
#include "pdtoken.h"
#include "ctoken.h"
#include "type.h"
#include "stab.h"
#include "fdep.h"
#include "call.h"
#include "fcall.h"
#include "mcall.h"
#include "eclass.h"

// Function currently being parsed
FCall *Call::current_fun = NULL;

// Set of all functions
Call::fun_map Call::all;

// The current function makes a call to f
void
Call::register_call(Call *f)
{
	register_call(f->get_tokid(), f);
}

// The current function makes a call to id
void
Call::register_call(const Id *id)
{
	register_call(id->get_token(), id);
}

// The current function (token t) makes a call to id
void
Call::register_call(const Token &t, const Id *id)
{
	register_call(t.constituents().begin()->get_tokid(), id->get_fcall());
}

// The current function (tokid t) makes a call to f
void
Call::register_call(Tokid t, Call *f)
{
	if (!current_fun)
		return;
	MCall *m = Pdtoken::get_body_token_macro_mcall(t);
	Call *caller;

	if (m)
		/*
		 * f appears in a macro body, so this is a call
		 * from the macro to the function.
		 */
		caller = m;
	else
		// Function to function call
		caller = current_fun;

	register_call(caller, f);
}

// From calls to
void
Call::register_call(Call *from, Call *to)
{
	from->add_call(to);
	to->add_caller(from);
	if (DP())
		cout << from->name << " calls " << to->name << "\n";
}

// ctor; never call it if the call for t already exists
Call::Call(const string &s, const Token &t) :
		name(s),
		token(t)
{
	if (DP())
		cout << "Construct new call for " << s << '\n';
	all.insert(fun_map::value_type(t.get_parts_begin()->get_tokid(), this));
}

// Return true if e appears in the eclasses comprising our name
bool
Call::contains(Eclass *e) const
{
	for (dequeTpart::const_iterator i = get_token().get_parts_begin(); i != get_token().get_parts_end(); i++) {
		int len = i->get_len();
		Tokid t = i->get_tokid();
		for (int pos = 0; pos < len;) {
			Eclass *e2 = t.get_ec();
			if (e2 == NULL) {
				if (DP())
					cout << "No eclass for tokid " << t << "\n";
				return false;
			}
			if (e == e2)
				return true;
			t += e2->get_len();
			pos += e2->get_len();
		}
	}
	return false;
}

void
Call::clear_visit_flags()
{
	for (const_fmap_iterator_type i = all.begin(); i != all.end(); i++)
		i->second->visited = false;
}

Call *
Call::get_call(const Token &t)
{
	pair <const_fmap_iterator_type, const_fmap_iterator_type> maybe(all.equal_range(t.get_parts_begin()->get_tokid()));
	const_fmap_iterator_type i;

	for (i = maybe.first; i != maybe.second; i++) {
		if (DP())
			cout << "Compare " << t << " with " << i->second->get_token() << "\n";
		if (t.equals(i->second->get_token())) {
			if (DP())
				cout << "Get call for " << t << " returns " << &(i->second) << "\n";
			return i->second;
		}
	}
	return NULL;
}

pair <Call::const_fmap_iterator_type, Call::const_fmap_iterator_type>
Call::get_calls(Tokid t)
{
	return all.equal_range(t);
}
