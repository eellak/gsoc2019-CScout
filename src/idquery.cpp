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
 * Encapsulates an (user interface) identifier query
 * Can be used to evaluate against IdProp elements
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
#include <cctype>
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
#include "idquery.h"

IdProp Identifier::ids;

// Construct an object based on URL parameters
IdQuery::IdQuery(bool icase, Attributes::size_type cp, bool e, bool r) :
	Query(!e, r, true),
	match(attr_end),
	current_project(cp)
{
	if (lazy){

		return;
		}
	// Query name
	const char *qname = server.getCharPParam("n");
	if (qname !=NULL && *qname)
		name = qname;
	if(qname!=NULL) delete qname;
	
	// Identifier EC match to change
	void * t = server.getAddrParam("ec"); 
	if (t!=NULL) {
		ec = (Eclass *)t;
	} else {
		ec = NULL;

		// Type of boolean match
		const char *m= server.getCharPParam("match");
		if (m==NULL ) {
			error = "Missing value: match";
			valid = return_val = false;
			lazy = true;
			return;
		}
		// cout<<"m:"<<m<<endl;;
		match_type = *m;
		delete m;
	}

	xfile = !!server.getIntParam("xfile");
	unused = !!server.getIntParam("unused");
	writable = !!server.getIntParam("writable");
	exclude_ire = !!server.getIntParam("xire");
	exclude_fre = !!server.getIntParam("xfre");
	error = compile_re("Identifier", "ire", ire, match_ire, str_ire);
	// Compile regular expression specs
	if (error != NULL)
		return;
	
	error = compile_re("Filename", "fre", fre, match_fre, str_fre, (icase ? REG_ICASE : 0));
	if (error!=NULL)
		return;

	// Store match specifications in a vector
	for (int i = attr_begin; i < attr_end; i++) {
		ostringstream varname;

		varname << "a" << i;
		match[i] = !!server.getIntParam(varname.str());
		/*if (DP())
			cout << "v=[" << varname.str() << "] m=" << match[i] << "\n";
*/
	}
	
}

// Report the string query specification usage
void
IdQuery::usage(void)
{
	cerr << "The monitored identifier attributes must be specified using the syntax:\n"
		"Y|L|E|T[:attr1][:attr2]...\n"
		"\tY: Match any of the specified attributes\n"
		"\tL: Match all of the specified attributes\n"
		"\tE: Exclude the specified attributes matched\n"
		"\tT: Exact match of the specified attributes\n\n"

		"Allowable attribute names are:\n"
		"\tunused: Unused\n"
		"\twritable: Writable\n";
	for (int i = attr_begin; i < attr_end; i++)
		cerr << "\t" << Attributes::shortname(i) << ": " << Attributes::name(i) << "\n";
	exit(1);
}

// Construct an object based on a string specification
// The syntax is Y|L|E|T[:attr1][:attr2]...
IdQuery::IdQuery(const string &s) :
	Query(false, false, true),
	match_fre(false),
	match_ire(false),
	match(attr_end),
	xfile(false),
	ec(NULL),
	current_project(0)
{
	// Type of boolean match
	if (s.length() == 0)
		usage();
	switch (s[0]) {
	case 'Y':
	case 'L':
	case 'E':
	case 'T':
		match_type = s[0];
		break;
	default:
		usage();
	}

	unused = (s.find(":unused") != string::npos);
	writable = (s.find(":writable") != string::npos);

	// Store match specifications in a vector
	for (int i = attr_begin; i < attr_end; i++)
		match[i] = (s.find(":" + Attributes::shortname(i)) != string::npos);
}

// Return the URL for re-executing this query
string
IdQuery::base_url() const
{
	return string("xiquery.html?") + param_url();
}


// Return the query's parameters as a URL
string
IdQuery::param_url() const
{
	string r("qt=id");
	if (ec) {
		char buff[256];

		sprintf(buff, "&ec=%p", ec);
		r += buff;
	} else {
		r += "&match=";
		r += Query::url(string(1, match_type));
	}
	if (xfile)
		r += "&xfile=1";
	if (unused)
		r += "&unused=1";
	if (writable)
		r += "&writable=1";
	if (match_ire)
		r += "&ire=" + Query::url(str_ire);
	if (exclude_ire)
		r += "&xire=1";
	if (exclude_fre)
		r += "&xfre=1";
	if (match_fre)
		r += "&fre=" + Query::url(str_fre);
	for (int i = attr_begin; i < attr_end; i++) {
		if (match[i]) {
			ostringstream varname;

			varname << "&a" << i << "=1";
			r += varname.str();
		}
	}
	if (name.length())
		r += "&n=" + Query::url(name);
	return r;
}

// Evaluate the object's identifier query against i
// return true if it matches
bool
IdQuery::eval(const IdPropElem &i)
{
	if (lazy)
		return return_val;

	if (ec)
		return (i.first == ec);
	if (current_project && !i.first->get_attribute(current_project))
		return false;
	int retval = exclude_ire ? 0 : REG_NOMATCH;
	if (match_ire && ire.exec(i.second.get_id()) == retval)
		return false;
	bool add = false;
//	cerr<<"not vars problem xiquery-match:"<<match_type<<endl;
	switch (match_type) {
	case 'Y':	// anY match
		add = false;
		for (int j = attr_begin; j < attr_end; j++)
			if (match[j] && i.first->get_attribute(j)) {
				add = true;
				break;
			}
		add = (add || (xfile && i.second.get_xfile()));
		add = (add || (unused && i.first->is_unused()));
		add = (add || (writable && !i.first->get_attribute(is_readonly)));
		break;
	case 'L':	// alL match
		add = true;
		for (int j = attr_begin; j < attr_end; j++)
			if (match[j] && !i.first->get_attribute(j)) {
				add = false;
				break;
			}
		add = (add && (!xfile || i.second.get_xfile()));
		add = (add && (!unused || i.first->is_unused()));
		add = (add && (!writable || !i.first->get_attribute(is_readonly)));
		break;
	case 'E':	// excludE match
		add = true;
		for (int j = attr_begin; j < attr_end; j++)
			if (match[j] && i.first->get_attribute(j)) {
				add = false;
				break;
			}
		add = (add && (!xfile || !i.second.get_xfile()));
		add = (add && (!unused || !(i.first->is_unused())));
		add = (add && (!writable || i.first->get_attribute(is_readonly)));
		break;
	case 'T':	// exactT match
		add = true;
		for (int j = attr_begin; j < attr_end; j++)
			if (match[j] != i.first->get_attribute(j)) {
				add = false;
				break;
			}
		add = (add && (xfile == i.second.get_xfile()));
		add = (add && (unused == (i.first->is_unused())));
		add = (add && (writable == !i.first->get_attribute(is_readonly)));
		break;
	}
	if (!add)
		return false;
	if (match_fre) {
		// Before we add it check if its filename matches the RE
		IFSet f = i.first->sorted_files();
		IFSet::iterator j;
		for (j = f.begin(); j != f.end(); j++)
			if (fre.exec((*j).get_path()) == 0) {
				if (DP())
					cout << "Identifier " << i.second.get_id() <<
					    " occurs in file " << j->get_path() <<
					    ", which matches RE " << str_fre << "\n";
				if (exclude_fre)
					return false;	// Match; fail
				else
					break;		// Match; stop search
			}
		if (!exclude_fre && j == f.end())
			return false;	// Asked to match and no match found
	}
	return true;
}

