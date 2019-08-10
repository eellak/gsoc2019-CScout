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
 * Portable graph display abstraction
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
#include <cerrno>		// errno


#include "getopt.h"

#include <regex.h>

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
#include "html.h"
#include "option.h"

#include "gdisplay.h"

#if defined(unix) || defined(__unix__) || defined(__MACH__)
#include <sys/types.h>		// mkdir
#include <sys/stat.h>		// mkdir
#include <unistd.h>		// unlink
#elif defined(WIN32)
#include <io.h>			// mkdir
#include <fcntl.h>		// O_BINARY
#endif

void
GDDot::head(const char *fname, const char *title, bool empty_node) {
	fprintf(fdot, "#!/usr/local/bin/dot\n"
		"#\n# Generated by CScout %s - %s\n#\n\n"
		"digraph %s {\n",
		Version::get_revision().c_str(),
		Version::get_date().c_str(), fname);
	if (Option::dot_graph_options->get().length() > 0)
		fprintf(fdot, "\t%s;\n", Option::dot_graph_options->get().c_str());
	if (empty_node)			// Empty nodes
		fprintf(fdot, "\tnode [height=.001,width=0.000001,shape=box,label=\"\",fontsize=8];\n");
	else
		fprintf(fdot, "\tnode [%s];\n", Option::dot_node_options->get().c_str());
	fprintf(fdot, "\tedge [%s];\n", Option::dot_edge_options->get().c_str());
}

void
GDDotImage::head(const char *fname, const char *title, bool empty_node)
{
	#if defined(unix) || defined(__unix__) || defined(__MACH__)
	strcpy(dot_dir, "/tmp");
	#elif defined(WIN32)
	char *tmp = getenv("TEMP");
	strcpy(dot_dir, tmp ? tmp : ".");
	#else
	#error "Don't know how to obtain temporary directory"
	#endif
	strcat(dot_dir, "/CS-XXXXXX");
	if (mkdtemp(dot_dir) == NULL) {
		*fo << "Unable to create temporary directory " << string(dot) << endl;
		return;
	}
	strcpy(dot, dot_dir);
	strcat(dot, "/in.dot");
	strcpy(img, dot_dir);
	strcat(img, "/out.img");
	fdot = fopen(dot, "w");
	if (fdot == NULL) {
		*fo << "Unable to open " + string(dot) + " for writing";
		return;
	}
	GDDot::head(fname, title, empty_node);
}

void
GDDotImage::tail()
{
	GDDot::tail();
	fclose(fdot);
	/*
	 * Changing to the tmp directory overcomes the problem of Cygwin
	 * differences between CScout and dot file paths
	 */
	cout << "here closed:" << cmd << endl;
	snprintf(cmd, sizeof(cmd), "cd %s && dot -T%s in.dot -o out.img ",
			dot_dir, format);
	cout << "here closed" << cmd << endl;
	
	if (DP())
		cout << cmd << '\n';
	cout << "cmd:" << cmd << endl;
	pid_t p = fork();
	
	switch(p){
		case(-1):
			cerr << "Fork Failed- Errno:" << errno <<endl;
			break;
		case(0):
			if (execl("/bin/sh","/bin/sh","-c",cmd,NULL) != 0) {
				cout << "cmd failed-"<< errno << endl;
				cout << "Unable to execute " << string(cmd) << ". Shell execution";
				raise(SIGKILL);
			}
			break;
		default:
			int stat;
			cout << "wait: " << waitpid(p, &stat, NULL) << endl;
			cout << "exited:" << WTERMSIG(stat) << endl;
			if(!WIFEXITED(stat)) {
				cout << "ex stat:" << WEXITSTATUS(stat);}
				cerr << "Shell didn't terminate peacefully" << endl;
			//	return;
		//	}
	}

	FILE *fimg = fopen(img, "rb");
	if (fimg == NULL) {
		*fo << "Unable to open " << string(img) << " for reading";
		return;
	}
	char c;
	#ifdef WIN32
	setmode(fileno(result), O_BINARY);
	#endif

	while ((c = getc(fimg)) != EOF)
		(*result) << c ;
	fclose(fimg);
	(void)unlink(dot);
	(void)unlink(img);
	(void)rmdir(dot_dir);
}
