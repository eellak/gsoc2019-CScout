/*
 * (C) Copyright 2002-2015 Diomidis Spinellis
 *
 * You may only use this code if you agree to the terms of the CScout
 * Source Code License agreement (see License.txt).
 * If you do not agree to the terms, do not use the code.
 *
 * For documentation read the corresponding .h file
 *
 */

#include <iostream>
#include <map>
#include <string>
#include <deque>
#include <fstream>
#include <stack>
#include <set>
#include <vector>
#include <sstream>		// ostringstream
#include <list>
#include <cmath>		// log
#include <errno.h>

#include "cpp.h"
#include "debug.h"
#include "error.h"
#include "attr.h"
#include "metrics.h"
#include "fileid.h"
#include "tokid.h"
#include "tokmap.h"
#include "eclass.h"
#include "fchar.h"
#include "token.h"
#include "call.h"

MetricDetails Metrics::metric_details[] = {
// BEGIN AUTOSCHEMA Metrics
	{ em_nchar,		"NCHAR",		"Number of characters"},
	{ em_nccomment,		"NCCOMMENT",		"Number of comment characters"},
	{ em_nspace,		"NSPACE",		"Number of space characters"},
	{ em_nlcomment,		"NLCOMMENT",		"Number of line comments"},
	{ em_nbcomment,		"NBCOMMENT",		"Number of block comments"},
	{ em_nline,		"NLINE",		"Number of lines"},
	{ em_maxlinelen,	"MAXLINELEN",		"Maximum number of characters in a line"},
	{ em_nstring,		"NSTRING",		"Number of character strings"},
	{ em_nuline,		"NULINE",		"Number of unprocessed lines"},

	{ em_npptoken,		"NPPTOKEN",		"Number of preprocessed tokens"},
	{ em_nctoken,		"NCTOKEN",		"Number of compiled tokens"},
	{ em_nppdirective,	"NPPDIRECTIVE",		"Number of C preprocessor directives"},
	{ em_nppcond,		"NPPCOND",		"Number of processed C preprocessor conditionals (ifdef, if, elif)"},
	{ em_nppfmacro,		"NPPFMACRO",		"Number of defined C preprocessor function-like macros"},
	{ em_nppomacro,		"NPPOMACRO",		"Number of defined C preprocessor object-like macros"},
// END AUTOSCHEMA Metrics
	{ metric_max,		"",			""},
};

// Global metrics
IdMetricsSummary id_msum;

// Called for every identifier
void
Metrics::process_id(const string &s, Eclass *ec)
{
	count[em_nchar] += s.length();
	currlinelen += s.length();
}

// Called for all file characters appart from identifiers
void
Metrics::process_char(char c)
{
	count[em_nchar]++;
	if (c == '\n') {
		if (DP())
			cout << "nline = " << count[em_nline] << "\n";
		count[em_nline]++;
		if (currlinelen > count[em_maxlinelen])
			count[em_maxlinelen] = currlinelen;
		currlinelen = 0;
	} else
		currlinelen++;
	switch (cstate) {
	case s_normal:
		if (isspace(c))
			count[em_nspace]++;
		else if (c == '/')
			cstate = s_saw_slash;
		else if (c == '\'')
			cstate = s_char;
		else if (c == '"') {
			cstate = s_string;
			count[em_nstring]++;
		}
		break;
	case s_char:
		if (c == '\'')
			cstate = s_normal;
		else if (c == '\\')
			cstate = s_saw_chr_backslash;
		break;
	case s_string:
		if (c == '"')
			cstate = s_normal;
		else if (c == '\\')
			cstate = s_saw_str_backslash;
		break;
	case s_saw_chr_backslash:
		cstate = s_char;
		break;
	case s_saw_str_backslash:
		cstate = s_string;
		break;
	case s_saw_slash:		// After a / character
		if (c == '/') {
			cstate = s_cpp_comment;
			count[em_nlcomment]++;
		} else if (c == '*') {
			cstate = s_block_comment;
			count[em_nbcomment]++;
		} else
			cstate = s_normal;
		break;
	case s_cpp_comment:		// Inside C++ comment
		if (c == '\n')
			cstate = s_normal;
		else
			count[em_nccomment]++;
		break;
	case s_block_comment:		// Inside C block comment
		if (c == '*')
			cstate = s_block_star;
		count[em_nccomment]++;
		break;
	case s_block_star:		// Found a * in a block comment
		if (c == '/') {
			cstate = s_normal;
			count[em_nccomment] -= 2;	// Don't count the trailing */
		} else if (c != '*')
			cstate = s_block_comment;
		count[em_nccomment]++;
		break;
	default:
		csassert(0);
	}
}

// Adjust class members by n according to the attributes of EC
template <class UnaryFunction>
void
IdCount::add(Eclass *ec, UnaryFunction f)
{
	total = f(total);
	// This counts typedefs as being file scope
	for (int i = attr_begin; i < attr_end; i++)
		if (ec->get_attribute(i))
			count[i] = f(count[i]);
}

// Called for each identifier occurence (all)
void
IdMetricsSummary::add_id(Eclass *ec)
{
	rw[ec->get_attribute(is_readonly)].all.add(ec, add_one());
}

// Called for each unique identifier occurence (EC)
void
IdMetricsSummary::add_unique_id(Eclass *ec)
{
	rw[ec->get_attribute(is_readonly)].once.add(ec, add_one());
	rw[ec->get_attribute(is_readonly)].len.add(ec, add_n(ec->get_len()));
	rw[ec->get_attribute(is_readonly)].maxlen.add(ec, set_max(ec->get_len()));
	rw[ec->get_attribute(is_readonly)].minlen.add(ec, set_min(ec->get_len()));
}

// Returns JSON of Id metrics in form
// {
// 	head: [
// 		"field names",		// e.g Identifier class
// 		...
// 	],
// 	content: [
// 		[
// 			values_for_all_identifers	
// 		],
// 		[
// 			values of identifer
// 		],
// 		...
// 	]
// }
web::json::value to_json(const IdMetricsSet &mi)
{
	web::json::value to_return;
	IdMetricsSet &m = (IdMetricsSet &)mi;
	to_return["head"][0] = web::json::value("Identifier class");
	to_return["head"][1] = web::json::value("Distinct # ids");
	to_return["head"][2] = web::json::value("Total # ids");
	to_return["head"][3] = web::json::value("Avg length");
	to_return["head"][4] = web::json::value("Min length");
	to_return["head"][5] = web::json::value("Max length");

	to_return["content"][0][0] = web::json::value("All identifiers");
	to_return["content"][0][1] = web::json::value(m.once.total);
	to_return["content"][0][2] = web::json::value(m.all.total);
	to_return["content"][0][3] = web::json::value(avg(m.len.total, m.once.total));
	to_return["content"][0][4] = web::json::value(m.minlen.total);
	to_return["content"][0][5] = web::json::value(m.maxlen.total);
	
	int no = 1;
	for (int i = is_readonly + 1; i < attr_end; i++) {
		to_return["content"][no][0] = web::json::value(Attributes::name(i));
		to_return["content"][no][1] = web::json::value(m.once.get_count(i));
		to_return["content"][no][2] = web::json::value(m.all.get_count(i));
		to_return["content"][no][3] = web::json::value(avg(m.len.get_count(i), m.once.get_count(i)));
		to_return["content"][no][4] = web::json::value(m.minlen.get_count(i));
		to_return["content"][no++][5] = web::json::value(m.maxlen.get_count(i));
	}
	return to_return;
}

ostream&
operator<<(ostream& o, const IdMetricsSet &mi)
{
	IdMetricsSet &m = (IdMetricsSet &)mi;

	o << "<table class='metrics'>"
		"<tr><th>" "Identifier class" "</th>"
		"<th>" "Distinct # ids" "</th>"
		"<th>" "Total # ids" "</th>"
		"<th>" "Avg length" "</th>"
		"<th>" "Min length" "</th>"
		"<th>" "Max length" "</th></tr>\n";
	o << "<tr><td>" "All identifiers" "</td>"
		"<td>" << m.once.total << "</td>"
		"<td>" << m.all.total << "</td>"
		"<td>" << avg(m.len.total, m.once.total) << "</td>"
		"<td>" << m.minlen.total << "</td>"
		"<td>" << m.maxlen.total << "</td></tr>\n";
	for (int i = is_readonly + 1; i < attr_end; i++)
		o << "<tr><td>" << Attributes::name(i) << "</td>"
			"<td>" << m.once.get_count(i) << "</td>"
			"<td>" << m.all.get_count(i) << "</td>"
			"<td>" << avg(m.len.get_count(i), m.once.get_count(i)) << "</td>"
			"<td>" << m.minlen.get_count(i) << "</td>"
			"<td>" << m.maxlen.get_count(i) << "</td></tr>\n";
	o << "</table>\n";
	return o;
}

ostream&
operator<<(ostream& o, const IdMetricsSummary &ms)
{
	o << "<h2>Writable Identifiers</h2>\n" << ms.rw[false];
	o << "<h2>Read-only Identifiers</h2>\n" << ms.rw[true];
	return o;
}

string
avg(double v, double n)
{
	if (n == 0.)
		return "-";
	ostringstream r;
	r << v / n;
	return r.str();
}

// Call the specified metrics function for the current file and function
void
Metrics::call_metrics(void (Metrics::*fun)())
{
	(Fchar::get_fileid().metrics().*fun)();
	Call::call_metrics(fun);
}
