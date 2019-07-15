/*
 * (C) Copyright 2001-2019 Diomidis Spinellis
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
 * Web-based interface for viewing and processing C code
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
#include <utility>
#include <functional>
#include <algorithm>		// set_difference
#include <cctype>
#include <sstream>		// ostringstream
#include <cstdio>		// perror, rename
#include <cstdlib>		// atoi
#include <cstring>		// strdup
#include <cerrno>		// errno
#include <regex.h> // regex
#include <wait.h>
#include <getopt.h>



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
#include "idquery.h"
#include "funquery.h"
#include "filequery.h"
#include "logo.h"
#include "pager.h"
#include "html.h"
#include "dirbrowse.h"
#include "fileutils.h"
#include "globobj.h"
#include "fifstream.h"
#include "ctag.h"
#include "timer.h"
#include "headers.h"

#ifdef PICO_QL
#include "pico_ql_search.h"
using namespace picoQL;
#endif

#include "sql.h"
#include "workdb.h"
#include "obfuscate.h"
using namespace web;
#define ids Identifier::ids

#define prohibit_remote_access(fs)
#define prohibit_browsers(fs) \
	do { \
		if (browse_only)  {\
			nonbrowse_operation_prohibited(fs); \
		} \
 	} while (0)


// Global command-line options
static enum e_process {
	pm_unspecified,			// Default (web front-end) must be 0
	pm_preprocess,			// Preprocess-only (-E)
	pm_compile,			// Compile-only (-c)
	pm_report,			// Generate a warning report
	pm_database,
	pm_obfuscation,
	pm_r_option
} process_mode;
static int portno = 8081;		// Port number (-p n)
static char *db_engine;			// Create SQL output for a specific db_iface

// Workspace modification state
static enum e_modification_state {
	ms_unmodified,			// Unmodified; can be modified
	ms_subst,			// An identifier has been substituted; only further substitutions are allowed
	ms_hand_edit			// A file has been hand-edited; only further hand-edits are allowed
} modification_state;

static Fileid input_file_id;

// This uses many of the above, and is therefore included here
#include "gdisplay.h"

// Set to true when the user has specified the application to exit
static bool must_exit = false;
 
// Set to true if we operate in browsing mode
static bool browse_only = false;
// Maximum number of nodes and edges allowed to browsing-only clients
#define MAX_BROWSING_GRAPH_ELEMENTS 1000

static CompiledRE sfile_re;			// Saved files replacement location RE

// Identifiers to monitor (-m parameter)
static IdQuery monitor;

static vector <Fileid> files;

Attributes::size_type current_project;

HttpServer server;
/*
 * A map from an equivallence class to the string
 * specifying the arguments of the corresponding
 * refactored function call (e.g. "@2, @1")
 */
class RefFunCall {
private:
	Call *fun;		// Function
	string repl;		// Replacement patttern
	bool active;		// True if active
public:
	typedef map <Eclass *, RefFunCall> store_type;	// Store of all refactorings
	static store_type store;
	RefFunCall(Call *f, string s) : fun(f), repl(s), active(true) {}
	const Call *get_function() const { return fun; }
	const string &get_replacement() const { return repl; }
	void set_replacement(const string &s) { repl = s; }
	bool is_active() const { return active; }
	void set_active(bool a) { active = a; }
};

RefFunCall::store_type RefFunCall::store;

// Matrix used to store graph edges
typedef vector<vector<bool> > EdgeMatrix;

// Boundaries of a function argument
struct ArgBound {
	streampos start, end;
};

typedef map <Tokid, vector <ArgBound> > ArgBoundMap;
static ArgBoundMap argbounds_map;

// Keep track of the number of replacements made when saving the files
static int num_id_replacements = 0;
static int num_fun_call_refactorings = 0;


// void index_page(FILE *of, void *data);

// Return the page suffix for the select call graph type
static string
graph_suffix()
{
	switch (Option::cgraph_type->get()) {
	case 't': return ".txt";
	case 'h': return ".html";
	case 'd': return "_dot.txt";
	case 's': return ".svg";
	case 'g': return ".gif";
	case 'p': return ".png";
	case 'f': return ".pdf";
	}
	return "";
}

// Display loop progress (non-reentant)
template <typename container>
static void
progress(typename container::const_iterator i, const container &c)
{
	static int count, opercent;

	if (i == c.begin())
		count = 0;
	int percent = ++count * 100 / c.size();
	if (percent != opercent) {
		cerr << '\r' << percent << '%' << flush;
		opercent = percent;
	}
}

// Return html link to identifer page
static string
html(const IdPropElem &i)
{
	ostringstream to_ret;
	to_ret << "<a href=\"id.html?id=" << i.first << "\">" 
		<< html_string((i.second).get_id()) + "</a>";
	return to_ret.str();
}



// Return json value of identifer's address 
static json::value
html_address(const IdPropElem &i)
{
	json::value to_return;
	char* s = new char[20];
	sprintf(s,"%p",i.first);
	to_return = json::value(s); 
	delete s;
	return to_return;
}

// Return html link to function page
static string
html(const Call &c)
{
	ostringstream to_ret;
	to_ret << "<a href=\"fun.html?f=" << &c << "\">"
	<< html_string(c.get_name()) << "</a>";
	return to_ret.str();
}

// Return json value of function's address
static json::value
html_address(const Call &c)
{
	
	json::value to_return;
	char* s = new char[20];
	sprintf(s,"%p",&c);
	to_return = json::value(s);
	delete s;
	return to_return;
}

// Return a hyperlink based on a string and its starting tokid as string
static json::value
html_string(const string &s, Tokid t)
{
	json::value to_return;
	int len = s.length();
	string to_ret;
	int no = 0;
	for (int pos = 0; pos < len;) {
		Eclass *ec = t.get_ec();
		Identifier id(ec, s.substr(pos, ec->get_len()));
		const IdPropElem ip(ec, id);
		to_return["identifiers"][no]["id"] = json::value(ip.first);
		to_return["identifiers"][no]["name"] = json::value((ip.second).get_id());
		to_ret.append(html(ip));
		pos += ec->get_len();
		t += ec->get_len();
		if (pos < len)
			to_ret.append("][");
	}
	to_return["html"] = json::value(to_ret);
	return to_return;
}

// Return html hyperlinks to a function's identifiers
static string
html_string(const Call *f)
{
	int start = 0;
	string to_ret;
	for (dequeTpart::const_iterator i = f->get_token().get_parts_begin(); i != f->get_token().get_parts_end(); i++) {
		Tokid t = i->get_tokid();
		to_ret.append('[' + html_string(f->get_name().substr(start, i->get_len()), t)["string"].as_string() + ']');
		start += i->get_len();
	}
	return to_ret;
}

// Return html hyperlinks to a function's identifiers and data in json
// {
//		string: "html to",
//		data: [
//			identifiers: [
//				{
//					id: "id-address",
//					name: "identifier name"
//				}
//			]
// }
static json::value
html_json(const Call *f)
{
	int start = 0;
	string to_ret;
	json::value to_return;
	int no = 0;
	for (dequeTpart::const_iterator i = f->get_token().get_parts_begin(); i != f->get_token().get_parts_end(); i++) {
		Tokid t = i->get_tokid();
		to_return["data"][no] = html_string(f->get_name().substr(start, i->get_len()), t);
		to_ret.append('[' + to_return["data"][no]["html"].as_string() + ']');
		to_return["data"][no++].erase("html");
		start += i->get_len();
	}
	to_return["string"] = json::value(to_ret);
	return to_return;
}

// Add identifiers of the file fi into ids
// Collect metrics for the file and its functions
// Populate the file's accociated files set
// Return true if the file contains unused identifiers
static bool
file_analyze(Fileid fi)
{
	using namespace std::rel_ops;

	fifstream in;
	bool has_unused = false;
	const string &fname = fi.get_path();
	int line_number = 0;

	FCallSet &fc = fi.get_functions();	// File's functions
	FCallSet::iterator fci = fc.begin();	// Iterator through them
	Call *cfun = NULL;			// Current function
	stack <Call *> fun_nesting;

	cerr << "Post-processing " << fname << endl;
	in.open(fname.c_str(), ios::binary);
	if (in.fail()) {
		perror(fname.c_str());
		exit(1);
	}
	// Go through the file character by character
	for (;;) {
		Tokid ti;
		int val;

		ti = Tokid(fi, in.tellg());
		if ((val = in.get()) == EOF)
			break;

		// Update current_function
		if (cfun && ti > cfun->get_end().get_tokid()) {
			cfun->metrics().summarize_identifiers();
			if (fun_nesting.empty())
				cfun = NULL;
			else {
				cfun = fun_nesting.top();
				fun_nesting.pop();
			}
		}
		if (fci != fc.end() && ti >= (*fci)->get_begin().get_tokid()) {
			if (cfun)
				fun_nesting.push(cfun);
			cfun = *fci;
			fci++;
		}

		char c = (char)val;
		mapTokidEclass::iterator ei;
		enum e_cfile_state cstate = fi.metrics().get_state();
		if (cstate != s_block_comment &&
		    cstate != s_string &&
		    cstate != s_cpp_comment &&
		    (isalnum(c) || c == '_') &&
		    (ei = ti.find_ec()) != ti.end_ec()) {
			Eclass *ec = (*ei).second;
			// Remove identifiers we are not supposed to monitor
			if (monitor.is_valid()) {
				IdPropElem ec_id(ec, Identifier());
				if (!monitor.eval(ec_id)) {
					ec->remove_from_tokid_map();
					delete ec;
					continue;
				}
			}
			// Identifiers we can mark
			if (ec->is_identifier()) {
				// Update metrics
				id_msum.add_id(ec);
				// Add to the map
				string s(1, c);
				int len = ec->get_len();
				for (int j = 1; j < len; j++)
					s += (char)in.get();
				fi.metrics().process_id(s, ec);
				if (cfun)
					cfun->metrics().process_id(s, ec);
				/*
				 * ids[ec] = Identifier(ec, s);
				 * Efficiently add s to ids, if needed.
				 * See Meyers, effective STL, Item 24.
				 */
				IdProp::iterator idi = ids.lower_bound(ec);
				if (idi == ids.end() || idi->first != ec)
					ids.insert(idi, IdProp::value_type(ec, Identifier(ec, s)));
				if (ec->is_unused())
					has_unused = true;
				else
					; // TODO fi.set_associated_files(ec);
				continue;
			} else {
				/*
				 * This equivalence class is not needed.
				 * (All potential identifier tokens,
				 * even reserved words get an EC. These are
				 * cleared here.)
				 */
				ec->remove_from_tokid_map();
				delete ec;
			}
		}
		fi.metrics().process_char((char)val);
		if (cfun)
			cfun->metrics().process_char((char)val);
		if (c == '\n') {
			fi.add_line_end(ti.get_streampos());
			if (!fi.is_processed(++line_number))
				fi.metrics().add_unprocessed();
		}
	}
	if (cfun)
		cfun->metrics().summarize_identifiers();
	fi.metrics().set_ncopies(fi.get_identical_files().size());
	if (DP())
		cout << "nchar = " << fi.metrics().get_metric(Metrics::em_nchar) << endl;
	in.close();
	return has_unused;
}

// Return Json with the contents of a file in hypertext form
// {
//		(IdMsg or FunMsg: "Error Message"),
//		handEd: 1, 							//(exists only if hand edited) 
//		file: [
//			{
//				type:"html"	
//				html:"html code of according file until a link"
//			},
//			{
//				type:"fun" or "id",
//				id: "addres or needed id"
//			}
//		]
//	}
static json::value
file_hypertext( Fileid * fi,bool eval_query)
{
	istream *in;
	const string &fname = (*fi).get_path();
	bool at_bol = true;
	int line_number = 1;
	int mark_unprocessed = 52;

	mark_unprocessed = server.getIntParam("marku");
	json::value to_return;

	/*
	 * In theory this could be handled by adding a class
	 * factory method to Query, and making eval virtual.
	 * In practice the IdQuery and FunQuery eval methods
	 * take incompatible arguments, and are difficult to
	 * reconcile.
	 */
	IdQuery idq;
	FunQuery funq;
	bool have_funq, have_idq;
	const char *qtype = server.getCharPParam("qt");
	if(DP())
	 	cout << "File Hypertext: qtype: " << qtype << endl;
	have_funq = have_idq = false;
	if (qtype == NULL || strcmp(qtype, "id") == 0) 
	{
		idq = IdQuery(Option::file_icase->get(), current_project, eval_query);
		have_idq = true;
		
		if(eval_query)
			if(idq.getError() != NULL) {
				to_return["IdMsg"] = json::value::string(idq.getError());
				if(DP())
			 		cout << "idq Error:" << idq.getError() << endl;
			}
	} 
	else if (strcmp(qtype, "fun") == 0) 
	{
		funq = FunQuery(Option::file_icase->get(), current_project, eval_query);
		have_funq = true;
	
		if(funq.getError() != NULL)
			to_return["FunMsg"] = json::value::string(funq.getError());
	} 
	else 
	{
		to_return["error"] = json::value::string("Unknown query type (try adding &qt=id to the URL).\n");
		delete qtype;
		return to_return;
	}

	if (DP())
		cout << "Write to " << fname << endl;
	if ((*fi).is_hand_edited()) 
	{
		if(DP())
			cout << "Hand Edited" << endl;
		in = new istringstream((*fi).get_original_contents());
		to_return["handEd"] = json::value(1);
	} 
	else {
		if(DP())
			cout << "Not Hand Edited" << endl;
		in = new ifstream(fname.c_str(), ios::binary);
		if (in->fail()) {
			string error_msg("Unable to open " + fname + " for reading" + ": " + string(strerror(errno)) + "\n");
			fputs(error_msg.c_str(), stderr);
			if(DP())
				cout << "error:" << error_msg << endl;
			to_return["error"] = json::value::string(error_msg);
			delete in;
			if(qtype != NULL) delete qtype;
			return to_return;
		}
	}
	
	(void)html('\n');	// Reset HTML tab handling
	// Go through the file character by character
	int no = 0;
	ostringstream file;
	for (;;) {
		Tokid ti;
		int val;
		ti = Tokid(*fi, in->tellg());
		if ((val = in->get()) == EOF)
			break;
		if (at_bol) {
			file << ("<a name=\"" + to_string(line_number) + "\"></a>\n");
			if (mark_unprocessed && !(*fi).is_processed(line_number))
				file << "<span class=\"unused\">";
			if (Option::show_line_number->get()) {
				char buff[50];
				snprintf(buff, sizeof(buff), "%5d ", line_number);
				// Do not go via HTML string to keep tabs ok
				for (char *s = buff; *s; s++)
					if (*s == ' ')
						file << "&nbsp;";
					else
						file << *s;
			}
			at_bol = false;
		}
		// Identifier we can mark
		Eclass *ec;
		if (have_idq && (ec = ti.check_ec()) && ec->is_identifier() && idq.need_eval()) {
			string s;
			s = (char)val;
			int len = ec->get_len();
			for (int j = 1; j < len; j++)
				s += (char)in->get();
			Identifier i(ec, s);
			const IdPropElem ip(ec, i);
		if (idq.eval(ip)){
				file << html(ip);
			}
			else	
				file << html_string(s) << endl;
			continue;
		}
		
		// Function we can mark
		if (have_funq && funq.need_eval()) {
			pair <Call::const_fmap_iterator_type, Call::const_fmap_iterator_type> be(Call::get_calls(ti));
			Call::const_fmap_iterator_type ci;
			for (ci = be.first; ci != be.second; ci++)
				if (funq.eval(ci->second)) {
					string s;
					s = (char)val;
					int len = ci->second->get_name().length();
					for (int j = 1; j < len; j++)
						s += (char)in->get();
					file << html(*(ci->second));
					break;
				}
			if (ci != be.second)
				continue;
		}
		
		file << html((char)val);
		if ((char)val == '\n') {
			at_bol = true;
			if (mark_unprocessed && !(*fi).is_processed(line_number))
				file << "</span>";
			
			line_number++;
		}
	}
	delete in;
	if(qtype != NULL) delete qtype;
	to_return["file"] = json::value(file.str());
	return to_return;
}


// Set the function argument boundaries for refactored
// function calls for the specified file
static void
establish_argument_boundaries(const string &fname)
{
	Pltoken::set_context(cpp_normal);
	Fchar::set_input(fname);

	for (;;) {
		Pltoken t;
again:
		t.getnext<Fchar>();
		if (t.get_code() == EOF)
			break;

		Tokid ti;
		Eclass *ec;
		RefFunCall::store_type::const_iterator rfc;
		if (t.get_code() == IDENTIFIER &&
		    (ti = t.get_parts_begin()->get_tokid(), ec = ti.check_ec()) &&
		    (rfc = RefFunCall::store.find(ec)) != RefFunCall::store.end() &&
		    rfc->second.is_active() &&
		    (int)t.get_val().length() == ec->get_len()) {
			Tokid call = t.get_parts_begin()->get_tokid();
			// Gather args
			FcharContext fc = Fchar::get_context();		// Save position to scan for another function
			// Move just before the first arg
			for (;;) {
				t.getnext<Fchar>();
				if (t.get_code() == EOF) {
					/*
					 * @error
					 * End of file encountered while scanning the opening
					 * bracket in a refactored function call
					 */
					Error::error(E_WARN, "Missing open bracket in refactored function call");
					break;
				}
				if (t.get_code() == '(')
					break;
				if (!isspace(t.get_code())) {
					Fchar::set_context(fc);		// Restore saved position
					goto again;
				}
			}
			// Gather the boundaries of all arguments of a single function
			vector <ArgBound> abv;
			for (;;) {
				ArgBound ab;
				// Set position of argument's first token part the delimiter read
				ab.start = t.get_delimiter_tokid().get_streampos();
				ab.start += 1;
				if (DP())
					cerr << "arg.start: " << ab.start << endl;
				t.getnext<Fchar>();	// We just read the delimiter; move past it
				int bracket = 0;
				// Scan to argument's end
				for (;;) {
					if (bracket == 0 && (t.get_code() == ',' || t.get_code() == ')')) {
						// End of arg
						ab.end = t.get_delimiter_tokid().get_streampos();
						abv.push_back(ab);
						if (DP())
							cerr << "arg.end: " << ab.end << endl;
						if (t.get_code() == ')') {
							// Done with this call
							argbounds_map.insert(pair<ArgBoundMap::key_type, ArgBoundMap::mapped_type>(call, abv));
							if (DP())
								cerr << "Finish function" << endl;
							Fchar::set_context(fc);		// Restore saved position
							goto again;			// Scan again from the token following the function's name
						}
						break;	// Next argument
					}
					switch (t.get_code()) {
					case '(':
						bracket++;
						break;
					case ')':
						bracket--;
						break;
					case EOF:
						/*
						 * @error
						 * The end of file was reached while
						 * gathering a refactored function call's arguments
						 */
						Error::error(E_WARN, "EOF while reading refactored function call arguments");
						Fchar::set_context(fc);
						goto again;
					}
					t.getnext<Fchar>();
				}
			}
			/* NOTREACHED */
		} // If refactored function call
	} // For all the file's tokens
}

// Trim whitespace at the left of the string
static string
ltrim(const string &s)
{
	string::const_iterator i;

	for (i = s.begin(); i != s.end(); i++)
		if (!isspace(*i))
			break;
	return string(i, s.end());
}

/*
 * Return (through the out params n and op) the value of
 * an @ template replacement operator and the corresponding modifier.
 * Update i to point to the first character after the @ sequence.
 * For conditional modifiers (+ and -) set b2 and e2 to the enclosed text,
 * otherwise set the two to point to end.
 * Return true if the operator's syntax is correct, false if not.
 * Set error to the corresponding error message.
 */
static bool
parse_function_call_replacement(string::const_iterator &i, string::const_iterator end, vector<string>::size_type &n,
    char &mod, string::const_iterator &b2, string::const_iterator &e2, const char **error)
{
	if (DP())
		cerr << "Scan replacement \"" << string(i, end) << '"' << endl;
	csassert(*i == '@');
	if (++i == end) {
		*error = "Missing argument to @";
		return false;
	}
	switch (*i) {
	case '.':
	case '+':
	case '-':
		mod = *i++;
		break;
	default:
		mod = '=';
		break;
	}
	int val, nchar;
	if (DP())
		cerr << "Scan number \"" << string(i, end) << '"' << endl;
	int nconv = sscanf(string(i, end).c_str(), "%d%n", &val, &nchar);
	if (nconv != 1 || val <= 0) {
		*error = "Invalid numerical value to @";
		return false;
	}
	i += nchar;
	n = (unsigned)val;
	if (mod == '+' || mod == '-') {
		// Set b2, e2 to the limits argument in braces and update i past them
		if (*i != '{') {
			*error = "Missing opening brace";
			return false;
		}
		b2 = i + 1;
		int nbrace = 0;
		for (; i != end; i++) {
			if (*i == '{') nbrace++;
			if (*i == '}') nbrace--;
			if (nbrace == 0)
				break;
		}
		if (i == end) {
			*error = "Non-terminated argument in braces (missing closing brace)";
			return false;
		} else
			e2 = i++;
		if (DP())
			cerr << "Enclosed range: \"" << string(b2, e2) << '"' << endl;
	} else
		b2 = e2 = end;
	if (DP())
		cerr << "nchar= " << nchar << " val=" << n << " remain: \"" << string(i, end) << '"' << endl;
	csassert(i <= end);
	*error = "No error";
	return true;
}


/*
 * Return true if a function call substitution string in the range is valid
 * Set error to the corresponding error value
 */
static bool
is_function_call_replacement_valid(string::const_iterator begin, string::const_iterator end, const char **error)
{
	if (DP())
		cerr << "Call valid for \"" << string(begin, end) << '"' << endl;
	for (string::const_iterator i = begin; i != end;)
		if (*i == '@') {
			vector<string>::size_type n;
			char modifier;
			string::const_iterator b2, e2;
			if (!parse_function_call_replacement(i, end, n, modifier, b2, e2, error))
				return false;
			if ((b2 != e2) && !is_function_call_replacement_valid(b2, e2, error))
			    	return false;
		} else
			i++;
	return true;
}

/*
 * Set arguments in the new order as specified by the template begin..end
 * @N  : use argument N
 * @.N : use argument N and append a comma-separated list of all arguments following N
 * @+N{...} : if argument N exists, substitute text in braces
 * @-N{...} : if argument N doesn't exist, substitute text in braces
 */
string
function_argument_replace(string::const_iterator begin, string::const_iterator end, const vector <string> &args)
{
	string ret;

	for (string::const_iterator i = begin; i != end;)
		if (*i == '@') {
			vector<string>::size_type n;
			char modifier;
			string::const_iterator b2, e2;
			const char *error;
			bool valid = parse_function_call_replacement(i, end, n, modifier, b2, e2, &error);
			csassert(valid);
			switch (modifier) {
			case '.':	// Append comma-separated varargs
				if (n <= args.size())
					ret += ltrim(args[n - 1]);
				for (vector<string>::size_type j = n; j < args.size(); j++) {
					ret += ", ";
					ret += ltrim(args[j]);
				}
				break;
			case '+':	// if argument N exists, substitute text in braces
				if (n <= args.size())
					ret += function_argument_replace(b2, e2, args);
				break;
			case '-':	// if argument N doesn't exist, substitute text in braces
				if (n > args.size())
					ret += function_argument_replace(b2, e2, args);
				break;
			case '=':	// Exact argument
				if (n <= args.size())
					ret += ltrim(args[n - 1]);
				break;
			}
		} else
			ret += *i++;
	return ret;
}

/*
 * Return the smallest part of the file that can be chunked
 * without renaming or having to reorder function arguments.
 * Otherwise, return the part suitably renamed and with the
 * function arguments reordered.
 */
static string
get_refactored_part(fifstream &in, Fileid fid)
{
	Tokid ti;
	int val;
	string ret;

	ti = Tokid(fid, in.tellg());
	if ((val = in.get()) == EOF)
		return ret;
	Eclass *ec;

	// Identifiers that should be replaced
	IdProp::const_iterator idi;
	if ((ec = ti.check_ec()) &&
	    ec->is_identifier() &&
	    (idi = ids.find(ec)) != ids.end() &&
	    idi->second.get_replaced() &&
	    idi->second.get_active()) {
		int len = ec->get_len();
		for (int j = 1; j < len; j++)
			(void)in.get();
		ret += (*idi).second.get_newid();
		num_id_replacements++;
	} else
		ret = (char)val;

	// Functions whose arguments need reordering
	RefFunCall::store_type::const_iterator rfc;
	ArgBoundMap::const_iterator abi;
	if ((ec = ti.check_ec()) &&
	    ec->is_identifier() &&
	    (rfc = RefFunCall::store.find(ec)) != RefFunCall::store.end() &&
	    rfc->second.is_active() &&
	    (abi = argbounds_map.find(ti)) != argbounds_map.end()) {
		const ArgBoundMap::mapped_type &argbounds = abi->second;
		csassert (in.tellg() < argbounds[0].start);
		// Gather material until first argument
		while (in.tellg() < argbounds[0].start)
			ret += get_refactored_part(in, fid);
		// Gather arguments
		vector<string> arg(argbounds.size());
		for (ArgBoundMap::mapped_type::size_type i = 0; i < argbounds.size(); i++) {
			while (in.tellg() < argbounds[i].end)
				arg[i] += get_refactored_part(in, fid);
			int endchar = in.get();
			if (DP())
			cerr << "arg[" << i << "] = \"" << arg[i] << "\" endchar: '" << (char)endchar << '\'' << endl;
			csassert ((i == argbounds.size() - 1 && endchar == ')') ||
			    (i < argbounds.size() - 1 && endchar == ','));
		}
		ret += function_argument_replace(rfc->second.get_replacement().begin(), rfc->second.get_replacement().end(), arg);
		ret += ')';
		num_fun_call_refactorings++;
	} // Replaced function call
	return ret;
}

// 	Go through the file doing any refactorings needed returns ok or error
// 	{
//		error: "Error message" or ok: "success message"
//	}
static json::value
file_refactor(Fileid fid)
{
	string plain;
	fifstream in;
	ofstream out;
	json::value to_return;
	cerr << "Processing file " << fid.get_path() << endl;

	if (RefFunCall::store.size())
		establish_argument_boundaries(fid.get_path());
	in.open(fid.get_path().c_str(), ios::binary);
	if (in.fail()) {
		to_return["error"] = json::value::string("Unable to open " 
		+ fid.get_path() + " for reading");
		return to_return;
	}
	string ofname(fid.get_path() + ".repl");
	out.open(ofname.c_str(), ios::binary);
	if (out.fail()) {
		to_return["error"] = json::value::string( "Unable to open " 
		+ ofname + " for writing");
		return to_return;
	}

	while (!in.eof())
		out << get_refactored_part(in, fid);
	argbounds_map.clear();

	// Needed for Windows
	in.close();
	out.close();

	if (Option::sfile_re_string->get().length()) {
		regmatch_t be;
		if (sfile_re.exec(fid.get_path().c_str(), 1, &be, 0) == REG_NOMATCH ||
		    be.rm_so == -1 || be.rm_eo == -1)
			 to_return["ok"] = json::value::string("File " + ofname + " does not match file replacement RE."
				"Replacements will be saved in " + ofname + ".repl.\n");
		else {
			string newname(fid.get_path().c_str());
			newname.replace(be.rm_so, be.rm_eo - be.rm_so, Option::sfile_repl_string->get());
			string cmd("cscout_checkout " + newname);
			if (system(cmd.c_str()) != 0) {
				to_return["error"] = json::value::string("Changes are saved in " 
				+ ofname + ", because executing the checkout command cscout_checkout failed");
				return to_return;
			}
			if (unlink(newname) < 0) {
				to_return["error"] = json::value::string("Changes are saved in " + ofname + ", because deleting the target file " + newname + " failed");
				return to_return;
			}
			if (rename(ofname.c_str(), newname.c_str()) < 0) {
				to_return["error"] = json::value::string("Changes are saved in " + ofname + ", because renaming the file " + ofname + " to " + newname + " failed");
				return to_return;
			}
			string cmd2("cscout_checkin " + newname);
			if (system(cmd2.c_str()) != 0) {
				to_return["error"] = json::value::string( "Checking in the file " + newname + " failed");
				return to_return;
			}
		}
	} else {
		string cmd("cscout_checkout " + fid.get_path());
		if (system(cmd.c_str()) != 0) {
			to_return["error"] = json::value::string( "Changes are saved in " + ofname + ", because checking out " + fid.get_path() + " failed");
			return to_return;
		}
		if (unlink(fid.get_path()) < 0) {
			to_return["error"] = json::value::string( "Changes are saved in " + ofname + ", because deleting the target file " + fid.get_path() + " failed");
			return to_return;
		}
		if (rename(ofname.c_str(), fid.get_path().c_str()) < 0) {
			to_return["error"] = json::value::string( "Changes are saved in " + ofname + ", because renaming the file " + ofname + " to " + fid.get_path() + " failed");
			return to_return;
		}
		string cmd2("cscout_checkin " + fid.get_path());
		if (system(cmd2.c_str()) != 0) {
			to_return["error"] = json::value::string("Checking in the file " + fid.get_path() + " failed");
			return to_return;
		}
	}
	return to_return;
}

// Return message in json::value::string
static json::value
change_prohibited()
{
	string to_ret;
	to_ret = "Identifier substitutions or function argument refactoring "
	"are not allowed to be performed together with and the hand-editing "
	"of files within the same CScout session.";
	return json::value::string(to_ret);
	
}


static void
nonbrowse_operation_prohibited(std::ostringstream *fs)
{
	(*fs) << "This is a multiuser browse-only CScout session."
		"Non-browsing operations are disabled.";
}

// Call before the start of a file list
static string
html_file_begin()
{
	if (Option::fname_in_context->get())
		return "<table class='dirlist'><tr><th>Directory</th><th>File</th>";
	else
		return "<table><tr><th></th><th></th>";
}

// Call before actually listing files (after printing additional headers)
static string
html_file_set_begin()
{
	return "</tr>\n";
}

// Called after html_file (after printing additional columns)
static string
html_file_record_end()
{
	return "</tr>\n";
}

// Called at the end
static string
html_file_end()
{
	return "</table>\n";
}

// Return a filename of an html file as string
static string
html_file(Fileid fi)
{
	if (!Option::fname_in_context->get()) {
		return("\n<tr><td></td><td><a href=\"file.html?id=" + to_string(fi.get_id()) + "\">"
		+ fi.get_path() + "</a></td>");
	}

	// Split path into dir and fname
	string s(fi.get_path());
	string::size_type k = s.find_last_of("/\\");
	if (k == string::npos)
		k = 0;
	else
		k++;
	string dir(s, 0, k);
	string fname(s, k);

	return "<tr><td align=\"right\">" + dir + "\n</td>\n"
		"<td><a href=\"file.html?id=" + to_string(fi.get_id()) + "\">"
		+ fname.c_str() + "</a></td>";
}

// File query page
// return filequery html page split in a JSON
// {
//		FileQuery: "html of form action and start",
//		mquery: "html of metrics query form",
//		inputs: "html of inputs and end form "
// }
static json::value
filequery_page(void *p)
{
	json::value to_return;
	to_return["FileQuery"] = json::value::string("<FORM ACTION=\"xfilequery.html\" METHOD=\"GET\">\n"
	"<input type=\"checkbox\" name=\"writable\" value=\"1\">Writable<br>\n"
	"<input type=\"checkbox\" name=\"ro\" value=\"1\">Read-only<br>\n",true);
	to_return["mquery"] = MQuery<FileMetrics, Fileid &>::metrics_query_form();
	to_return["inputs"] = json::value::string("<p>"
	"<input type=\"radio\" name=\"match\" value=\"Y\" CHECKED>Match any of the above\n"
	"&nbsp; &nbsp; &nbsp; &nbsp;\n"
	"<input type=\"radio\" name=\"match\" value=\"L\">Match all of the above\n"
	"<br><hr>\n"
	"File names should "
	"(<input type=\"checkbox\" name=\"xfre\" value=\"1\"> not) \n"
	" match RE\n"
	"<INPUT TYPE=\"text\" NAME=\"fre\" SIZE=20 MAXLENGTH=256>\n"
	"<hr>\n"
	"<p>Query title <INPUT TYPE=\"text\" NAME=\"n\" SIZE=60 MAXLENGTH=256>\n"
	"&nbsp;&nbsp;<INPUT TYPE=\"submit\" NAME=\"qf\" VALUE=\"Show files\">\n"
	"</FORM>\n", true
	);
	return to_return;
}

struct ignore : public binary_function <int, int, bool> {
	inline bool operator()(int a, int b) const { return true; }
};


// Process a file query and return json
// {
//		xfilequery: "title",
//		table: {
//			h: "html table header start"
//			h1: "html of next header",				// not always
//			h2: "html of metrics header table" 		// not always
//			hend: "html of table end",
//			contents : [
//				"html of table rows"
//			]		
//		},
//		mname: "name of metric",
//		file: [
// 			{
//				id: file id,
//				name: file path,
//				metric: file query metric	// not always
//			},
//		],
//		timer: "time elapsed calculating the query"
//	}
static json::value
xfilequery_page(void *p)
{
	Timer timer;
	json::value to_return;
	const char *qname = server.getCharPParam("n");
	int pageSize = server.getIntParam("ps");
	if(pageSize == -1){
		pageSize = Option::entries_per_page->get();
	}

	std::ostringstream fs;
	FileQuery query(&fs, Option::file_icase->get(), current_project);

	if(!(fs.str().empty()))
		to_return["Xerror"] = json::value::string(fs.str());
	if (!query.is_valid()) {
		to_return["error"] = json::value::string("Non valid query");
		if(qname!=NULL) delete qname;
		return to_return;
	}
	multiset <Fileid, FileQuery::specified_order> sorted_files;
	to_return["xfilequery"] = json::value::string((qname && *qname) ? qname : "File Query Results");

	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		if (query.eval(*i))
			sorted_files.insert(*i);
	}

	if (query.get_sort_order() != -1) {
		to_return["mname"] = json::value::string(Metrics::get_name<FileMetrics>(query.get_sort_order()));
		;
	}

	Pager pager(pageSize, query.base_url(), query.bookmarkable());	
		
	
	fs.flush();
	int no = 0;
	to_return["metric"] = json::value(query.get_sort_order() != -1);
	for (multiset <Fileid, FileQuery::specified_order>::iterator i = sorted_files.begin(); i != sorted_files.end(); i++) {
		Fileid f = *i;
		if (current_project && !f.get_attribute(current_project))
			continue;
		//if (pager.show_next()) {
			fs << html_file(*i);
			to_return["file"][no]["id"] = json::value(i->get_id());
			size_t t = (i->get_path()).find_last_of('/');
			to_return["file"][no]["name"] = json::value::string((i->get_path()).substr(t+1));
			to_return["file"][no]["path"] = json::value::string((i->get_path()).substr(0,t));
			if (modification_state != ms_subst && !browse_only)
				fs << "<td><a href=\"fedit.html?id=" << to_string(i->get_id()) << "\">edit</a></td>";
			if (query.get_sort_order() != -1) {
				fs << "<td align=\"right\">" << to_string(i->const_metrics().get_metric(query.get_sort_order())) 
					<< "</td>";
				to_return["file"][no]["metric"] = json::value(i->const_metrics().get_metric(query.get_sort_order()));
			}
			fs << html_file_record_end();
			fs.flush();
			no++;
		//}
	}
	
	to_return["timer"] = json::value::string(timer.print_elapsed(),true);
	
	if(qname!=NULL) delete qname;
	return to_return;
}


/*
 * Return the sorted identifiers or functions, taking into account the reverse sort property
 * for properly aligning the output as json.
 * {
 * 		start: "start of html code",
 * 		html: [
 * 			"html code of buttons to sorted_ids elemets"
 * 		],
 * 		end: "end of html code",
 * 		address: [
 * 			"memory address of sorted ids elements"
 * 		]
 * }
 */

template <typename container>
static json::value
display_sorted(const Query &query, const container &sorted_ids)
{
	json::value to_return;
	
	if (Option::sort_rev->get())
		to_return["start"] = json::value::string("<table><tr><td width=\"50%\" align=\"right\">\n");
	else
		to_return["start"] = json::value::string("<p>\n");

	Pager pager(Option::entries_per_page->get(), query.base_url() + "&qi=1", query.bookmarkable());
	typename container::const_iterator i;
	int no = 0;

	for (i = sorted_ids.begin(); i != sorted_ids.end(); i++) {
		if (pager.show_next()) {
			to_return["address"][no] = html_address(**i);			
			to_return["html"][no++] = json::value::string(html(**i));
		}
	}
	if (Option::sort_rev->get())
		to_return["end"] = json::value::string("</td> <td width=\"50%\"> </td></tr></table>\n",true);
	else
		to_return["end"] = json::value::string("</p>\n",true);
	return to_return;
}

/*
 * Return the sorted functions with their metrics,
 * taking into account the reverse sort property
 * for properly aligning the output as JSON.
 * {
 * 		start: "start of html code",
 * 		html: [
 * 			"html code of buttons to sorted_ids elements"
 * 		],
 * 		end: "end of html code",
 * 		mname: "name of metric",
 * 		address: [
 * 			"memory address of sorted_ids elements"
 * 		],
 * 		metric: [
 * 			"metrics"	
 * 		]
 * 	}
 */
static json::value
display_sorted_function_metrics(const FunQuery &query, const Sfuns &sorted_ids)
{
	json::value to_return;
	to_return["start"] = json::value::string("<table class=\"metrics\"><tr>"
	    "<th width='50%%' align='left'>Name</th>"
	    "<th width='50%%' align='right'>" +
		Metrics::get_name<FunMetrics>(query.get_sort_order())
		+ "</th>\n",true);
	to_return["mname"] = json::value(Metrics::get_name<FunMetrics>(query.get_sort_order()));
	Pager pager( Option::entries_per_page->get(), query.base_url() + "&qi=1", query.bookmarkable());
	int no = 0;
	char * s = new char[20];
	for (Sfuns::const_iterator i = sorted_ids.begin(); i != sorted_ids.end(); i++) {
		if (pager.show_next()) {
			sprintf(s, "%p", &(**i));
			to_return["address"][no] = json::value(s);
			s[0] = 0;
			to_return["metric"][no] = json::value((*i)->const_metrics().get_metric(query.get_sort_order()));
			to_return["html"][no++] = json::value::string("<tr><td witdh='50%'>" +
				html(**i) +
				"</td><td witdh='50%%' align='right'>" +
				to_string((*i)->const_metrics().get_metric(query.get_sort_order()))
				+ "</td></tr>\n", true);
		}
	}
	to_return["end"] = json::value::string("</table>\n",true);
	delete s;
	return to_return;
}


// Return Identifier query page split in JSON
// {
//		form: "html code of form start",
//		input:[
//			"html code of checkboxes"	
//		],
//		restIn: "html of other inputs",
//		table: "html code of input table",
// 		end: "html code of end",
//		attributes:[
//			"attribute names"	
//		]
// }
static json::value
iquery_page(void *p)
{
	json::value to_return;
	to_return["form"] = json::value::string("<FORM ACTION=\"xiquery.html\""
		" METHOD=\"GET\">\n"
		"<input type=\"checkbox\" name=\"writable\" value=\"1\">Writable<br>\n", true);
	int i;
	for (i = attr_begin; i < attr_end; i++) {
		to_return["attributes"][i] = json::value(Attributes::name(i));
		to_return["input"][i] = json::value::string("<input type=\"checkbox\" name=\"a"
		+ to_string(i) + "\" value=\"1\">" + Attributes::name(i) + "<br>\n", true);
	}
	to_return["restIn"] = json::value::string("<input type=\"checkbox\" name=\"xfile\" value=\"1\">Crosses file boundary<br>\n"
		"<input type=\"checkbox\" name=\"unused\" value=\"1\">Unused<br>\n"
		"<p>\n"
		"<input type=\"radio\" name=\"match\" value=\"Y\" CHECKED>Match any marked\n"
		"&nbsp; &nbsp; &nbsp; &nbsp;\n"
		"<input type=\"radio\" name=\"match\" value=\"L\">Match all marked\n"
		"&nbsp; &nbsp; &nbsp; &nbsp;\n"
		"<input type=\"radio\" name=\"match\" value=\"E\">Exclude marked\n"
		"&nbsp; &nbsp; &nbsp; &nbsp;\n"
		"<input type=\"radio\" name=\"match\" value=\"T\" >Exact match\n"
		"<br><hr>\n", true);
	to_return["table"] = json::value::string("<table>\n"
		"<tr><td>\n"
		"Identifier names should "
		"(<input type=\"checkbox\" name=\"xire\" value=\"1\"> not) \n"
		" match RE\n"
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"ire\" SIZE=20 MAXLENGTH=256>\n"
		"</td></tr>\n"
		"<tr><td>\n"
		"Select identifiers from filenames "
		"(<input type=\"checkbox\" name=\"xfre\" value=\"1\"> not) \n"
		" matching RE\n"
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"fre\" SIZE=20 MAXLENGTH=256>\n"
		"</td></tr>\n"
		"</table>\n", true);
	to_return["end"] = json::value::string(
		"<INPUT TYPE=\"text\" NAME=\"n\" SIZE=60 MAXLENGTH=256>\n"
		"&nbsp;&nbsp;<INPUT TYPE=\"submit\" NAME=\"qi\" VALUE=\"Show identifiers\">\n"
		"<INPUT TYPE=\"submit\" NAME=\"qf\" VALUE=\"Show files\">\n"
		"<INPUT TYPE=\"submit\" NAME=\"qfun\" VALUE=\"Show functions\">\n"
		"</FORM>", true);
	return to_return;
}

// 	Function query page split in JSON
//	{
//		form: "html code of form start",
//		input:[
//			"html code of radio buttons"	
//		],
//		restIn: "html of other inputs",
//		table: "html code of input table",
// 		end: "html code of end",
//		mquery: {
//			metrics_query_form return object
//		},			
//		select: "query equality selection"
// }
static json::value
funquery_page(void *p)
{
	json::value to_return;
	to_return["form"] = json::value::string("<FORM ACTION=\"xfunquery.html\" METHOD=\"GET\">\n"
		"<input type=\"checkbox\" name=\"cfun\" value=\"1\">C function<br>\n"
		"<input type=\"checkbox\" name=\"macro\" value=\"1\">Function-like macro<br>\n"
		"<input type=\"checkbox\" name=\"writable\" value=\"1\">Writable declaration<br>\n"
		"<input type=\"checkbox\" name=\"ro\" value=\"1\">Read-only declaration<br>\n"
		"<input type=\"checkbox\" name=\"pscope\" value=\"1\">Project scope<br>\n"
		"<input type=\"checkbox\" name=\"fscope\" value=\"1\">File scope<br>\n"
		"<input type=\"checkbox\" name=\"defined\" value=\"1\">Defined<br>\n");
	to_return["mquery"] = MQuery<FunMetrics, Call &>::metrics_query_form();
	to_return["select"] = json::value(Query::equality_selection());
	to_return["input"] = json::value::string(
		"<p>"
		"<input type=\"radio\" name=\"match\" value=\"Y\" CHECKED>Match any marked\n"
		"&nbsp; &nbsp; &nbsp; &nbsp;\n"
		"<input type=\"radio\" name=\"match\" value=\"L\">Match all marked\n"
		"&nbsp; &nbsp; &nbsp; &nbsp;\n"
		"<input type=\"radio\" name=\"match\" value=\"E\">Exclude marked\n"
		"&nbsp; &nbsp; &nbsp; &nbsp;\n"
		"<input type=\"radio\" name=\"match\" value=\"T\" >Exact match\n"
		"<br><hr>\n");
	to_return["table"] = json::value::string(
		"<table>\n"
		"<tr><td>\n"
		"Number of direct callers\n"
		"<select name=\"ncallerop\" value=\"1\">\n"
		+ Query::equality_selection() +
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"ncallers\" SIZE=5 MAXLENGTH=10>\n"
		"</td><td>\n"

		"<tr><td>\n"
		"Function names should "
		"(<input type=\"checkbox\" name=\"xfnre\" value=\"1\"> not) \n"
		" match RE\n"
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"fnre\" SIZE=20 MAXLENGTH=256>\n"
		"</td></tr>\n"

		"<tr><td>\n"
		"Names of calling functions should "
		"(<input type=\"checkbox\" name=\"xfure\" value=\"1\"> not) \n"
		" match RE\n"
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"fure\" SIZE=20 MAXLENGTH=256>\n"
		"</td></tr>\n"

		"<tr><td>\n"
		"Names of called functions should "
		"(<input type=\"checkbox\" name=\"xfdre\" value=\"1\"> not) \n"
		" match RE\n"
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"fdre\" SIZE=20 MAXLENGTH=256>\n"
		"</td></tr>\n"

		"<tr><td>\n"
		"Select functions from filenames "
		"(<input type=\"checkbox\" name=\"xfre\" value=\"1\"> not) \n"
		" matching RE\n"
		"</td><td>\n"
		"<INPUT TYPE=\"text\" NAME=\"fre\" SIZE=20 MAXLENGTH=256>\n"
		"</td></tr>\n"
		"</table>\n");
	to_return["end"] = json::value::string("<hr>\n"
		"<p>Query title <INPUT TYPE=\"text\" NAME=\"n\" SIZE=60 MAXLENGTH=256>\n"
		"&nbsp;&nbsp;<INPUT TYPE=\"submit\" NAME=\"qi\" VALUE=\"Show functions\">\n"
		"<INPUT TYPE=\"submit\" NAME=\"qf\" VALUE=\"Show files\">\n"
		"</FORM>\n"
	);
	return to_return;
}

// Returns file information and links in JSON
// {
// 		start: "html code of start",
//		html: "main html code with links and info",
//		end: "html code of end",
//		files: [
//			{
//				id: file_id(int),
//				path: "file path"
//			}
//		]
// }		
json::value
display_files(const Query &query, const IFSet &sorted_files)
{
	const string query_url(query.param_url());
	json::value to_return;
	
	to_return["start"] = json::value::string(html_file_begin() + html_file_set_begin());
	Pager pager(Option::entries_per_page->get(), query.base_url() + "&qf=1", query.bookmarkable());
	ostringstream fs;
	int no = 0;
	to_return["query"] = json::value(query_url);
	for (IFSet::iterator i = sorted_files.begin(); i != sorted_files.end(); i++) {
		Fileid f = *i;
		if (current_project && !f.get_attribute(current_project))
			continue;
		if (pager.show_next()) {
			to_return["files"][no]["id"] = json::value(f.get_id());
			to_return["files"][no]["path"] = json::value(f.get_path());		
			fs << html_file(*i);
			fs << "<td><a href=\"qsrc.html?id=" << f.get_id() << "&"
			<< query_url << "\">marked source</a></td>";
			if (modification_state != ms_subst && !browse_only)
				fs << "<td><a href=\"fedit.html?id=" << f.get_id() << "\">edit</a></td>";
			fs << html_file_record_end();
		}
	}
	to_return["html"] = json::value::string(fs.str());
	to_return["end"] = json::value::string(html_file_end());
	return to_return;
}

// Process an identifier query and return as JSON
// {
//		qname: "name of identifier query page",
//		(
//			ids: {
//				display_sorted return object without addresses
//			},
//			id: [
//				"addresses"
//			]
//		) or 
//		(
//			files: {
//				display_files return JSON
//			}
//		) or
//		(
//			funs: {
//				display_sorted return object without addresses
//			},
//			f: [
//				"addresses"
//			]
//		)
// }
static json::value 
xiquery_page(void * p)
{
	Timer timer;
	json::value to_return;
	std::ostringstream fs;
	prohibit_remote_access(&fs);
	if(!fs.str().empty()) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	} 

	Sids sorted_ids;
	IFSet sorted_files;
	set <Call *> funs;
	bool q_id = !!server.getBoolParam("qi");	// Show matching identifiers
	bool q_file = !!server.getBoolParam("qf");	// Show matching files
	bool q_fun = !!server.getBoolParam("qfun");	// Show matching functions
	const char *qname = server.getCharPParam("n");
	IdQuery query(Option::file_icase->get(), current_project);

	if (!query.is_valid()) {
		to_return["error"] = json::value::string("Invalid query");
		if(qname!=NULL) delete qname;
		return to_return;
	}

	to_return["qname"] = json::value::string((qname && *qname) ? qname : "Identifier Query Results");
	if(DP())
		cout << "Evaluating identifier query" << endl;
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		if (!query.eval(*i))
			continue;
		if (q_id) {
			sorted_ids.insert(&*i);
		}
		else if (q_file) {
			IFSet f = i->first->sorted_files();
			sorted_files.insert(f.begin(), f.end());
		} else if (q_fun) {
			set <Call *> ecfuns(i->first->functions());
			funs.insert(ecfuns.begin(), ecfuns.end());
		}
	}
	
	if (q_id) {
		to_return["ids"] = display_sorted(query, sorted_ids);
		to_return["id"] = to_return["ids"]["address"];
		to_return["ids"].erase("address");		
	}	
	if (q_file)
		to_return["files"] = display_files(query, sorted_files);
	if (q_fun) {
		fputs("<h2>Matching Functions</h2>\n", stdout);
		Sfuns sorted_funs;
		sorted_funs.insert(funs.begin(), funs.end());
		to_return["funs"] = display_sorted(query, sorted_funs);
		to_return["f"] = to_return["funs"]["address"];
		to_return["funs"].erase("address");
	}

	to_return["timer"] = json::value::string(timer.print_elapsed());
	if(DP())
		cout << "xiquery:" << to_return.serialize() << endl;
	if(qname!=NULL) 
		delete qname;
	return to_return;
}

// Process a function query and return as JSON
// {
//		qname: "name of identifier query page",
//		(
//			files: {
//				display_files return JSON
//			}
//		) or
//		(
//			funs: {
//				display_sorted return object without addresses
//			},
//			f: [
//				"addresses"
//			]
//		)
// }
static json::value
xfunquery_page(void *p)
{
	ostringstream fs;
	json::value to_return;
	prohibit_remote_access(&fs);
	if(!fs.str().empty()) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	} 
	Timer timer;

	Sfuns sorted_funs;
	IFSet sorted_files;

	bool q_id =!!server.getBoolParam("qi");
	bool q_file = !!server.getBoolParam("qf");	// Show matching files
	const char *qname = server.getCharPParam("n");
	FunQuery query(NULL, Option::file_icase->get(), current_project);
	if (!query.is_valid()) {
		to_return["error"] = json::value::string("Invalid Query");
		if(qname!=NULL) 
			delete qname;
		return to_return;
	}
		

	if(qname && *qname)
		to_return["qname"] = json::value::string(qname);
	if(DP())
		std::cerr << "Evaluating function query" << endl;
	for (Call::const_fmap_iterator_type i = Call::fbegin(); i != Call::fend(); i++) {
		progress(i, Call::functions());
		if (!query.eval(i->second))
			continue;
		if (q_id)
			sorted_funs.insert(i->second);
		if (q_file)
			sorted_files.insert(i->second->get_fileid());
	}
	if (q_id) {
		if (query.get_sort_order() != -1)
			to_return["funs"] = display_sorted_function_metrics(query, sorted_funs);
		else {
			to_return["funs"] = display_sorted(query, sorted_funs);
		}
		to_return["f"] = to_return["funs"]["address"];
		to_return["funs"].erase("address");
	}
	if (q_file)
		to_return["files"] = display_files(query, sorted_files);
	to_return["timer"] = json::value::string(timer.print_elapsed(), true);
	if(qname != NULL) 
		delete qname;
	return to_return;
}

// Return an identifier property
static string
show_id_prop(const string &name, bool val)
{
	
	if (!Option::show_true->get() || val)
		return "<li>" + name + ": " + (val ? "Yes" : "No") + "\n" ;
	else 
		return "";
}

// Details for each identifier and return as JSON
// {
//		form: "html start",
//		attribute: [
//			{
//				name: "attribute name",
//				get: value yes or no,
//				html: "html code"
//			}
//		],
//		file_boundary: {
//			name: "attribute name",
//			get: value yes or no,
//			html: "html code"
//		},
//		unused: {
//			name: "attribute name",
//			get: value yes or no,
//			html: "html code"
//		},
//		match: "html number of matches",
//		occurances: number of occurancies,
//		(projects: {
//			start: "html start of list",
//			contents: [
//				"project names"
//			],
//		end: "html end of list"
//		}),
//		endAttr: [
//				"html dependant link",
//				"html associated link"
//		],
//		ec: "memory address of id",
//		identifier: "id",
//		(functions: {
//			start: "html start of list",
//			contents: [
//				{
//					f:"function address",
//					html: "html of function page link"
//				}
//			],
//			end: "html of end"
//		}).
//		(substitute: {
//			start: "html start of input form",
//			content: [
//				"html of input for sname",
//				"html of input for ",
//				"html of input for "
//			],
//			end: "html of end form"
//			(,inactive: "html link to replacements page")
//		}),
//		end: html end of page
// }
json::value
identifier_page(void *p)
{
	json::value to_return;
	Eclass *e;
	int no = 0;
	
	e = (Eclass*)server.getAddrParam("id");
	if (!e) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}

	const char *subst;
	Identifier &id = ids[e];
	if ((subst = server.getCharPParam("sname")) != NULL) {
		if (modification_state == ms_hand_edit) {
			to_return["error"] = change_prohibited();
			delete subst;
			return to_return;
		}
		ostringstream fs;
		prohibit_browsers(&fs);
		prohibit_remote_access(&fs);
		if (fs.str().length() > 0) {
			to_return["error"] = json::value::string(fs.str());
			delete subst;
			return to_return;
		}

		// Passing subst directly core-dumps under
		// gcc version 2.95.4 20020320 [FreeBSD 4.7]
		string ssubst(subst);
		id.set_newid(ssubst);
		modification_state = ms_subst;
		delete subst;
	}

	
	to_return["form"] = json::value::string("<FORM ACTION=\"id.html\" METHOD=\"GET\">\n<ul>\n");
	string s;
	for (int i = attr_begin; i < attr_end; i++) {		
		s = show_id_prop(Attributes::name(i), e->get_attribute(i));		

		if(!s.empty()) {
			to_return["attribute"][no]["name"] = json::value(Attributes::name(i));
			to_return["attribute"][no]["get"] = json::value(e->get_attribute(i));
			to_return["attribute"][no++]["html"] = json::value::string(s);
		}
	}
	
	s = show_id_prop("Crosses file boundary", id.get_xfile());
	if(!s.empty()) {
		to_return["file_boundary"]["name"] = json::value("Crosses file boundary");
		to_return["file_boundary"]["get"] = json::value(id.get_xfile());
		to_return["file_boundary"]["html"] = json::value::string(s);
	}
	s = show_id_prop("Unused", e->is_unused());

	if(!s.empty()) {
		to_return["unused"]["name"] = json::value("Unused");
		to_return["unused"]["get"] = json::value(e->is_unused());
		to_return["unused"]["html"] = json::value::string(s);
	}	
	to_return["match"] = json::value::string("<li> Matches " + to_string(e->get_size()) + " occurence(s)\n");
	to_return["occurences"] = json::value(e->get_size());
	no = 0;
	if (Option::show_projects->get()) {
		to_return["projects"]["start"] = json::value::string("Appears in project(s): \n");
		if (DP()) {
			cout << "First project " << attr_end << endl;
			cout << "Last project " <<  Attributes::get_num_attributes() - 1 << endl;
		}
		for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++)
			if (e->get_attribute(j))
				to_return["projects"]["content"][no++] = json::value::string(Project::get_projname(j));
		to_return["projects"]["end"] = json::value::string("</ul>\n");
	}
	ostringstream fs;
	fs << e;
	to_return["endAttr"][0] = json::value::string("<li><a href=\"xiquery.html?ec="
		+ fs.str() + "&n=Dependent+Files+for+Identifier+" +
		id.get_id() + "&qf=1\">Dependent files</a>");
	to_return["endAttr"][1] = json::value::string("<li><a href=\"xfunquery.html?ec=" +
	fs.str() + "&qi=1&n=Functions+Containing+Identifier+"
	+ id.get_id() + "\">Associated functions</a>");
	to_return["ec"] = json::value(fs.str());
	to_return["identifier"] = json::value(id.get_id());
	no = 0;
	if (e->get_attribute(is_cfunction) || e->get_attribute(is_macro)) {
		bool found = false;
		// Loop through all declared functions
		for (Call::const_fmap_iterator_type i = Call::fbegin(); i != Call::fend(); i++) {
			if (i->second->contains(e)) {
				fs.flush();
				fs << i->second;
				if (!found) {
					to_return["functions"]["start"] = json::value::string(
						"<li> The identifier occurs (wholy or in part) in function name(s): \n<ol>\n");
					found = true;
				}
				to_return["functions"]["content"][no]["f"] = json::value(fs.str());

				to_return["functions"]["content"][no++]["html"] = json::value::string("\n<li>" + html_string(i->second)
				+ " &mdash; <a href=\"fun.html?f=" + fs.str() + "\">function page</a>");

			}
		}
		if (found)
			to_return["functions"]["end"] = json::value::string("</ol><br />\n");
	}

	if ((!e->get_attribute(is_readonly) || Option::rename_override_ro->get()) &&
	    modification_state != ms_hand_edit &&
	    !browse_only) 
	{
		to_return["substitute"]["start"] = json::value::string("<li> Substitute with: \n");
		to_return["substitute"]["content"][0] = json::value::string("<INPUT TYPE=\"text\" NAME=\"sname\" VALUE=\""
			+ (id.get_replaced() ? id.get_newid() : id.get_id()) + "\" SIZE=10 MAXLENGTH=256> ");
		to_return["substitute"]["content"][1] = json::value::string("<INPUT TYPE=\"submit\" NAME=\"repl\" VALUE=\"Save\">\n");

		to_return["substitute"]["content"][2] = json::value::string("<INPUT TYPE=\"hidden\" NAME=\"id\" VALUE=\""
		+ fs.str() + "\">\n");
		if (!id.get_active())
			to_return["inactive"] = json::value::string("<a href='replacements.html'>replacements page</a> ");
	}
	to_return["end"] = json::value::string("</ul>\n</FORM>\n");
	return to_return;

}

// Details for each function 
// {
//		fname: "function name",
//		fun_name: "html function name".
//		data: [
//			identifiers: [
//				{
//					id: "id-address",
//					name: "identifier name"
//				}
//		],	
//		form: "html of start form",
//		(
//			declared: {
//				tokid: declaration token id,
//				tokpath: "declaration token path name",
//				html: "html of file declarations",
//				lnum: line number of declaration
//			},
//		)
//		defined: {
//			html: "html of defined"
//			(,
//				tokid: definition token id,
//				tokpath: "definition token path name",
//				lnum: line number of definition
//			)
//		}
//		list: [
//			8 html links to graphs or pages about the function
//		],
//		f: "function call object address",
//		no_call: number of functions this function calls,
//		no_called: number of functions that call this function 	
//		(
//			refactor: {
//				start: "html of start refactor form",
//				content: [
//					"html of input ncall",
//					"html of input replace"
//				],
//				hidden: [
//					"html f value",
//					"html ec value"
//				],
//				inactive: "html of refactorings page link",
//			},
//		)
//		end_list: "html end list of links",
//		(,
//			metrics: {
//				start: "html of metrics table head",
//				content: [
//					"html of table rows"
//				],
//				end: "html of end table".
//				data:{
//					metrics_name0: metrics_value0,
//					metrics_name1: metrics_value1,
//					....
//				},
//		)
//		end: "html of end"
//	}
json::value
function_page(void *p)
{
	json::value to_return;
	Call *f = (Call *)server.getAddrParam("f");
	if (f == NULL) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}
	const char *subst;
	string sust;
	if (!server.getStrParam("ncall").empty()) {
		
		sust = server.getStrParam("ncall");
		subst = sust.c_str();
		string ssubst(subst);
		const char *error;
		if (!is_function_call_replacement_valid(ssubst.begin(), ssubst.end(), &error)) {
			to_return["error"] = json::value(
				"Invalid function call refactoring template: " + string(error));
			return to_return;
		}
		Eclass *ec = (Eclass *)server.getAddrParam("id");
		if (ec == NULL) {
			to_return["error"] = json::value::string("Missing value");
			return to_return;
		}
		if (modification_state == ms_hand_edit) {
			to_return["error"] = change_prohibited();
			return to_return;
		}
		std::ostringstream fs;
		prohibit_browsers(&fs);
		prohibit_remote_access(&fs);
		if (fs.str().length() > 0) {
			to_return["error"] = json::value::string(fs.str());
			return to_return;
		}
		RefFunCall::store.insert(RefFunCall::store_type::value_type(ec, RefFunCall(f, subst)));
		modification_state = ms_subst;
	}
	std::ostringstream fs;
	to_return["fname"] = json::value(f->get_name());
	to_return["etype"] = json::value(f->entity_type_name());
	to_return["fun_name"] = json::value::string(html(f->get_name()) + " (" + f->entity_type_name() + ')');
	json::value temp = html_json(f);
	to_return["data"] = temp["data"];
	to_return["form"] = json::value::string("<FORM ACTION=\"fun.html\" METHOD=\"GET\">\n"
	"<ul>\n <li> Associated identifier(s): " + temp["string"].as_string());

	Tokid t = f->get_tokid();
	
	if (f->is_declared()) {
		fs << "\n<li> Declared in file <a href=\"file.html?id=" << t.get_fileid().get_id()
		<< "\">" << t.get_fileid().get_path() << "</a>";
		int lnum = t.get_fileid().line_number(t.get_streampos());
		fs << " <a href=\"src.html?id=" << t.get_fileid().get_id() << "#" << lnum
		 << "\">line " << lnum << "</a><br />(and possibly in other places)\n"
		 << " &mdash; <a href=\"qsrc.html?qt=fun&id=" << t.get_fileid().get_id()
		 << "&match=Y&call=" << f << "&n=Declaration+of+" << f->get_name() 
		 << "\">marked source</a>";
			if (modification_state != ms_subst && !browse_only)
				fs << " &mdash; <a href=\"fedit.html?id=" << t.get_fileid().get_id() <<
				"&re=" << f->get_name() << "\">edit</a>";
		to_return["declared"]["tokid"] = json::value(t.get_fileid().get_id());
		to_return["declared"]["tokpath"] = json::value(t.get_fileid().get_path());
		to_return["declared"]["html"] = json::value::string(fs.str());
		to_return["declared"]["lnum"] = json::value(lnum);

	}
	fs.flush();
	if (f->is_defined()) {
		t = f->get_definition();
		fs << "<li> Defined in file <a href=\"file.html?id="
		<< t.get_fileid().get_id() << "\">" << t.get_fileid().get_path() << "</a>";
		int lnum = t.get_fileid().line_number(t.get_streampos());
		fs << " <a href=\"src.html?id=" <<	t.get_fileid().get_id()
		<< "#" << lnum << "\">line " << lnum << "</a>\n";
		if (modification_state != ms_subst && !browse_only)
			fs << " &mdash; <a href=\"fedit.html?id=" << t.get_fileid().get_id()
			<< "&re=" << f->get_name() << "\">edit</a>";
		to_return["defined"]["tokid"] = json::value(t.get_fileid().get_id());
		to_return["defined"]["tokpath"] = json::value(t.get_fileid().get_path());
		to_return["defined"]["lnum"] = json::value(lnum);
	} else
		fs << "<li> No definition found\n";
	to_return["defined"]["html"] = json::value::string(fs.str());
	fs.flush();
	fs << f;
	// Functions that are Down from us in the call graph
	to_return["list"][0] = json::value::string("<li> Calls directly " +
	to_string(f->get_num_call()) + " functions" );
	to_return["list"][1] = json::value::string("<li> <a href=\"funlist.html?f=" + fs.str() + "&n=d&e=1\">Explore directly called functions</a>\n");
	to_return["list"][2] = json::value::string("<li> <a href=\"funlist.html?f=" +
	fs.str() + "&n=D\">List of all called functions</a>\n");
	to_return["list"][3] = json::value::string("<li> <a href=\"cgraph" +
	graph_suffix() + "?all=1&f=" + fs.str() + "&n=D\">Call graph of all called functions</a>");
	// Functions that are Up from us in the call graph
	to_return["list"][4] = json::value::string("<li> Called directly by " + to_string(f->get_num_caller()) + " functions");
	to_return["list"][5] = json::value::string("<li> <a href=\"funlist.html?f=" +
	fs.str() + "&n=u&e=1\">Explore direct callers</a>\n");
	to_return["list"][6] = json::value::string("<li> <a href=\"funlist.html?f=" +
	fs.str() + "&n=U\">List of all callers</a>\n");
	to_return["list"][7] = json::value::string("<li> <a href=\"cgraph"
	+ graph_suffix() + "?all=1&f=" + fs.str() + "&n=U\">Call graph of all callers</a>");
	to_return["list"][8] = json::value::string("<li> <a href=\"cgraph"
	+ graph_suffix() + "?all=1&f=" + fs.str() + "&n=B\">Call graph of all calling and called functions</a> (function in context)");
	to_return["f"] = json::value::string(fs.str());
	to_return["graph_suffix"] = json::value::string(graph_suffix());
	to_return["no_call"] = json::value(f->get_num_call());
	to_return["no_called"] = json::value(f->get_num_caller());
	// Allow function call refactoring only if there is a one to one relationship between the identifier and the function
	Eclass *ec;
	if (f->get_token().get_parts_size() == 1 &&
	    modification_state != ms_hand_edit &&
	    !browse_only &&
	    (ec = f->get_token().get_parts_begin()->get_tokid().check_ec()) &&
	    (!ec->get_attribute(is_readonly) || Option::refactor_fun_arg_override_ro->get())
	    ) {
		// Count associated declared functions
		int nfun = 0;
		for (Call::const_fmap_iterator_type i = Call::fbegin(); i != Call::fend(); i++)
			if (i->second->contains(ec))
				nfun++;
		if (nfun == 1) {
			ostringstream repl_temp;		// Replacement template
			RefFunCall::store_type::const_iterator rfc;
		    	if ((rfc = RefFunCall::store.find(ec)) != RefFunCall::store.end())
				repl_temp << html(rfc->second.get_replacement());
			else if (f->is_defined())
				for (int i = 0; i < f->metrics().get_metric(FunMetrics::em_nparam); i++) {
					repl_temp << '@' << i + 1;
					if (i + 1 < f->metrics().get_metric(FunMetrics::em_nparam))
						repl_temp << ", ";
				}
			to_return["refractor"]["start"] = json::value::string("<li> Refactor arguments into: \n");
			to_return["refractor"]["content"][0] = json::value::string("<INPUT TYPE=\"text\" NAME=\"ncall\" VALUE=\""
			+ repl_temp.str() + "\" SIZE=40 MAXLENGTH=256> ");
			
			to_return["refractor"]["content"][1] = json::value::string("<INPUT TYPE=\"submit\" NAME=\"repl\" VALUE=\"Save\">\n");
			to_return["refractor"]["hidden"][0] = json::value::string("<INPUT TYPE=\"hidden\" NAME=\"f\" VALUE=\"" + fs.str() + "\">\n");
			fs.flush();
			fs << ec;
			to_return["refractor"]["hidden"][1] = json::value::string("<INPUT TYPE=\"hidden\" NAME=\"id\" VALUE=\""
			+ fs.str() + "\">\n");
			to_return["ec"] = json::value::string(fs.str());
			if (rfc != RefFunCall::store.end() && !rfc->second.is_active())
				to_return["refractor"]["inactive"] = json::value::string("<br>(This refactoring is inactive."
				"  Visit the <a href='funargrefs.html'>refactorings page</a> to activate it again.)");
		}
	}
	to_return["end_list"] = json::value::string("</ul>\n");
	int no = 0;
	if (f->is_defined()) {
		to_return["metrics"]["start"] = json::value::string("<h2>Metrics</h2>\n<table class='metrics'>\n<tr><th>Metric</th>"
		"<th>Value</th></tr>\n");
		for (int j = 0; j < FunMetrics::metric_max; j++)
			if (!Metrics::is_internal<FunMetrics>(j)) {
				to_return["metrics"]["data"][Metrics::get_name<FunMetrics>(j)] = json::value(f->metrics().get_metric(j));
				to_return["metrics"]["content"][no++] = json::value::string("<tr><td>" +
				Metrics::get_name<FunMetrics>(j) + "</td><td align='right'>" +
				to_string(f->metrics().get_metric(j)) + "</td></tr>");
			}
		to_return["metrics"]["end"] = json::value::string("</table>\n");
	}
	to_return["end"] = json::value::string("</FORM>\n");
	return to_return;
}

/*
 * Visit all functions associated with a call/caller relationship with f
 * Call_path is an HTML string to print next to each function that
 * leads to a page showing the function's call path.  A single %p
 * in the string is used as a placeholder to fill the function's address.
 * The methods to obtain the relationship containers are passed through
 * the fbegin and fend method pointers.
 * If recurse is true the list will also contain all correspondingly
 * associated children functions.
 * If show is true, then a function hyperlink is printed, otherwise
 * only the visited flag is set to visit_id.
 * [
 * 		(
 * 		(cgraph:"call path",)
 * 		fid: "html file id link",
 * 		f: "function call object address",
 * 		fname: "funciton name"),
 * 		call: [
 * 			recursive json visit_functions
 * 		]
 */
static json::value
visit_functions(const char *call_path, Call *f,
	Call::const_fiterator_type (Call::*fbegin)() const,
	Call::const_fiterator_type (Call::*fend)() const,
	bool recurse, bool show, int level, int visit_id = 1)
{
	json::value to_return;
	if (level == 0)
		return json::value::array();

	Call::const_fiterator_type i;
	int no = 0;
	char * s = new char[256];
	
	f->set_visited(visit_id);
	for (i = (f->*fbegin)(); i != (f->*fend)(); i++) {
		if (show && (!(*i)->is_visited(visit_id) || *i == f)) {			
			if (recurse && call_path) {
				sprintf(s, call_path, *i);
				to_return[no]["cgraph"] = json::value(s);
			}
			to_return[no]["fid"] = json::value::string(html(**i));			
			char * p = new char[20];
			sprintf(p, "%p", *i);
			to_return[no]["fname"] = json::value((*i)->get_name());
			to_return[no++]["f"] = json::value(s);
			delete p;
			
		}
		if (recurse && !(*i)->is_visited(visit_id)) {
			if(show || *i == f)
				to_return[no-1]["call"] = visit_functions(call_path, *i, fbegin, fend, recurse, show, level - 1, visit_id);
			else 
				to_return[no++]["call"] = visit_functions(call_path, *i, fbegin, fend, recurse, show, level - 1, visit_id);
		}
	}
	delete s;
	return to_return;
}

/*
 * Visit all files associated with a includes/included relationship with f
 * The method to obtain the relationship container is passed through
 * the get_map method pointer.
 * The method to check if a file should be included in the visit is passed through the
 * the is_ok method pointer.
 * Set the visited flag for all nodes visited.
 * 
 * 
 */
static void
visit_include_files(Fileid f, const FileIncMap & (Fileid::*get_map)() const,
    bool (IncDetails::*is_ok)() const, int level)
{
	if (level == 0)
		return;

	if (DP())
		cout << "Visiting " << f.get_fname() << endl;
	f.set_visited();
	const FileIncMap &m = (f.*get_map)();
	for (FileIncMap::const_iterator i = m.begin(); i != m.end(); i++) {
		if (!i->first.is_visited() && (i->second.*is_ok)())
			visit_include_files(i->first, get_map, is_ok, level - 1);
	}
}

/*
 * Visit all files associated with a global variable def/ref relationship with f
 * The method to obtain the relationship container is passed through
 * the get_set method pointer.
 * Set the visited flag for all nodes visited.
 */
static void
visit_globobj_files(Fileid f, const Fileidset & (Fileid::*get_set)() const, int level)
{
	if (level == 0)
		return;

	if (DP())
		cout << "Visiting " << f.get_fname() << endl;
	f.set_visited();
	const Fileidset &s = (f.*get_set)();
	for (Fileidset::const_iterator i = s.begin(); i != s.end(); i++) {
		if (!i->is_visited())
			visit_globobj_files(*i, get_set, level - 1);
	}
}

/*
 * Visit all files associated with a function call relationship with f
 * (a control dependency).
 * The methods to obtain the relationship iterators are passed through
 * the abegin and aend method pointers.
 * Set the visited flag for all nodes visited and the edges matrix for
 * the corresponding edges.
 */
static void
visit_fcall_files(Fileid f, Call::const_fiterator_type (Call::*abegin)() const, Call::const_fiterator_type (Call::*aend)() const, int level, EdgeMatrix &edges)
{
	if (level == 0)
		return;

	if (DP())
		cout << "Visiting " << f.get_fname() << endl;
	f.set_visited();
	/*
	 * For every function in this file:
	 * for every function associated with this function
	 * set the edge and visit the corresponding files.
	 */
	for (FCallSet::const_iterator filefun = f.get_functions().begin(); filefun != f.get_functions().end(); filefun++) {
		if (!(*filefun)->is_cfun())
			continue;
		for (Call::const_fiterator_type afun = ((*filefun)->*abegin)(); afun != ((*filefun)->*aend)(); afun++)
			if ((*afun)->is_defined() && (*afun)->is_cfun()) {
				Fileid f2((*afun)->get_definition().get_fileid());
				edges[f.get_id()][f2.get_id()] = true;
				if (!f2.is_visited())
					visit_fcall_files(f2, abegin, aend, level - 1, edges);
			}
	}
}


 extern "C" { const char *swill_getquerystring(void); }

/*
 * Return a list of callers or called functions for the given function,
 * recursively expanding functions that the user has specified.
 * 	[
 * 		fname: "name of function",
 * 		f: "function call object address",
 * 		html: "html code of function link"
 * 		(,call: [
 * 			recursive json of explore_functions			
 * 		])
 * 	]
 */
static json::value
explore_functions(Call *f,
	Call::const_fiterator_type (Call::*fbegin)() const,
	Call::const_fiterator_type (Call::*fend)() const,
	bool recursive)
{
	Call::const_fiterator_type i;
	json::value to_return = NULL;
	int no = 0;
	char * s = new char[20];
	for (i = (f->*fbegin)(); i != (f->*fend)(); i++) {				
		to_return[no]["fname"] = json::value((**i).get_name());
		sprintf(s, "%p", *i);
		to_return[no]["f"] = json::value(s);
		s[0] = 0;
		to_return[no++]["html"] = json::value::string(html(**i));
		if(recursive) {
			to_return[no-1]["call"] = explore_functions(f, fbegin, fend, true);
		}
	} 
	delete s;
	return to_return;		
}


// List of functions associated with a given one
// {
//		start: "html of start",
//		fname: "function name",
//		f: "function adress",
//		ltype: "",
// 		calltype: "calltype directly or all",
//		graph_suffix: "suffix for graph()",
//		title: "html title and link to graph",
//		exfuns: { explore_function return JSON} or
//		vfuns: { visit_function return JSON}	
// }
static json::value
funlist_page(void *p)
{
	json::value to_return;
	Call *f = (Call *)server.getAddrParam("f");
	char buff[256];
	string s = server.getStrParam("n");
	const char *ltype = s.c_str();
	
	if (f == NULL || !ltype) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}
	char * temp = new char[20];
	sprintf(temp,"%p",f);
	to_return["fname"] = json::value(f->get_name());
	to_return["f"] = json::value(s);
	to_return["start"] = json::value::string("<h2>Function " + html(*f) + "</h2>");
	delete temp;
	const char *calltype;
	bool recurse;
	switch (*ltype) {
	case 'u': case 'd':
		calltype = "directly";
		recurse = false;
		break;
	case 'U': case 'D':
		calltype = "all";
		recurse = true;
		break;
	default:
		to_return["error"] = json::value::string("Illegal value");
		return to_return;
	}
	// Pointers to the ...begin and ...end methods
	Call::const_fiterator_type (Call::*fbegin)() const;
	Call::const_fiterator_type (Call::*fend)() const;
	to_return["ltype"] = json::value(ltype);
	to_return["calltype"] = json::value(calltype);
	to_return["graph_suffix"] = json::value(graph_suffix().c_str());
	switch (*ltype) {
	default:
	case 'u':
	case 'U':
		fbegin = &Call::caller_begin;
		fend = &Call::caller_end;
		to_return["title"] = json::value::string("List of " + string(calltype) + " calling functions\n");
		sprintf(buff, " &mdash; <a href=\"cpath%s?from=%%p&to=%p\">call path from function</a>", graph_suffix().c_str(), f);
		break;
	case 'd':
	case 'D':
		fbegin = &Call::call_begin;
		fend = &Call::call_end;
		to_return["title"] = json::value::string("List of " + string(calltype) + " called functions\n");
		sprintf(buff, " &mdash; <a href=\"cpath%s?from=%p&to=%%p\">call path to function</a>", graph_suffix().c_str(), f);
		break;
	}
	if (server.getBoolParam("e")) {
		to_return["exfuns"] = explore_functions(f, fbegin, fend, false);
	} else {
		Call::clear_visit_flags();
		to_return["vfuns"] = visit_functions(buff, f, fbegin, fend, recurse, true, Option::cgraph_depth->get());
	}
	return to_return;
}

// Display the call paths between functions from and to
// This should be called once to generate the nodes and a second time
// to generate the edges
static bool
call_path(GraphDisplay *gd, Call *from, Call *to, bool generate_nodes)
{
	bool ret = false;

	from->set_visited();
	if (DP())
		cout << "From path: from: " << from->get_name() << " to " << to->get_name() << endl;
	int count = 0;
	for (Call::const_fiterator_type i = from->call_begin(); i != from->call_end(); i++)
		if (!(*i)->is_visited() && (*i == to || call_path(gd, *i, to, generate_nodes))) {
			if (generate_nodes) {
				if (!from->is_printed()) {
					gd->node(from);
					from->set_printed();
					if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
						break;
				}
				if (!(*i)->is_printed()) {
					gd->node(*i);
					(*i)->set_printed();
					if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
						break;
				}
			} else {
				gd->edge(from, *i);
				if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
					break;
			}
			ret = true;
		}
	if (DP())
		cout << "Returns " << ret << endl;
	return (ret);
}

// List the call graph from one function to another
static void
cpath_page(GraphDisplay *gd)
{
	Call *from, *to;
	from = (Call *)server.getAddrParam("from");
	if (from == NULL) {
		fprintf(stderr, "Missing from value");
		return;
	}
	to = (Call *)server.getAddrParam("to");
	if (to == NULL) {
		fprintf(stderr, "Missing to value");
		return;
	}
	gd->head("callpath", "Function Call Path", Option::cgraph_show->get() == 'e');
	gd->subhead(string("Path ") +
	    function_label(from, true) +
	    string(" &rarr; ") +
	    function_label(to, true));
	Call::clear_print_flags();
	Call::clear_visit_flags();
	call_path(gd, from, to, true);
	Call::clear_visit_flags();
	call_path(gd, from, to, false);
	gd->tail();
}


// Front-end global options page
// {
//		form: "html start of form",
//		main: { display_all return JSON},
//		end: "html end of form"
// }
json::value
options_page(void *p)
{
	json::value to_return;
	to_return["form"] = json::value("<FORM ACTION=\"soptions.html\" METHOD=\"PUT\">\n");
	to_return["main"] = Option::display_all();
	to_return["end"] = json::value("<p><p><INPUT TYPE=\"submit\" NAME=\"set\" VALUE=\"OK\">\n"
		"<INPUT TYPE=\"submit\" NAME=\"set\" VALUE=\"Cancel\">\n"
		"<INPUT TYPE=\"submit\" NAME=\"set\" VALUE=\"Apply\">\n"
		"</FORM>\n");
	return to_return;
}

// Front-end global options page
// {
//		action: "action to take"
// }
json::value
set_options_page(void *p)
{
	json::value to_return;
	ostringstream fs;
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
			to_return["error"] = json::value::string(fs.str());
			return to_return;
		}

	if (server.getStrParam("set") == "Cancel") {
		to_return["action"] = json::value("index");
		return to_return;
	}
	Option::set_all();
	if (Option::sfile_re_string->get().length()) {
		sfile_re = CompiledRE(Option::sfile_re_string->get().c_str(), REG_EXTENDED);
		if (!sfile_re.isCorrect()) {
			
			to_return["error"] = json::value::string("Filename regular expression error" + sfile_re.getError());
			return to_return;
		}
	}
	if (server.getStrParam("set") == "Apply")
		to_return = options_page(p);
	else
		to_return["action"] = json::value("index");
	
	return to_return;;
}

// Save options in .cscout/options
// { 
// 		fname: "filename" if save was a success	
// }
static json::value
save_options_page(void *p)
{
	// define JSON func
	json::value to_return;
	ostringstream fs;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if(!fs.str().empty()) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	} 

	ofstream out;
	string fname;
	if (!cscout_output_file("options", out, fname)) {
		to_return["error"] = json::value::string("Unable to open " + fname + " for writing");
		return to_return;
	}
	Option::save_all(out);
	out.close();
 	to_return["file_name"] = json::value::string(fname);
	 
	return to_return;
}

// Load the CScout options.
static void
options_load()
{
	ifstream in;
	string fname;

	if (!cscout_input_file("options", in, fname)) {
		fprintf(stderr, "No options file found; will use default options.\n");
		return;
	}
	Option::load_all(in);
	if (Option::sfile_re_string->get().length()) {
		sfile_re = CompiledRE(Option::sfile_re_string->get().c_str(), REG_EXTENDED);
		if (!sfile_re.isCorrect()) {
			fprintf(stderr, "Filename regular expression error: %s", sfile_re.getError().c_str());
			Option::sfile_re_string->erase();
		}
	}
	in.close();
	fprintf(stderr, "Options loaded from %s\n", fname.c_str());
}

// Return file metrics JSON
json::value
file_metrics_page(void *p)
{
	return file_msum.json();
}

// Return function metrics JSON
json::value
function_metrics_page(void *p)
{
	return fun_msum.json();
}

// Return id metrics JSON
json::value
id_metrics_page(void *p)
{
	return id_msum.json();
}

/*
 * Return true if the call graph is specified for a single function.
 * In this case only show entries that have the visited flag set
 */
static bool
single_function_graph()
{
	Call *f = (Call *)server.getAddrParam("f");
	string s = server.getStrParam("n");
	const char *ltype = s.c_str();
	if ((f == NULL) || !ltype)
		return false;
	Call::clear_visit_flags();
	// No output, just set the visited flag
	switch (*ltype) {
	case 'D':
		visit_functions( NULL, f, &Call::call_begin, &Call::call_end, true, false, Option::cgraph_depth->get());
		break;
	case 'U':
		visit_functions( NULL, f, &Call::caller_begin, &Call::caller_end, true, false, Option::cgraph_depth->get());

		break;
	case 'B':
		visit_functions( NULL, f, &Call::call_begin, &Call::call_end, true, false, Option::cgraph_depth->get(), 1);
		visit_functions( NULL, f, &Call::caller_begin, &Call::caller_end, true, false, Option::cgraph_depth->get(), 2);

		break;
	}
	return true;
}

/*
 * Return true if the include/global/call graph is specified for a single file.
 * In this case caller will only show entries that have the visited flag set, so
 * set this flag as specified.
 * For function * call graphs also fill edges with a matrix indicating the dges to draw
 */
static bool
single_file_graph(char gtype, EdgeMatrix &edges)
{
	int id;
	string s = server.getStrParam("n");
	const char *ltype = s.c_str();
	if (!(id = server.getIntParam("f")) || !ltype)
		return false;
	Fileid fileid(id);
	Fileid::clear_all_visited();
	// No output, just set the visited flag
	switch (gtype) {
	case 'I':		// Include graph
		switch (*ltype) {
		case 'D':
			visit_include_files(fileid, &Fileid::get_includers, &IncDetails::is_directly_included, Option::cgraph_depth->get());
			break;
		case 'U':
			visit_include_files(fileid, &Fileid::get_includes, &IncDetails::is_directly_included, Option::cgraph_depth->get());
			break;
		}
		break;
	case 'C':		// Compile-time dependency graph
		switch (*ltype) {
		case 'D':
			visit_include_files(fileid, &Fileid::get_includers, &IncDetails::is_required, Option::cgraph_depth->get());
			break;
		case 'U':
			visit_include_files(fileid, &Fileid::get_includes, &IncDetails::is_required, Option::cgraph_depth->get());
			break;
		}
		break;
	case 'G':		// Global object def/ref graph (data dependency)
		switch (*ltype) {
		case 'D':
			visit_globobj_files(fileid, &Fileid::glob_uses, Option::cgraph_depth->get());
			break;
		case 'U':
			visit_globobj_files(fileid, &Fileid::glob_used_by, Option::cgraph_depth->get());
			break;
		}
		break;
	case 'F':		// Function call graph (control dependency)
		int size = Fileid::max_id() + 1;
		edges.insert(edges.begin(), size, vector<bool>(size, 0));
		switch (*ltype) {
		case 'D':
			visit_fcall_files(fileid, &Call::call_begin, &Call::call_end, Option::cgraph_depth->get(), edges);
			break;
		case 'U':
			visit_fcall_files(fileid, &Call::caller_begin, &Call::caller_end, Option::cgraph_depth->get(), edges);
			break;
		}
		break;
	}
	return true;
}

/*
 * Return true if the call graph is specified for functions in a single file.
 * In this case only show entries that have the visited flag set
 */
static bool
single_file_function_graph()
{
	int id;
	if (!(id = server.getIntParam("id")))
		return false;
	Fileid fileid(id);

	Call::clear_visit_flags();
	Call::const_fmap_iterator_type fun;
	for (fun = Call::fbegin(); fun != Call::fend(); fun++)
		if (fun->second->get_begin().get_tokid().get_fileid() == fileid)
			fun->second->set_visited();
	return true;
}

// Call graph
static void
cgraph_page(GraphDisplay *gd)
{
	bool all, only_visited;
	if (gd->uses_swill) {
		all = !!server.getBoolParam("all");
		only_visited = (single_function_graph() || single_file_function_graph());
	}
	else {
		all = gd->all;
		only_visited = gd->only_visited;
	}
	gd->head("cgraph", "Call Graph", Option::cgraph_show->get() == 'e');
	int count = 0;
	// First generate the node labels
	Call::const_fmap_iterator_type fun;
	Call::const_fiterator_type call;
	for (fun = Call::fbegin(); fun != Call::fend(); fun++) {
		if (!all && fun->second->is_file_scoped())
			continue;
		if (only_visited && !fun->second->is_visited())
			continue;
		gd->node(fun->second);
		if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
			goto end;
	}
	// Now the edges
	for (fun = Call::fbegin(); fun != Call::fend(); fun++) {
		if (!all && fun->second->is_file_scoped())
			continue;
		if (only_visited && !fun->second->is_visited())
			continue;
		for (call = fun->second->call_begin(); call != fun->second->call_end(); call++) {
			if (!all && (*call)->is_file_scoped())
				continue;
			// No edge unless both functions were visited on the same tour
			// as indicated by the corresponding bitmasks.
			if (only_visited && !((*call)->get_visited() & fun->second->get_visited()))
				continue;
			gd->edge(fun->second, *call);
			if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
				goto end;
		}
	}
end:
	gd->tail();
}

// File dependency graph
static void
fgraph_page(GraphDisplay *gd)
{

	const char *gtype = NULL;
	const char *ltype = NULL;
	if (gd->uses_swill) {
		gtype = server.getCharPParam("gtype");		// Graph type
		ltype = server.getCharPParam("n");
	}
	else {
		gtype = gd->gtype;
		ltype = gd->ltype;
	}
	if (!gtype || !*gtype || (*gtype == 'F' && !ltype)) {
		gd->head("fgraph", "Error", false);
		gd->error("Missing value");
		gd->tail();
		return;
	}
	bool all, only_visited;
	EdgeMatrix edges;
	bool empty_node = (Option::fgraph_show->get() == 'e');
	if (gd->uses_swill) {
		all = !!server.getBoolParam("all");		// Otherwise exclude read-only files
		only_visited = single_file_graph(*gtype, edges);
	}
	else {
		all = gd->all;
		only_visited = gd->only_visited;
	}

	switch (*gtype) {
	case 'I':		// Include graph
		gd->head("fgraph", "Include Graph", empty_node);
		break;
	case 'C':		// Compile-time dependency graph
		gd->head("fgraph", "Compile-Time Dependency Graph", empty_node);
		break;
	case 'G':		// Global object def/ref graph (data dependency)
		gd->head("fgraph", "Global Object (Data) Dependency Graph", empty_node);
		break;
	case 'F':		// Function call graph (control dependency)
		gd->head("fgraph", "Function Call (Control) Dependency Graph", empty_node);
		if (!only_visited) {
			int size = Fileid::max_id() + 1;
			edges.insert(edges.begin(), size, vector<bool>(size, 0));
			// Fill the edges for all files
			Fileid::clear_all_visited();
			for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
				if (i->is_visited())
					continue;
				if (!all && i->get_readonly())
					continue;
				switch (*ltype) {
				case 'D':
					visit_fcall_files(*i, &Call::call_begin, &Call::call_end, Option::cgraph_depth->get(), edges);
					break;
				case 'U':
					visit_fcall_files(*i, &Call::caller_begin, &Call::caller_end, Option::cgraph_depth->get(), edges);
					break;
				}
			}
		}
		break;
	default:
		gd->head("fgraph", "Error", empty_node);
		gd->error("Unknown graph type");
		gd->tail();
		return;
	}
	int count = 0;
	// First generate the node labels
	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		if (!all && i->get_readonly())
			continue;
		if (only_visited && !i->is_visited())
			continue;
		gd->node(*i);
		if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
			goto end;
	}
	// Now the edges
	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		if (!all && i->get_readonly())
			continue;
		if (only_visited && !i->is_visited())
			continue;
		switch (*gtype) {
		case 'C':		// Compile-time dependency graph
		case 'I': {		// Include graph
			const FileIncMap &m(i->get_includes());
			for (FileIncMap::const_iterator j = m.begin(); j != m.end(); j++) {
				if (*gtype == 'I' && !j->second.is_directly_included())
					continue;
				if (*gtype == 'C' && !j->second.is_required())
					continue;
				if (!all && j->first.get_readonly())
					continue;
				if (only_visited && !j->first.is_visited())
					continue;
				gd->edge(j->first, *i);
				if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
					goto end;
			}
			break;
		}
		case 'F':		// Function call graph (control dependency)
			for (vector <Fileid>::iterator j = files.begin(); j != files.end(); j++) {
				if (!all && j->get_readonly())
					continue;
				if (only_visited && !j->is_visited())
					continue;
				if (*i == *j)
					continue;
				if (DP())
					cout << "Checking " << i->get_fname() << " - " << j->get_fname() << endl;
				if (edges[i->get_id()][j->get_id()])
					switch (*ltype) {
					case 'D':
						gd->edge(*j, *i);
						break;
					case 'U':
						gd->edge(*i, *j);
						break;
					}
				if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
					goto end;
			}
			break;
		case 'G':		// Global object def/ref graph (data dependency)
			for (Fileidset::const_iterator j = i->glob_uses().begin(); j != i->glob_uses().end(); j++) {
				if (!all && j->get_readonly())
					continue;
				if (only_visited && !j->is_visited())
					continue;
				gd->edge(*j, *i);
				if (browse_only && count++ >= MAX_BROWSING_GRAPH_ELEMENTS)
					goto end;
			}
			break;
		default:
			csassert(0);
			break;
		}
	}
end:
	gd->tail();
		if (gd->uses_swill) {
		if(gtype != NULL) delete gtype;		// Graph type
		if(ltype != NULL) delete ltype;
	}
}

// Graph: text
// static void
// graph_txt_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	// Add output and outfile argument to enable output to outfile
// 	int output = server.getIntParam("output");
// 	const char *outfile = server.getStrParam("outfile").c_str();
// 	if (!output && (outfile != NULL) && output && strlen(outfile)) {
// 		FILE *ofile = fopen(outfile, "w+");
// 		GDTxt gdout(ofile);
// 		graph_fun(&gdout);
// 		fclose(ofile);
// 	}

// 	if (process_mode != pm_r_option) {
// 		GDTxt gd(fo);
// 		graph_fun(&gd);
// 	}

// }

// // Graph: HTML
// static void
// graph_html_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	GDHtml gd(fo);
// 	graph_fun(&gd);
// }

// // Graph: dot
// static void
// graph_dot_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	GDDot gd(fo);
// 	graph_fun(&gd);
// }

// // Graph: SVG via dot
// static void
// graph_svg_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	prohibit_remote_access();
// 	GDSvg gd(fo);
// 	graph_fun(&gd);
// }

// // Graph: GIF via dot
// static void
// graph_gif_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	prohibit_remote_access();
// 	GDGif gd(fo);
// 	graph_fun(&gd);
// }


// // Graph: PNG via dot
// static void
// graph_png_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	prohibit_remote_access();
// 	GDPng gd(fo);
// 	graph_fun(&gd);
// }


// // Graph: PDF via dot
// static void
// graph_pdf_page(FILE *fo, void (*graph_fun)(GraphDisplay *))
// {
// 	prohibit_remote_access();
// 	GDPdf gd(fo);
// 	graph_fun(&gd);
// }


// Split a string by delimiter
vector<string> split_by_delimiter(string &s, char delim) 
{
	string buf;                 // Have a buffer string
	stringstream ss(s);       // Insert the string into a stream

	vector<string> tokens; // Create vector to hold our words

	while (getline(ss, buf, delim))
		tokens.push_back(buf);

	return tokens;
}


// // Produce call graphs with -R option
// static void produce_call_graphs(const vector <string> &call_graphs)
// {
// 	char base_splitter = '?';
// 	char opts_splitter = '&';
// 	char opt_spltter = '=';
// 	GDArgsKeys gdargskeys;

// 	for (string url: call_graphs) {
// 		vector<string> split_base_and_opts = split_by_delimiter(url, base_splitter);
// 		if (split_base_and_opts.size() == 0) {
// 			cerr << url << "is not a valid url" << endl;
// 			continue;
// 		}
// 		FILE *target = fopen(split_base_and_opts[0].c_str() , "w+");
// 		string base = split_base_and_opts[0];
// 		GDTxt gd(target);
// 		// Disable swill
// 		gd.uses_swill = false;
// 		vector<string> opts;

// 		if (split_base_and_opts.size() != 1) {

// 			opts = split_by_delimiter(split_base_and_opts[1], opts_splitter);

// 			// Parse opts
// 			for (string opt: opts) {
// 				vector<string> opt_tmp = split_by_delimiter(opt, opt_spltter);
// 				if (opt_tmp.size() < 2) continue;

// 				// Key-value pairs
// 				string key = opt_tmp[0];
// 				string val = opt_tmp[1];

// 				if (!key.compare(gdargskeys.ALL)) {
// 					gd.all = (bool) atoi(val.c_str());
// 				} else if (!key.compare(gdargskeys.ONLY_VISITED)) {
// 					gd.only_visited = (bool) atoi(val.c_str());
// 				} else if (!key.compare(gdargskeys.GTYPE)) {
// 					gd.gtype = strdup(val.c_str());
// 				} else if (!key.compare(gdargskeys.LTYPE)) {
// 					gd.ltype = strdup(val.c_str());
// 				}

// 			}

// 		}

// 		if (!base.compare(gdargskeys.CGRAPH)) {
// 			cgraph_page(&gd);
// 		}
// 		else if (!base.compare(gdargskeys.FGRAPH)) {
// 			fgraph_page(&gd);
// 		}

// 		fclose(target);
// 	}



// }


// Setup graph handling for all supported graph output types
static void
graph_handle(string name, void (*graph_fun)(GraphDisplay *))
{
//	server.addHandler((name + ".html").c_str(), graph_html_page, graph_fun);
// 	swill_handle((name + ".txt").c_str(), graph_txt_page, graph_fun);
// 	swill_handle((name + "_dot.txt").c_str(), graph_dot_page, graph_fun);
// 	swill_handle((name + ".svg").c_str(), graph_svg_page, graph_fun);
// 	swill_handle((name + ".gif").c_str(), graph_gif_page, graph_fun);
// 	swill_handle((name + ".png").c_str(), graph_png_page, graph_fun);
// 	swill_handle((name + ".pdf").c_str(), graph_pdf_page, graph_fun);
// 
}

// Display all projects, allowing user to select
// {
//		project name: project id
// } 
json::value
select_project_page(void *p)
{
	json::value to_return;
	for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++)
		to_return[Project::get_projname(j)] = json::value(j);

	return to_return;
}

// Select a single project (or none) to restrict file/identifier results
json::value
set_project_page(void *p)
{
	std::ostringstream fs;
	json::value to_return;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}

	if (!(current_project = server.getIntParam("projid)"))) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}
	to_return["ok"] = json::value(true); 
	//index_page(fo, p);
	return to_return;
}

// Return version information
static string
version_info(bool html)
{
	ostringstream v;

	string end = html ? "<br />" : "\n";
	string fold = html ? " " : "\n";

	v << "CScout version " <<
	Version::get_revision() << " - " <<
	Version::get_date() << end << end <<
	// 80 column terminal width---------------------------------------------------
	"(c) Copyright 2003-" << ((char *)__DATE__ + string(__DATE__).length() - 4) <<
				 // Current year
	" Diomidis Spinelllis." << end <<
	end <<
	// C grammar
	"Portions Copyright (c) 1989, 1990 James A. Roskind." << end <<
	// MD-5
	"Portions derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm." << end <<

	"Includes the SWILL (Simple Web Interface Link Library) library written by David" << fold <<
	"Beazley and Sotiria Lampoudi.  Copyright (c) 1998-2002 University of Chicago." << fold <<
	"SWILL is distributed under the terms of the GNU Lesser General Public License" << fold <<
	"version 2.1 available " <<
	(html ? "<a href=\"http://www.gnu.org/licenses/lgpl-2.1.html\">online</a>." : "online at http://www.gnu.org/licenses/lgpl-2.1.html.") << end <<

	end <<
	"CScout is distributed as open source software under the GNU" << fold <<
	"General Public License, available in the CScout documentation and ";
	if (html)
		v << "<a href=\"http://www.gnu.org/licenses/\">online</a>.";
	else
		v << "online at" << end <<
		"http://www.gnu.org/licenses/." << end;
	v << "Other licensing options and professional support are available"
		" on request." << end;
	return v.str();
}

// Display information about CScout
// void
// about_page(FILE *fo, void *p)
// {
// 	html_head(fo, "about", "About CScout");
// 	fputs(version_info(true).c_str(), fo);
// 	html_tail(fo);
// }

// Return top directory JSON
json::value top_file(void *p) 
{
	return dir_top("Browse file tree");
}

// Index
// void
// index_page(FILE *of, void *data)
// {
	
// 	html_head(of, "index", "CScout Main Page", "<img src=\"logo.png\">Scout Main Page");
// 	fputs(
// 		"<table><tr><td valign=\"top\">\n"
// 		"<div class=\"mainblock\">\n"
// 		"<h2>Files</h2>\n"
// 		"<ul>\n"
// 		"<li> <a href=\"filemetrics.html\">File metrics</a>\n"
// 		"<li>\n", of);
// 	dir_top("Browse file tree");
// 	fprintf(of,
// 		"<li> <a href=\"xfilequery.html?ro=1&writable=1&match=Y&n=All+Files\">All files</a>\n"
// 		"<li> <a href=\"xfilequery.html?ro=1&match=Y&n=Read-only+Files\">Read-only files</a>\n"
// 		"<li> <a href=\"xfilequery.html?writable=1&match=Y&n=Writable+Files\">Writable files</a>\n");
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qf=1&n=Files+Containing+Unused+Project-scoped+Writable+Identifiers\">Files containing unused project-scoped writable identifiers</a>\n", is_lscope);
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qf=1&n=Files+Containing+Unused+File-scoped+Writable+Identifiers\">Files containing unused file-scoped writable identifiers</a>\n", is_cscope);
// 	fprintf(of, "<li> <a href=\"xfilequery.html?writable=1&c%d=%d&n%d=0&match=L&fre=%%5C.%%5BcC%%5D%%24&n=Writable+.c+Files+Without+Any+Statements\">Writable .c files without any statements</a>\n", FileMetrics::em_nstatement, Query::ec_eq, FileMetrics::em_nstatement);
// 	fprintf(of, "<li> <a href=\"xfilequery.html?writable=1&order=%d&c%d=%d&n%d=0&reverse=0&match=L&n=Writable+Files+Containing+Unprocessed+Lines\">Writable files containing unprocessed lines</a>\n", Metrics::em_nuline, Metrics::em_nuline, Query::ec_gt, Metrics::em_nuline);
// 	fprintf(of, "<li> <a href=\"xfilequery.html?writable=1&c%d=%d&n%d=0&match=L&n=Writable+Files+Containing+Strings\">Writable files containing strings</a>\n", Metrics::em_nstring, Query::ec_gt, Metrics::em_nstring);
// 	fprintf(of, "<li> <a href=\"xfilequery.html?writable=1&c%d=%d&n%d=0&match=L&fre=%%5C.%%5BhH%%5D%%24&n=Writable+.h+Files+With+%%23include+directives\">Writable .h files with #include directives</a>\n", FileMetrics::em_nincfile, Query::ec_gt, FileMetrics::em_nincfile);
// 	fprintf(of, "<li> <a href=\"filequery.html\">Specify new file query</a>\n"
// 		"</ul></div>\n");

// 	fputs(
// 		"<div class=\"mainblock\">\n"
// 		"<h2>File Dependencies</h2>"
// 		"<ul>\n", of);
// 	fprintf(of, "<li> File include graph: <a href=\"fgraph%s?gtype=I\">writable files</a>, ", graph_suffix().c_str());
// 	fprintf(of, "<a href=\"fgraph%s?gtype=I&all=1\">all files</a>", graph_suffix().c_str());
// 	fprintf(of, "<li> Compile-time dependency graph: <a href=\"fgraph%s?gtype=C\">writable files</a>, ", graph_suffix().c_str());
// 	fprintf(of, "<a href=\"fgraph%s?gtype=C&all=1\">all files</a>", graph_suffix().c_str());
// 	fprintf(of, "<li> Control dependency graph (through function calls): <a href=\"fgraph%s?gtype=F&n=D\">writable files</a>, ", graph_suffix().c_str());
// 	fprintf(of, "<a href=\"fgraph%s?gtype=F&n=D&all=1\">all files</a>", graph_suffix().c_str());
// 	fprintf(of, "<li> Data dependency graph (through global variables): <a href=\"fgraph%s?gtype=G\">writable files</a>, ", graph_suffix().c_str());
// 	fprintf(of, "<a href=\"fgraph%s?gtype=G&all=1\">all files</a>", graph_suffix().c_str());
// 	fputs("</ul></div>", of);

// 	fputs(
// 		"<div class=\"mainblock\">\n"
// 		"<h2>Functions and Macros</h2>\n"
// 		"<ul>\n"
// 		"<li> <a href=\"funmetrics.html\">Function metrics</a>\n"
// 		"<li> <a href=\"xfunquery.html?writable=1&ro=1&match=Y&ncallerop=0&ncallers=&n=All+Functions&qi=x\">All functions</a>\n"
// 		"<li> <a href=\"xfunquery.html?writable=1&pscope=1&match=L&ncallerop=0&ncallers=&n=Project-scoped+Writable+Functions&qi=x\">Project-scoped writable functions</a>\n"
// 		"<li> <a href=\"xfunquery.html?writable=1&fscope=1&match=L&ncallerop=0&ncallers=&n=File-scoped+Writable+Functions&qi=x\">File-scoped writable functions</a>\n"
// 		"<li> <a href=\"xfunquery.html?writable=1&match=Y&ncallerop=1&ncallers=0&n=Writable+Functions+that+Are+Not+Directly+Called&qi=x\">Writable functions that are not directly called</a>\n"
// 		"<li> <a href=\"xfunquery.html?writable=1&match=Y&ncallerop=1&ncallers=1&n=Writable+Functions+that+Are++Called+Exactly+Once&qi=x\">Writable functions that are called exactly once</a>\n", of);
// 	fprintf(of, "<li> <a href=\"cgraph%s\">Non-static function call graph</a>", graph_suffix().c_str());
// 	fprintf(of, "<li> <a href=\"cgraph%s?all=1\">Function and macro call graph</a>", graph_suffix().c_str());
// 	fputs("<li> <a href=\"funquery.html\">Specify new function query</a>\n"
// 		"</ul></div>\n", of);

// 	fprintf(of, "</td><td valign=\"top\">\n");

// 	fputs(
// 		"<div class=\"mainblock\">\n"
// 		"<h2>Identifiers</h2>\n"
// 		"<ul>\n"
// 		"<li> <a href=\"idmetrics.html\">Identifier metrics</a>\n",
// 		of);
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&match=Y&qi=1&n=All+Identifiers\">All identifiers</a>\n", is_readonly);
// 	fprintf(of, "<li> <a href=\"xiquery.html?a%d=1&match=Y&qi=1&n=Read-only+Identifiers\">Read-only identifiers</a>\n", is_readonly);
// 	fputs("<li> <a href=\"xiquery.html?writable=1&match=Y&qi=1&n=Writable+Identifiers\">Writable identifiers</a>\n"
// 		"<li> <a href=\"xiquery.html?writable=1&xfile=1&match=L&qi=1&n=File-spanning+Writable+Identifiers\">File-spanning writable identifiers</a>\n", of);
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qi=1&n=Unused+Project-scoped+Writable+Identifiers\">Unused project-scoped writable identifiers</a>\n", is_lscope);
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qi=1&n=Unused+File-scoped+Writable+Identifiers\">Unused file-scoped writable identifiers</a>\n", is_cscope);
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&unused=1&match=L&qi=1&n=Unused+Writable+Macros\">Unused writable macros</a>\n", is_macro);
// 	// xfile is implicitly 0
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&a%d=1&match=T&ire=&fre=&n=Writable+identifiers+that+should+be+made+static&qi=1\">Writable variable identifiers that should be made static</a>\n", is_ordinary, is_lscope);
// 	fprintf(of, "<li> <a href=\"xiquery.html?writable=1&a%d=1&a%d=1&a%d=1&match=T&ire=&fre=&n=Writable+identifiers+that+should+be+made+static&qi=1\">Writable function identifiers that should be made static</a>\n", is_ordinary, is_lscope, is_cfunction);
// 	fprintf(of,
// 		"<li> <a href=\"iquery.html\">Specify new identifier query</a>\n"
// 		"</ul></div>"
// 	);


// 	if (!browse_only)
// 		fputs(
// 			"<div class=\"mainblock\">\n"
// 			"<h2>Operations</h2>"
// 			"<ul>\n"
// 			"<li> <a href=\"options.html\">Global options</a>\n"
// 			" &mdash; <a href=\"save_options.html\">save global options</a>\n"
// 			"<li> <a href=\"replacements.html\">Identifier replacements</a>\n"
// 			"<li> <a href=\"funargrefs.html\">Function argument refactorings</a>\n"
// 			"<li> <a href=\"sproject.html\">Select active project</a>\n"
// 			"<li> <a href=\"about.html\">About CScout</a>\n"
// 			"<li> <a href=\"save.html\">Save changes and continue</a>\n"
// 			"<li> <a href=\"sexit.html\">Exit &mdash; saving changes</a>\n"
// 			"<li> <a href=\"qexit.html\">Exit &mdash; ignore changes</a>\n"
// 			"</ul></div>", of);
// 	fputs("</td></tr></table>\n", of);
// 	html_tail(of);

// }

// Return file information as JSON
// {
//		pathname: "path of the file",
//		readonly: true if file is readonly,
//		(
//			files: [
//				"project names"
//			],
//		)
//		(
//			copies: [
//				"copies path names"
//			],
//		),
//		handEdit: true if file is hand edited,
//		fileDir: "file object adress",
//		queries: {
//			id: file id,
//			read_only: query read only code,
//			lscope: query lscope code,
//			(fedit: true if file is edited ,)
//			fname: "file name",
//			graph_suffix: "graph suffix"
//		},
//		metrics: [
//			[metrics_name0, metrics value0]
//			[metrics_name1, metrics value1]
//			...
//		]
// }


json::value
file_page(void *p)
{
	int id;
	json::value to_return;
	if (!(id = server.getIntParam("id"))) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}
	Fileid i(id);
	const string &pathname = i.get_path();
	to_return["pathname"] = json::value::string(pathname);
	to_return["readonly"] = json::value(i.get_readonly());
	
	if (Option::show_projects->get()) {
		int no = 0;
		for (Attributes::size_type j = attr_end; j < Attributes::get_num_attributes(); j++)
			if (i.get_attribute(j))
				to_return["files"][no++] = json::value::string(Project::get_projname(j));

	}
	if (Option::show_identical_files->get()) {
		const set <Fileid> &copies(i.get_identical_files());

		int no = 0;
		for (set <Fileid>::const_iterator j = copies.begin(); j != copies.end(); j++) {
			
			if (*j != i) {				
				to_return["copies"][no++] = json::value::string(j->get_path());
			}
		}

	}
	char * s = new char[20];
	sprintf(s, "%p", dir_add_file(i));
	to_return["handEdit"] = json::value(i.is_hand_edited());
	to_return["fileDir"] = json::value(s);
	delete s;
	to_return["queries"]["id"] = json::value(i.get_id());
	to_return["queries"]["readOnly"] = json::value(is_readonly);
	to_return["queries"]["lscope"] = json::value(is_lscope);
	if (modification_state != ms_subst && !browse_only)
		to_return["queries"]["fedit"] = json::value(true);

	to_return["queries"]["fname"] = json::value::string(i.get_fname());
	to_return["queries"]["graph_suffix"] = json::value(graph_suffix());

	
	
	for (int j = 0; j < FileMetrics::metric_max; j++){
			to_return["metrics"][j][0] = json::value(Metrics::get_name<FileMetrics>(j)); 
			to_return["metrics"][j][1] = json::value(i.metrics().get_metric(j));
	}
	return to_return;
}

json::value
source_page(void *p)
{
	
	int id;
	json::value to_return;
	
	if (!(id = server.getIntParam("id"))) {
		to_return["error"] = json::value::string("No id found");
		return to_return;
	}
	Fileid i(id);
	
	const string &pathname = i.get_path();
	to_return["source"] = json::value::string(pathname);
// 	modify file_hypertext
	to_return["html"] = file_hypertext(&i, false);
	
	return to_return;

}

// Edit file returns JSON
// {
//		ok:true if edited or else
//		error: "error message"
// }
static json::value
fedit_page(void *p)
{
	json::value to_return;
	if (modification_state == ms_subst) {
		to_return["error"] = change_prohibited();
		return to_return;
	}
	std::ostringstream fs;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}
	int id;
	if (!(id = server.getIntParam("id"))) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}
	Fileid i(id);
	i.hand_edit();
	const char *re = server.getCharPParam("re");
	char buff[4096];
	snprintf(buff, sizeof(buff), Option::start_editor_cmd ->get().c_str(), (re ? re : "^"), i.get_path().c_str());
	if(DP())
		cout << "Running " << buff << endl;
	
	if (system(buff) != 0) {
		to_return["error"] = json::value::string("Launching" + string(buff) + "failed");
		if(re != NULL) delete re;
		return to_return;
	}
		
	modification_state = ms_hand_edit;
	to_return["ok"] = json::value(true);
	if(re != NULL) delete re;
	return to_return;
}

// Return file source code with links based on a query
// {
//		(qname: "query name",)
//		pathname: "path name",
//		html: { file_hypertext return JSON}
// }
json::value
query_source_page(void *)
{
	int id;
	json::value to_return;
	
	if (!(id = server.getIntParam("id"))) {
		to_return["error"] = json::value::string("File not found");
		return to_return;
	}

	id =  server.getIntParam("id"); 
	
	Fileid i(id);
	const string &pathname = i.get_path();
	const char *qname;
	qname = server.getCharPParam("n");

	if (qname && *qname)
		to_return["qname"] = json::value::string(qname);		

	to_return["pathname"] = json::value::string(pathname);

	to_return["html"] = file_hypertext(&i, true);
	if(qname != NULL) delete qname;
	cout << "query sent" << endl;
	return to_return;
}

// Return info about includes for a file based on a query
// {
//		(qname: "query name",)
//		pathname: "path name of file",
//		table: {
//			h: "html table start",
//			hend: "html table header end",
//			content: "html of table body",
//			end: "html of table end"
//		},
//		(
//			data: [
//				{
//					id: file id,
//					name: "file name",
//					(includes: [
//						{
//							id: included file id,
//							name: "included file name"
//						}
//					],)
//					required: true if it is required
//				}
//			],
//		)
//		end: "html end of page"
// }
json::value
query_include_page(void *p)
{
	int id;
	json::value to_return;
	if (!(id = server.getIntParam("id"))) {
		to_return["error"] = json::value::string("Missing value");
		return to_return;
	}
	Fileid f(id);
	const string &pathname = f.get_path();
	const char *qname = server.getCharPParam("n");
	if (qname && *qname)
		to_return["qname"] = json::value(qname);
	
	to_return["pathname"] = json::value::string(html(pathname));
	
	bool writable = !!server.getBoolParam("writable");
	bool direct = !!server.getBoolParam("direct");
	bool unused = !!server.getBoolParam("unused");
	bool used = !!server.getBoolParam("used");
	bool includes = !!server.getBoolParam("includes");
	const FileIncMap &m = includes ? f.get_includes() : f.get_includers();
	to_return["table"]["h"] = json::value::string(html_file_begin());
	to_return["table"]["hend"] = json::value::string(html_file_set_begin());
	std::ostringstream fs;
	int no = 0;
	for (FileIncMap::const_iterator i = m.begin(); i != m.end(); i++) {
		Fileid f2 = (*i).first;
		const IncDetails &id = (*i).second;
		if ((!writable || !f2.get_readonly()) &&
		    (!direct || id.is_directly_included()) &&
		    (!used || id.is_required()) &&
		    (!unused || !id.is_required())) {
			fs << html_file(f2);
			to_return["data"][no]["id"] = json::value(f2.get_id());
			to_return["data"][no]["name"] = json::value(f2.get_path());

			if (id.is_directly_included()) {
				fs << "<td>line ";
				const set <int> &lines = id.include_line_numbers();
				int m = 0;
				for (set <int>::const_iterator j = lines.begin(); j != lines.end(); j++) {
					fs << " <a href=\"src.html?id=" << (includes ? f : f2).get_id() << "#" << *j << "\">" << *j << "</a> ";
					to_return["data"][no]["includes"][m]["id"] = json::value((includes ? f : f2).get_id());
					to_return["data"][no]["includes"][m++]["name"] = json::value(*j);
				}
				to_return["data"][no]["required"] = json::value(id.is_required());
				no++;
				if (!id.is_required())
					fs << " (not required)";
				fs << "</td>";
			}
			fs << html_file_record_end();
		}
	}
	to_return["table"]["content"] = json::value::string(fs.str());
	to_return["table"]["end"] = json::value::string(html_file_end());
	to_return["end"] = json::value::string("</ul>\n");
	if(qname != NULL) delete qname;
	return to_return;
}


// Return replacement page
// {
//		form: "html start form table",
//		content: [
//			{
//				start:	"html of row start",
//				name: "html of link to id page",
//				text: "html of input text",
//				checkbox: "html of checkbox",
//				id: "Eclass object address",
//				id_adress: "id address",
//				new_id: "new id",
//				active: true if active id replacement
//			},
//		],
//		end: "html of page end"
// }
static json::value
replacements_page(void *p)
{
	json::value to_return;
	// define JSON func
	if(DP()) {
		cerr << "Creating identifier list" << endl;
	}
	ostringstream fs;
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}
	to_return["form"] = json::value::string("<form action=\"xreplacements.html\" method=\"put\">\n"
		"<table><tr><th>Identifier</th><th>Replacement</th><th>Active</th></tr>\n");
	
	int no = 0;
	

	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		if (i->second.get_replaced()) {
			fs.flush();
			to_return["content"][no]["start"] = json::value::string("<tr><td>");
			to_return["content"][no]["name"] = json::value::string(html(*i));
			fs << "</td><td><input type=\"text\" name=\"r" << &(i->second)
			<< "\" value=\"" << i->second.get_newid() << "\" size=\"10\" maxlength=\"256\"></td>";
			to_return["content"][no]["text"] = json::value::string(fs.str());
			fs.flush();
			fs << "<td><input type=\"checkbox\" name=\"a" << &(i->second) << "\" value=\"1\" "
			<< (i->second.get_active() ? "checked" : "") << "></td></tr>\n";
			to_return["content"][no]["checkbox"] = json::value::string(fs.str());
			fs.flush();
			fs << &(i->first);
			to_return["content"][no]["id"] = json::value(fs.str());
			fs.flush();
			fs << &(i->second);
			to_return["content"][no]["id_adress"] = json::value::string(fs.str());
			to_return["content"][no]["new_id"] = json::value::string(i->second.get_newid());
			to_return["content"][no++]["active"] = json::value(i->second.get_active());
		}
	}
	to_return["end"] = json::value::string("</table><p><INPUT TYPE=\"submit\" name=\"repl\" value=\"OK\">\n");

	return to_return;

}

// Process an identifier replacements form
// Return { ok: true} if replacement was succesful
// or {error: "error message"}
static json::value
xreplacements_page(void *p)
{
	json::value to_return;
	ostringstream fs;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}
	cerr << "Creating identifier list" << endl;

	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		if (i->second.get_replaced()) {
			char varname[128];
			snprintf(varname, sizeof(varname), "r%p", &(i->second));
			const char *subst;
			
			if ((subst = server.getCharPParam(varname)) != NULL) {
				string ssubst(subst);
				i->second.set_newid(ssubst);
				delete subst;
			}

			snprintf(varname, sizeof(varname), "a%p", &(i->second));
			i->second.set_active(!!server.getBoolParam(varname));
		}
	}

	to_return["ok"] = json::value(true);
	return to_return;

}

// Return function argument refactorings
// {
//		form: "html start of form",
//		table: {
//	 		start: "html head start",
//			contents: [
//				"html of table row"
//			],
//			end: "html end of table"
//		},
//		data: [
//			{
//				address: "function Eclass object address",
//				f: "function call object adress",
//				name: "function name",
//				replacement: "function replacement",
//				active: true if refactoring active
//			}
//		]
// }
static json::value
funargrefs_page( void *p)
{
	json::value to_return;
	ostringstream fs;
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}
	to_return["form"] = json::value::string("<form action=\"xfunargrefs.html\" method=\"get\">\n");
	to_return["table"]["start"] = json::value::string("<table><tr><th>Function</th><th>Arguments</th><th>Active</th></tr>\n");

	int no = 0;

	for (RefFunCall::store_type::iterator i = RefFunCall::store.begin(); i != RefFunCall::store.end(); i++) {
		fs.flush();
		fs << i->first;
		to_return["table"]["contents"][no] = json::value::string("<tr><td>" + html(*(i->second.get_function()))
		+ "</td><td><input type=\"text\" name=\"r" + fs.str() + "\" value=\"" + i->second.get_replacement() +
		"\" size=\"10\" maxlength=\"256\"></td>""<td><input type=\"checkbox\" name=\"a" + fs.str() +
		"\" value=\"1\" " + (i->second.is_active() ? "checked" : "") + "></td></tr>\n");
		to_return["data"][no]["address"] = json::value(fs.str());
		fs.flush();
		fs << i->second.get_function();
		to_return["data"][no]["f"] = json::value(fs.str());
		to_return["data"][no]["name"] = json::value(i->second.get_function()->get_name());
		to_return["data"][no]["replacement"] = json::value(i->second.get_replacement());
		to_return["data"][no++]["active"] = json::value(i->second.is_active());
	}
	to_return["table"]["end"] = json::value::string("</table><p><INPUT TYPE=\"submit\" name=\"repl\" value=\"OK\">\n");
	return to_return;
}

// Process a function argument refactorings form
// Return {ok: true} if exit perimitted
// else {error: "error message"}
static json::value
xfunargrefs_page(void *p)
{
	json::value to_return;
	ostringstream fs;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}

	for (RefFunCall::store_type::iterator i = RefFunCall::store.begin(); i != RefFunCall::store.end(); i++) {
		char varname[128];
		snprintf(varname, sizeof(varname), "r%p", i->first);
		const char *subst;
		if ((subst = server.getStrParam(varname).c_str()) != NULL) {
			string ssubst(subst);
			i->second.set_replacement(ssubst);
		}

		snprintf(varname, sizeof(varname), "a%p", i->first);
		i->second.set_active(!!server.getBoolParam(varname));
	}
	
	to_return["ok"] = json::value(true);
	return to_return;
}

// Save refactorings and replacements and exit return JSON message
// {
//		refactors:[
//			"refactoring messages from files"
//		],
//		statistics: {
//			msg: "msg of statistics",
//			no_id_replacement: number of id replacements,
//			no_fun_refactorings: number of function refactorings,
//			no_files: number of refactored files
//		}
//		(,exit: true if going to exit)
// }
json::value
write_quit_page(void *exit)
{
	// define JSON func
	json::value to_return;
	ostringstream fs;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}

	if (exit) {
		to_return["exit"] = json::value(true);
		must_exit=true;
	}
	else {
		if (Option::sfile_re_string->get().length() == 0) {
			to_return["error"] = json::value::string("Not Allowed");
			return to_return;
		}
	}

	// Determine files we need to process
	IFSet process;
	cerr << "Examining identifiers for renaming" << endl;
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		if (i->second.get_replaced() && i->second.get_active()) {
			Eclass *e = (*i).first;
			IFSet ifiles = e->sorted_files();
			process.insert(ifiles.begin(), ifiles.end());
		}
	}
	cerr << endl;

	// Check for identifier clashes
	Token::found_clashes = false;
	if (Option::refactor_check_clashes->get() && process.size()) {
		cerr << "Checking rename refactorings for name clashes." << endl;
		Token::check_clashes = true;
		// Reparse everything
		Fchar::set_input(input_file_id.get_path());
		Error::set_parsing(true);
		Pdtoken t;
		do
			t.getnext();
		while (t.get_code() != EOF);
		Error::set_parsing(false);
		Token::check_clashes = false;
	}
	if (Token::found_clashes) {
		to_return["error"] = json::value::string("Renamed identifier clashes detected."
		" Errors reported on console output. No files were saved.");
		return to_return;
	}

	cerr << "Examining function calls for refactoring" << endl;
	for (RefFunCall::store_type::iterator i = RefFunCall::store.begin(); i != RefFunCall::store.end(); i++) {
		progress(i, RefFunCall::store);
		if (!i->second.is_active())
			continue;
		Eclass *e = i->first;
		IFSet ifiles = e->sorted_files();
		process.insert(ifiles.begin(), ifiles.end());
	}
	cerr << endl;

	// Now do the replacements
	cerr << "Processing files" << endl;
	int no = 0;
	for (IFSet::const_iterator i = process.begin(); i != process.end(); i++)
		to_return["refactors"][no++] = file_refactor(*i);
	to_return["statistics"]["msg"] = json::value::string("A total of " +
		to_string(num_id_replacements) + " replacements and " +
		to_string(num_fun_call_refactorings) + " function call refactorings were made in " +
		to_string((unsigned)(process.size())) + " files.");
	to_return["statistics"]["no_id_replacement"] = json::value (num_id_replacements); 
	to_return["statistics"]["no_fun_refactorings"] = json::value (num_fun_call_refactorings);
	to_return["statistics"]["no_files"] = json::value((unsigned)(process.size()));
	if (exit) {
		to_return["exit"] = json::value(true);
		must_exit = true;
	} 

	return to_return;
}

// Return {exit: true} if exit perimitted
// else {error: "error message"}
json::value
quit_page(void *p)
{
	json::value to_return;
	ostringstream fs;
	prohibit_browsers(&fs);
	prohibit_remote_access(&fs);
	if (fs.str().length() > 0) {
		to_return["error"] = json::value::string(fs.str());
		return to_return;
	}

	to_return["exit"] = json::value(true);
	must_exit = true;
	return to_return;
}

// Parse the access control list acl.
static void
parse_acl()
{

	ifstream in;
	string ad, host;
	string fname;

	if (cscout_input_file("acl", in, fname)) {
		cerr << "Parsing ACL from " << fname << endl;
		for (;;) {
			in >> ad;
			if (in.eof())
				break;
			in >> host;
			if (ad == "A") {
				cerr << "Allow from IP address " << host << endl;
//	to change		swill_allow(host.c_str());
			} else if (ad == "D") {
				cerr << "Deny from IP address " << host << endl;
//	to change		swill_deny(host.c_str());
			} else
				cerr << "Bad ACL specification " << ad << ' ' << host << endl;
		}
		in.close();
	} else {
		cerr << "No ACL found.  Only localhost access will be allowed." << endl;
//to change		swill_allow("127.0.0.1");
	}
}

// Included file site information
// See warning_report
class SiteInfo {
private:
	bool required;		// True if this site contains at least one required include file
	set <Fileid> files;	// Files included here
public:
	SiteInfo(bool r, Fileid f) : required(r) {
		files.insert(f);
	}
	void update(bool r, Fileid f) {
		required |= r;
		files.insert(f);
	}
	const set <Fileid> & get_files() const { return files; }
	bool is_required() const { return required; }
};

// Generate a warning report
static void
warning_report()
{
	struct {
		const char *message;
		const char *query;
	} reports[] = {
		{ "unused project scoped writable identifier",
		  "L:writable:unused:pscope" },
		{ "unused file scoped writable identifier",
		  "L:writable:unused:fscope" },
		{ "unused writable macro",
		  "L:writable:unused:macro" },
		{ "writable identifier should be made static",
		  "T:writable:obj:pscope" }, // xfile is implicitly 0
	};

	// Generate identifier warnings
	for (unsigned i = 0; i < sizeof(reports) / sizeof(reports[0]); i++) {
		IdQuery query(reports[i].query);

		csassert(query.is_valid());
		for (IdProp::iterator j = ids.begin(); j != ids.end(); j++) {
			if (!query.eval(*j))
				continue;
			const Tokid t = *((*j).first->get_members().begin());
			const string &id = (*j).second.get_id();
			cerr << t.get_path() << ':' <<
				t.get_fileid().line_number(t.get_streampos()) << ": " <<
				id << ": " << reports[i].message << endl;
		}
	}

	/*
	 * Generate unneeded include file warnings
	 * The hard work has already been done by Fdep::mark_required()
	 * Here we do some additional processing, because
	 * a given include directive can include different files on different
	 * compilations (through different include paths or macros)
	 * Therefore maintain a map for include directive site information:
	 */

	typedef map <int, SiteInfo> Sites;
	Sites include_sites;

	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		if (i->get_readonly() ||		// Don't report on RO files
		    !i->compilation_unit() ||		// Algorithm only works for CUs
		    *i == input_file_id ||		// Don't report on main file
		    i->get_includers().size() > 1)	// For files that are both CUs and included
							// by others all bets are off
			continue;
		const FileIncMap &m = i->get_includes();
		// Find the status of our include sites
		include_sites.clear();
		for (FileIncMap::const_iterator j = m.begin(); j != m.end(); j++) {
			Fileid f2 = (*j).first;
			const IncDetails &id = (*j).second;
			if (!id.is_directly_included())
				continue;
			const set <int> &lines = id.include_line_numbers();
			for (set <int>::const_iterator k = lines.begin(); k != lines.end(); k++) {
				Sites::iterator si = include_sites.find(*k);
				if (si == include_sites.end())
					include_sites.insert(Sites::value_type(*k, SiteInfo(id.is_required(), f2)));
				else
					(*si).second.update(id.is_required(), f2);
			}
		}
		// And report those containing unused files
		Sites::const_iterator si;
		for (si = include_sites.begin(); si != include_sites.end(); si++)
			if (!(*si).second.is_required()) {
				const set <Fileid> &sf = (*si).second.get_files();
				int line = (*si).first;
				for (set <Fileid>::const_iterator fi = sf.begin(); fi != sf.end(); fi++)
					cerr << i->get_path() << ':' <<
						line << ": " <<
						"(" << i->const_metrics().get_int_metric(Metrics::em_nuline) << " unprocessed lines)"
						" unused included file " <<
						fi->get_path() <<
						endl;
			}
	}
}

// Report usage information and exit
static void
usage(char *fname)
{
	cerr << "usage: " << fname <<
		" ["
#ifndef WIN32
		"-b|"	// browse-only
#endif
		"-C|-c|-R|-d D|-d H|-E RE|-o|"
		"-r|-s db|-v] "
		"[-l file] "

#ifdef PICO_QL
#define PICO_QL_OPTIONS "q"
		"-q|"
#else
#define PICO_QL_OPTIONS ""
#endif

		"[-p port] [-m spec] file\n"
#ifndef WIN32
		"\t-b\tRun in multiuser browse-only mode\n"
#endif
		"\t-C\tCreate a ctags(1)-compatible tags file\n"
		"\t-c\tProcess the file and exit\n"
		"\t-R\tMake the specified REST API calls and exit\n"
		"\t-d D\tOutput the #defines being processed on standard output\n"
		"\t-d H\tOutput the included files being processed on standard output\n"
		"\t-E RE\tPrint preprocessed results on standard output and exit\n"
		"\t\t(Will process file(s) matched by the regular expression)\n"
		"\t-l file\tSpecify access log file\n"
		"\t-m spec\tSpecify identifiers to monitor (unsound)\n"
		"\t-o\tCreate obfuscated versions of the processed files\n"
		"\t-p port\tSpecify TCP port for serving the CScout web pages\n"
		"\t\t(the port number must be in the range 1024-32767)\n"
#ifdef PICO_QL
		"\t-q\tProvide a PiCO_QL query interface\n"
#endif
		"\t-r\tGenerate an identifier and include file warning report\n"
		"\t-s db\tGenerate SQL output for the specified RDBMS\n"
		"\t-v\tDisplay version and copyright information and exit\n"
		"\t-3\tEnable the handling of trigraph characters\n"
		;
	exit(1);
}

int
main(int argc, char *argv[])
{
	Pdtoken t;
	int c;
	CompiledRE pre;
#ifdef PICO_QL
	bool pico_ql = false;
#endif

	vector<string> call_graphs;
	Debug::db_read();
	ofstream *logfile = NULL;
	while ((c = getopt(argc, argv, "3bCcd:rvE:p:m:l:os:R:" PICO_QL_OPTIONS)) != EOF)
		switch (c) {
		case '3':
			Fchar::enable_trigraphs();
			break;
		case 'E':
			if (!optarg || process_mode)
				usage(argv[0]);
			// Preprocess the specified file
			pre = CompiledRE(optarg, REG_EXTENDED | REG_NOSUB);
			if (!pre.isCorrect()) {
				cerr << "Filename regular expression error:" <<
					pre.getError() << '\n';
				exit(1);
			}
			Pdtoken::set_preprocessed_output(pre);
			process_mode = pm_preprocess;
			break;
		case 'C':
			CTag::enable();
			break;
		#ifdef PICO_QL
		case 'q':
			pico_ql = true;
			/* FALLTHROUGH */
		#endif
		case 'c':
			if (process_mode)
				usage(argv[0]);
			process_mode = pm_compile;
			break;
		case 'd':
			if (!optarg)
				usage(argv[0]);
			switch (*optarg) {
			case 'D':	// Similar to gcc -dD
				Pdtoken::set_output_defines();
				break;
			case 'H':	// Similar to gcc -H
				Fchar::set_output_headers();
				break;
			default:
				usage(argv[0]);
			}
			break;
		case 'p':
			if (!optarg)
				usage(argv[0]);
			portno = atoi(optarg);
			if (portno < 1024 || portno > 32767)
				usage(argv[0]);
			break;
		case 'm':
			if (!optarg)
				usage(argv[0]);
			monitor = IdQuery(optarg);
			break;
		case 'r':
			if (process_mode)
				usage(argv[0]);
			process_mode = pm_report;
			break;
		case 'v':
			cout << version_info(false);
			exit(0);
		case 'b':
			browse_only = true;
			break;
		case 'l':
			if (!optarg)
				usage(argv[0]);
			logfile = new ofstream(optarg);
			if (!(*logfile).is_open()) {
				perror(optarg);
				exit(1);
			}
			
			break;
		case 'o':
			if (process_mode)
				usage(argv[0]);
			process_mode = pm_obfuscation;
			break;
		case 's':
			if (process_mode)
				usage(argv[0]);
			if (!optarg)
				usage(argv[0]);
			process_mode = pm_database;
			db_engine = strdup(optarg);
			break;
		case 'R':
			if (!optarg)
				usage(argv[0]);
			process_mode = pm_r_option;
			call_graphs.push_back(string(optarg));
			break;
		case '?':
			usage(argv[0]);
		}


	// We require exactly one argument
	if (argv[optind] == NULL || argv[optind + 1] != NULL)
		usage(argv[0]);

	utility::string_t address = U("http://localhost:");
	address.append(U(to_string(portno)));
	
	
	if (process_mode != pm_compile && process_mode != pm_preprocess) {
		

		
		server = HttpServer(address,logfile);
		Option::initialize();
		options_load();
		parse_acl();
	}

	if (db_engine) {
		if (!Sql::setEngine(db_engine))
			return 1;
		cout << Sql::getInterface()->begin_commands();
		workdb_schema(Sql::getInterface(), cout);
	}

	Project::set_current_project("unspecified");

	// Set the contents of the master file as immutable
	Fileid fi = Fileid(argv[optind]);

	fi.set_readonly(true);

	// Pass 1: process master file loop
	Fchar::set_input(argv[optind]);
	Error::set_parsing(true);
	do
		t.getnext();
	while (t.get_code() != EOF);
	Error::set_parsing(false);

	if (process_mode == pm_preprocess)
		return 0;

	input_file_id = Fileid(argv[optind]);

	Fileid::unify_identical_files();

	if (process_mode == pm_obfuscation)
		return obfuscate();

	// Pass 2: Create web pages
	files = Fileid::files(true);

	

	if (process_mode != pm_compile) {
		server.addHandler("sproject.html",select_project_page, 0);
		/*change these functions*/
		server.addHandler("replacements.html", replacements_page, 0);
		server.addPutHandler("xreplacements.html", xreplacements_page, NULL);
		server.addHandler("funargrefs.html", funargrefs_page, 0);
		server.addHandler("xfunargrefs.html", xfunargrefs_page, NULL);
		server.addHandler("options.html", options_page, 0);
		server.addPutHandler("soptions.html", set_options_page, 0);
		server.addPutHandler("save_options.html", save_options_page, 0);
		json::value arg = json::value::string("exit");
		server.addPutHandler("sexit.html", write_quit_page, &arg);
		server.addHandler("save.html", write_quit_page, 0);
		server.addPutHandler("qexit.html", quit_page, 0);

	}

	// Populate the EC identifier member and the directory tree
	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		file_analyze(*i);
		dir_add_file(*i);
	}

	// Update file and function metrics
	file_msum.summarize_files();
	fun_msum.summarize_functions();

	// Set runtime file dependencies
	GlobObj::set_file_dependencies();

	// Set xfile and  metrics for each identifier
	cerr << "Processing identifiers" << endl;
	for (IdProp::iterator i = ids.begin(); i != ids.end(); i++) {
		progress(i, ids);
		Eclass *e = (*i).first;
		IFSet ifiles = e->sorted_files();
		(*i).second.set_xfile(ifiles.size() > 1);
		// Update metrics
		id_msum.add_unique_id(e);
	}
	cerr << endl;

	if (DP())
		cout << "Size " << file_msum.get_total(Metrics::em_nchar) << endl;

	if (Sql::getInterface()) {
		workdb_rest(Sql::getInterface(), cout);
		Call::dumpSql(Sql::getInterface(), cout);
		cout << Sql::getInterface()->end_commands();
#ifdef LINUX_STAT_MONITOR
		char buff[100];
		sprintf(buff, "cat /proc/%u/stat >%u.stat", getpid(), getpid());
		if (system(buff) != 0) {
			fprintf(stderr, "Unable to run %s\n", buff);
			exit(1);
		}
#endif
		return 0;
	}

	if (process_mode != pm_compile) {
		server.addHandler("src.html", source_page, NULL);
		server.addHandler("qsrc.html", query_source_page, NULL);
		server.addPutHandler("fedit.html", fedit_page, NULL);
		server.addHandler("file.html", file_page, NULL);
		server.addHandler("dir.html", dir_page, NULL);

		// Identifier query and execution
		server.addHandler("iquery.html", iquery_page, NULL);
		
		server.addHandler("xiquery.html", xiquery_page, NULL);
		// File query and execution
		server.addHandler("filequery.html", filequery_page, NULL);
		server.addHandler("xfilequery.html", xfilequery_page, NULL);
		server.addHandler("qinc.html", query_include_page, NULL);

		// Function query and execution
		server.addHandler("funquery.html", funquery_page, NULL);
		server.addHandler("xfunquery.html", xfunquery_page, NULL);

		server.addHandler("id.html", identifier_page, NULL);
		server.addHandler("fun.html", function_page, NULL);
		server.addHandler("funlist.html", funlist_page, NULL);
		server.addHandler("funmetrics.html", function_metrics_page, NULL);
		server.addHandler("filemetrics.html", file_metrics_page, NULL);
		server.addHandler("idmetrics.html", id_metrics_page, NULL);

		graph_handle("cgraph", cgraph_page);
		graph_handle("fgraph", fgraph_page);
		graph_handle("cpath", cpath_page);
		server.addHandler("browseTop.html",top_file, NULL);
	//	server.addHandler("about.html", about_page, NULL);
		server.addHandler("setproj.html", set_project_page, NULL);
		// server.addHandler("logo.png", logo_page, NULL);
		//server.addHandler("index.html", (void (*)(FILE *, void *))((char *)index_page), 0);
	}

	if (file_msum.get_writable(Metrics::em_nuline)) {
		ostringstream msg;
		msg << file_msum.get_writable(Metrics::em_nuline) <<
		    " conditionally compiled writable lines" << endl <<
		    "(out of a total of " <<
		    (int)file_msum.get_writable(Metrics::em_nline) <<
		    " writable lines) were not processed";
		Error::error(E_WARN, msg.str(), false);
	}

	CTag::save();
	if (process_mode == pm_report) {
		if (!must_exit)
			warning_report();
		return (0);
	}

#ifdef PICO_QL
	if (pico_ql) {
		pico_ql_register(&files, "files");
		pico_ql_register(&Identifier::ids, "ids");
		pico_ql_register(&Tokid::tm, "tm");
		pico_ql_register(&Call::functions(), "fun_map");
		while (pico_ql_serve(portno))
			;
		return (0);
	}
#endif

	if (process_mode == pm_r_option) {
		cerr << "Producing call graphs for: ";
		for (string d : call_graphs) cerr << d << " ";
		cerr << endl;
		// produce_call_graphs(call_graphs);

		return (0);
	}

	if (process_mode == pm_compile)
		return (0);
	if (DP())
		cout  << "Tokid EC map size is " << Tokid::map_size() << endl;
	// Serve web pages
	if (!must_exit) {
		cerr << "CScout is now ready to serve you at http://localhost:" << portno << endl;
		
	}

	if (!must_exit) {
		server.serve();	
	}

#ifdef NODE_USE_PROFILE
	cout << "Type node count = " << Type_node::get_count() << endl;
#endif
	return (0);
}


/*
 * Clear equivalence classes that do not satisfy the monitoring criteria.
 * Called after processing each input file, for that file.
 */
void
garbage_collect(Fileid root)
{
	vector <Fileid> files(Fileid::files(false));
	set <Fileid> touched_files;

	int count = 0;
	int sum = 0;

	root.set_compilation_unit(true);
	for (vector <Fileid>::iterator i = files.begin(); i != files.end(); i++) {
		Fileid fi = (*i);

		/*
		 * All files from which we input data during parsing
		 * are marked as in need for GC. Therefore all the files
		 * our parsing touched are marked as dirty
		 * (and will be marked clean again at the end of this loop)
		 */
		if (fi.garbage_collected())
			continue;

		fi.set_required(false);	// Mark the file as not being required
		touched_files.insert(fi);

		if (!monitor.is_valid()) {
			fi.set_gc(true);	// Mark the file as garbage collected
			continue;
		}

		const string &fname = fi.get_path();
		fifstream in;

		in.open(fname.c_str(), ios::binary);
		if (in.fail()) {
			perror(fname.c_str());
			exit(1);
		}
		// Go through the file character by character
		for (;;) {
			Tokid ti;
			int val;

			ti = Tokid(fi, in.tellg());
			if ((val = in.get()) == EOF)
				break;
			mapTokidEclass::iterator ei = ti.find_ec();
			if (ei != ti.end_ec()) {
				sum++;
				Eclass *ec = ei->second;
				IdPropElem ec_id(ec, Identifier());
				if (!monitor.eval(ec_id)) {
					count++;
					ec->remove_from_tokid_map();
					delete ec;
				}
			}
		}
		in.close();
		fi.set_gc(true);	// Mark the file as garbage collected
	}
	if (DP())
		cout << "Garbage collected " << count << " out of " << sum << " ECs" << endl;

	// Monitor dependencies
	set <Fileid> required_files;

	// Recursively mark all the files containing definitions for us
	Fdep::mark_required(root);
	// Store them in a set to calculate set difference
	for (set <Fileid>::const_iterator i = touched_files.begin(); i != touched_files.end(); i++)
		if (*i != root && *i != input_file_id)
			root.includes(*i, /* directly included (conservatively) */ false, i->required());
	if (Sql::getInterface())
		Fdep::dumpSql(Sql::getInterface(), root);
	Fdep::reset();

	return;
}
