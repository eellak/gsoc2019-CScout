/*
 * (C) Copyright 2006-2015 Diomidis Spinellis
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
 * A pager for HTML output
 *
 */

#include <string>
#include <set>
#include <algorithm>		// max
#include <cstdio>		// FILE
#include <cstdlib>		// atoi

#include "headers.h"
#include "getopt.h"

#include "cpp.h"
#include "debug.h"
#include "pager.h"

Pager::Pager(int ps, const string &qurl, bool bmk) : pagesize(ps), current(0), url(qurl), bookmarkable(bmk)
{
	skip = (unsigned int)server.getIntParam("skip");

}

bool
Pager::show_next()
{
	bool ret = (skip == -1) || ((current >= skip) && (current < skip + pagesize));
	current++;
	return (ret);
}

json::value
Pager::end()
{
	// Total number of elements
	int nelem = current;
	// Total number of pages
	int npages = nelem / pagesize + (nelem % pagesize ? 1 : 0);
	int thispage = skip / pagesize;
	json::value to_return;
	switch (nelem) {
	case 0:
		to_return["total"]=json::value(0);
		break;
	case 1:
		to_return["total"]=json::value(1);
		break;
	default:
		if (skip == -1){
			to_return["start"]=json::value(1);
			to_return["end"]=json::value(nelem);
		}	
		else{
			to_return["start"]=json::value(thispage * pagesize + 1);
			to_return["end"]=json::value(min(thispage * pagesize + pagesize, nelem));			
		}
		to_return["total"]=json::value(nelem);
		break;
	}
	
	if (nelem > pagesize) {
		if (skip > 0)
			to_return["prev"] =json::value::string(url+"&skip=" +to_string(skip - pagesize),true);
		for (int i = 0; i < npages; i++){
			
			if (i == thispage && skip != -1)
				to_return["others"][i] = json::value::string("this");
			else
				to_return["others"][i] = json::value::string(url+"skip="+to_string(i * pagesize),true);
		}
		if (skip != -1 && thispage + 1 < npages)
			to_return["next"] =json::value::string(url+"&skip=" +to_string(skip + pagesize));
		if (skip != -1)
			to_return["all"] =json::value::string(url+"&skip=-1");
	}
	if (bookmarkable)
		to_return["url"] = json::value::string(url);
	return to_return;
}
