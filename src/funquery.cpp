/*
 * (C) Copyright 2001-2015 Diomidis Spinellis
 *
 * This file is part of CScout.
 *
 * CScout is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CScout is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CScout.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Encapsulates a (user interface) function query
 *
 */

#include <map>
#include <string>
#include <deque>
#include <vector>
#include <stack>
#include <iterator>
#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <functional>
#include <algorithm>		// set_difference
#include <sstream>		// ostringstream
#include <cstdio>		// perror, rename
#include <cstdlib>		// atoi

#include <regex.h>

#include "getopt.h"

#include "cpp.h"
#include "debug.h"
#include "error.h"
#include "parse.tab.h"
#include "attr.h"
#include "metrics.h"
#include "fileid.h"
#include "tokid.h"
#include "token.h"
#include "ptoken.h"
#include "fchar.h"
#include "pltoken.h"
#include "macro.h"
#include "pdtoken.h"
#include "eclass.h"
#include "ctoken.h"
#include "type.h"
#include "stab.h"
#include "fdep.h"
#include "version.h"
#include "call.h"
#include "fcall.h"
#include "mcall.h"
#include "compiledre.h"
#include "option.h"
#include "query.h"
#include "mquery.h"
#include "funquery.h"

int FunQuery::specified_order::order;
bool FunQuery::specified_order::reverse;
bool FunQuery::specified_order::q_ncall;


// Construct an object based on URL parameters
FunQuery::FunQuery(bool icase, Attributes::size_type cp, bool e, bool r) :
	Query(!e, r, true),
	match_type('Y'),
	match_fid(false),
	id_ec(NULL),
	call(NULL),
	current_project(cp),
	error(new char[256])
{
	if (lazy)
		return;

	valid = true;
	// Query name
	const char *qname = server.getCharPParam("n");
	if (qname != NULL && *qname)
		name = qname;
	// if(qname != NULL) delete qname;
	// Match specific file
	int flid = server.getIntParam("fid");
	if (!flid) {
		match_fid = true;
		fid = Fileid(flid);
	}

	// Function call declaration direct match
	call = (Call *)server.getAddrParam("call");
	// Identifier EC match
	id_ec = (Eclass *)server.getAddrParam("ec");
	// Type of boolean match
	const char *m;
	if (id_ec == NULL){
		m = server.getCharPParam("match");
		if (m == NULL) {
			sprintf(error, "Missing value: match");
			valid = return_val = false;
			lazy = true;
			return;
		}
		match_type = *m;
	}
	mquery.set_match_type(match_type);
	cfun = !!server.getBoolParam("cfun");
	macro = !!server.getBoolParam("macro");
	writable = !!server.getBoolParam("writable");
	ro = !!server.getBoolParam("ro");
	pscope = !!server.getBoolParam("pscope");
	fscope = !!server.getBoolParam("fscope");
	defined = !!server.getBoolParam("defined");

	// Identifier EC match
	if (server.getBoolParam("ncallers") && !(ncallers = server.getIntParam("ncallers"))) {
		ncallerop = ec_ignore;
	} else {
		if((ncallerop = server.getIntParam("ncallerop")) == -1)
			ncallerop = ec_ignore;
	}

	exclude_fnre = !!server.getBoolParam("xfnre");
	exclude_fure = !!server.getBoolParam("xfure");
	exclude_fdre = !!server.getBoolParam("xfdre");
	exclude_fre = !!server.getBoolParam("xfre");
	delete m;
	
	// Compile regular expression specs
	error = compile_re("Function name", "fnre", fnre, match_fnre, str_fnre);
	if(error != NULL){ 
		delete error;
		return ;
	}	
	error = compile_re("Calling function name", "fure", fure, match_fure, str_fure);
	if(error != NULL){ 
		delete error;
		return ;
	}
	error = compile_re("Called function name", "fdre", fdre, match_fdre, str_fdre);
	if(error != NULL){ 
		delete error;
		return ;
	}
	error = compile_re("Filename", "fre", fre, match_fre, str_fre, (icase ? REG_ICASE : 0));
	if(error != NULL){ 
		delete error;
		return ;
	}		
	specified_order::set_order(mquery.get_sort_order(), mquery.get_reverse(), server.getBoolParam("qncall"));
}

// Return the URL for re-executing this query
string
FunQuery::base_url() const
{
	return string("xfunquery.html?") + param_url();
}

// Return the query's parameters as a URL
string
FunQuery::param_url() const
{
	char buff[256];

	string r("qt=fun");
	if (id_ec) {
		char buff[256];

		sprintf(buff, "&ec=%p", id_ec);
		r += buff;
	} else {
		r += "&match=";
		r += Query::url(string(1, match_type));
	}
	r += mquery.param_url();
	if (call) {
		sprintf(buff, "&call=%p", call);
		r += buff;
	}
	if (match_fid) {
		sprintf(buff, "&fid=%d", fid.get_id());
		r += buff;
	}
	if (cfun)
		r += "&cfun=1";
	if (macro)
		r += "&macro=1";
	if (writable)
		r += "&writable=1";
	if (ro)
		r += "&ro=1";
	if (pscope)
		r += "&pscope=1";
	if (fscope)
		r += "&fscope=1";
	if (defined)
		r += "&defined=1";
	if (match_fnre)
		r += "&fnre=" + Query::url(str_fnre);
	if (match_fure)
		r += "&fure=" + Query::url(str_fure);
	if (match_fdre)
		r += "&fdre=" + Query::url(str_fdre);
	if (match_fre)
		r += "&fre=" + Query::url(str_fre);
	if (exclude_fnre)
		r += "&xfnre=1";
	if (exclude_fure)
		r += "&xfure=1";
	if (exclude_fdre)
		r += "&xfdre=1";
	if (exclude_fre)
		r += "&xfre=1";
	if (ncallerop != ec_ignore) {
		ostringstream varname;

		varname << "&ncallers=" << ncallers;
		varname << "&ncallerop=" << ncallerop;
		r += varname.str();
	} else
		r += "&ncallerop=0";
	if (name.length())
		r += "&n=" + Query::url(name);
	return r;
}

// Find how many times an identifier has appeared
// in a function
int 
FunQuery::appeared(Call *c){
	int no = 0;
	if (id_ec) {
		if (!c->is_span_valid())
			return 0;
		const setTokid &m = id_ec->get_members();
		for (setTokid::const_iterator i = m.begin(); i != m.end(); i++)
			if (*i >= c->get_begin().get_tokid() && *i <= c->get_end().get_tokid())
				no++;
		return no;
	}
	else {
		return -1;
	}
	
}

// Evaluate the object's identifier query against i
// return true if it matches
bool
FunQuery::eval(Call *c)
{
	if (lazy)
		return return_val;

	if (call)
		return (c == call);

	if (id_ec) {
		if (!c->is_span_valid())
			return false;
		const setTokid &m = id_ec->get_members();
		for (setTokid::const_iterator i = m.begin(); i != m.end(); i++)
			if (*i >= c->get_begin().get_tokid() && *i <= c->get_end().get_tokid())
			    	return true;
		return false;
	}

	if (match_fid && c->get_begin().get_tokid().get_fileid() != fid)
		return false;

	Eclass *ec = c->get_tokid().get_ec();
	if (current_project && !ec->get_attribute(current_project))
		return false;

	bool add = mquery.eval(*c);
	switch (match_type) {
	case 'Y':	// anY match
		add = (add || (cfun && c->is_cfun()));
		add = (add || (macro && c->is_macro()));
		add = (add || (writable && !ec->get_attribute(is_readonly)));
		add = (add || (ro && ec->get_attribute(is_readonly)));
		add = (add || (pscope && !c->is_file_scoped()));
		add = (add || (fscope && c->is_file_scoped()));
		add = (add || (defined && c->is_defined()));
		break;
	case 'L':	// alL match
		add = (add && (!cfun || c->is_cfun()));
		add = (add && (!macro || c->is_macro()));
		add = (add && (!writable || !ec->get_attribute(is_readonly)));
		add = (add && (!ro || ec->get_attribute(is_readonly)));
		add = (add && (!pscope || !c->is_file_scoped()));
		add = (add && (!fscope || c->is_file_scoped()));
		add = (add && (!defined || c->is_defined()));
		break;
	case 'E':	// excludE match
		add = (add && (!cfun || !c->is_cfun()));
		add = (add && (!macro || !c->is_macro()));
		add = (add && (!writable || ec->get_attribute(is_readonly)));
		add = (add && (!ro || !ec->get_attribute(is_readonly)));
		add = (add && (!pscope || c->is_file_scoped()));
		add = (add && (!fscope || !c->is_file_scoped()));
		add = (add && (!defined || !c->is_defined()));
		break;
	case 'T':	// exactT match
		add = (add && (cfun == c->is_cfun()));
		add = (add && (macro == c->is_macro()));
		add = (add && (writable == !ec->get_attribute(is_readonly)));
		add = (add && (ro == ec->get_attribute(is_readonly)));
		add = (add && (pscope == !c->is_file_scoped()));
		add = (add && (fscope == c->is_file_scoped()));
		add = (add && (defined == c->is_defined()));
		break;
	}
	if (!add)
		return false;
	if (ncallerop && !Query::apply(ncallerop, c->get_num_caller(), ncallers))
		return false;
	int retval = exclude_fnre ? 0 : REG_NOMATCH;
	if (match_fnre && fnre.exec(c->get_name()) == retval)
		return false;
	retval = exclude_fre ? 0 : REG_NOMATCH;
	if (match_fre && fre.exec(c->get_fileid().get_path()) == retval)
			return false;
	Call::const_fiterator_type c2;
	if (match_fdre) {
		for (c2 = c->call_begin(); c2 != c->call_end(); c2++)
			if (fdre.exec((*c2)->get_name()) == 0) {
				if (exclude_fdre)
					return false;
				else
					break;
			}
		if (!exclude_fdre && c2 == c->call_end())
			return false;
	}
	if (match_fure) {
		for (c2 = c->caller_begin(); c2 != c->caller_end(); c2++)
			if (fure.exec((*c2)->get_name()) == 0) {
				if (exclude_fure)
					return false;
				else
					break;
			}
		if (!exclude_fure && c2 == c->caller_end())
			return false;
	}
	return true;
}
