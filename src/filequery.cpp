/*
 * (C) Copyright 2001-2015 Diomidis Spinellis.
 *
 * Encapsulates a (user interface) file query
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
#include <algorithm> // set_difference
#include <sstream>   // ostringstream
#include <cstdio>	// perror, rename
#include <cstdlib>   // atoi

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
#include "filequery.h"

int FileQuery::specified_order::order;
bool FileQuery::specified_order::reverse;

// Construct an object based on URL parameters
FileQuery::FileQuery(std::ostringstream *of, bool icase, Attributes::size_type cp, bool e, bool r) : Query(!e, r, true),
																									 current_project(cp)
{
	if (lazy)
		return;

	valid = true;

	// Query name
	const char *qname = server.getCharPParam("n");
	if (qname != NULL && *qname)
	{
		name = string(qname);
	}
	if (qname != NULL)
		delete qname;
	// Type of boolean match
	const char *m = server.getCharPParam("match");
	if (m == NULL)
	{
		(*of) << "Missing value: match";
		valid = return_val = false;
		lazy = true;
		return;
	}
	match_type = *m;
	mquery.set_match_type(match_type);

	writable = !!server.getBoolParam("writable");
	ro = !!server.getBoolParam("ro");
	exclude_fre = !!server.getBoolParam("xfre");
	// Compile regular expression specs
	(*of) << compile_re("Filename", "fre", fre, match_fre, str_fre, (icase ? REG_ICASE : 0));
	
	if (!(of->str().empty()))
		return;
	specified_order::set_order(mquery.get_sort_order(), mquery.get_reverse());
}

// Return the URL for re-executing this query
string
FileQuery::base_url() const
{
	return string("xfilequery.html?") + param_url();
}

// Return the query's parameters as a URL
string
FileQuery::param_url() const
{
	ostringstream url;
	url << "match=";
	url << Query::url(string(1, match_type));
	url << mquery.param_url();
	if (writable)
		url << "&writable=1";
	if (ro)
		url << "&ro=1";
	if (match_fre)
		url << "&fre=" << Query::url(str_fre);
	if (exclude_fre)
		url << "&xfre=1";
	if (name.length())
		url << "&n=" << Query::url(name);
	return url.str();
}

// Evaluate the object's identifier query against i
// return true if it matches
bool FileQuery::eval(Fileid &f)
{
	if (lazy)
		return return_val;

	if (current_project && !f.get_attribute(current_project))
		return false;

	bool add = mquery.eval(f);
	switch (match_type)
	{
	case 'Y': // anY match
		add = (add || (ro && f.get_readonly()));
		add = (add || (writable && !f.get_readonly()));
		break;
	case 'L': // alL match
		add = (add && (!ro || f.get_readonly()));
		add = (add && (!writable || !f.get_readonly()));
		break;
	}
	if (!add)
		return false;
	
	int retval = exclude_fre ? 0 : REG_NOMATCH;
	if (match_fre && fre.exec(f.get_path()) == retval){
		return false; // RE match failed spec
	}
	return true;
}
