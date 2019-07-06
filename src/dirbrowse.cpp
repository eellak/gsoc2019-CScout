/*
 * (C) Copyright 2008-2015 Diomidis Spinellis
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
 * Directory browsing code
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
#include <algorithm> // set_difference
#include <cctype>
#include <sstream> // ostringstream
#include <cstdio>  // perror, rename
#include <cstdlib> // atoi
#include <cerrno>  // errno

#include "getopt.h"
#include "headers.h"
#include "cpp.h"
#include "debug.h"
#include "error.h"
#include "parse.tab.h"
#include "attr.h"
#include "metrics.h"
#include "fileid.h"
#include "html.h"

class DirDir;
class DirFile;

// An entry in a directory
class DirEntry
{
public:
	// Return entry's name
	virtual string get_name() const = 0;
	// Display a link to the entry's contents as HTML on of
	virtual string html() const = 0;
	virtual ~DirEntry() {}
};

// A file
class DirFile : public DirEntry
{
private:
	Fileid id; // File identifier
public:
	DirFile(Fileid i) : id(i) {}

	virtual string get_name() const
	{
		return id.get_fname();
	}
	// Display a link to the files's contents as HTML on of
	virtual string html() const
	{
		return "<a href=\"file.html?id=" + to_string(id.get_id()) + "\">" +
			   id.get_fname() + "</a><br />";
	}
	virtual ~DirFile() {}
};

typedef map<string, DirEntry *> DirContents;

// A directory
class DirDir : public DirEntry
{
private:
	string name;	 // Directory name
	DirDir *parent;  // Parent directory
	DirContents dir; // Directory contents
public:
	static DirDir *root;

	// Construct the root directory
	DirDir()
	{
		name = "/";
		parent = this;
	}
	// Construct a directory given its name and parent
	DirDir(const string &n, DirDir *p) : name(n), parent(p) {}

	// Add a file (if needed)
	void add_file(Fileid id)
	{
		const string &n = id.get_fname();
		if (dir.find(n) == dir.end())
			dir.insert(DirContents::value_type(n, new DirFile(id)));
	}
	
	virtual string get_name() const 
	{
		return name;
	}

	/*
	 * Add a directory (if needed) and return a pointer to it
	 * Return NULL if we are asked to descent an existing file
	 * entry (i.e. not a directory).
	 */
	DirDir *add_dir(const string &n)
	{
		DirDir *ret;
		DirContents::const_iterator i = dir.find(n);
		if (i == dir.end())
		{
			ret = new DirDir(n, this);
			dir.insert(DirContents::value_type(n, ret));
		}
		else
			ret = dynamic_cast<DirDir *>(i->second);
		return ret;
	}

	// Return the directory's full path
	const string get_path() const
	{
		if (parent == this)
			return "";
		else
			return parent->get_path() + "/" + name;
	}

	// Display a link to the directory's contents as HTML on of with the specified name
	string html(const char *n) const
	{
		char *s = new char[20];
		sprintf(s, "%p", this);
		return "<a href=\"dir.html?dir=" + string(s) + "\">" + n + "</a><br />";
	}

	// Display a link to the directory's contents as HTML on of
	virtual string html() const
	{
		return html(name.c_str());
	}

	// Return the directory's contents as HTML on of
	json::value dirlist() const
	{
		json::value to_return;
		char* s = new char[20];
		
		if (parent != this) {
			sprintf(s,"%p",parent);
			to_return["parent"] = json::value(s);
		}
		int no = 0;
		s[0] = 0;
		for (DirContents::const_iterator i = dir.begin(); i != dir.end(); i++){
			sprintf(s,"%p",i->second);
			to_return["children"][no]["addr"] = json::value(s);
			to_return["children"][no++]["name"] = json::value(i->second->get_name());
			s[0] = 0;
		}
		return to_return;
	}
	virtual ~DirDir() {}
	// Return a pointer for browsing the project's top directory
	static DirDir *top()
	{
		// Descent into directories having exactly one directory node
		DirDir *p, *p2;
		for (p = root; p->dir.size() == 1 && (p2 = dynamic_cast<DirDir *>(p->dir.begin()->second)) != NULL; p = p2)
			;
		return p;
	}
};

DirDir *DirDir::root = new DirDir();

// Add a file to the directory tree for later browsing
// Return a pointer suitable for browsing the file's directory
void *
dir_add_file(Fileid f)
{
	string c;
	const string &path = f.get_path();
	string::size_type start = 0;
	DirDir *cd = DirDir::root;

	for (;;)
	{
		string::size_type slash = path.find_first_of("/\\", start);
		if (slash == string::npos)
		{
			cd->add_file(f);
			return cd;
		}
		else
		{
			cd = cd->add_dir(string(path, start, slash - start));
			if (DP())
				cout << "Add element " << string(path, start, slash - start) << endl;
			if (cd == NULL)
			{
				cerr << "Path " << path << " has a combined file/directory entry" << endl;
				return cd;
			}
			start = slash + 1;
		}
	}
}

// Return a directory's contents
// 	Response JSON object in form
// 	{
// 		dir: "html code here",
// 	}
web::json::value
dir_page(void *p)
{
	DirDir *d;
	json::value to_return;
	d = (DirDir *)server.getAddrParam("dir");
	if (d == NULL)
	{
		to_return["error"] = json::value("Missing value");
		return to_return;
	}
	to_return["dir"] = json::value(html(d->get_path()));
	to_return["tree"] = d->dirlist();
	return to_return;
}

// 	Return a URL and HTML for browsing the project's top dir
// 	Response JSON object in form
// 	{
// 		html: "html code here",
// 		addr: "dir.html?dir=Memory address" //resource link for the top directory
// 	}
json::value
dir_top(const char *name)
{
	json::value to_return;
	to_return["html"] = json::value::string(DirDir::top()->html(name));
	char *s = new char[20];
	sprintf(s, "%p", DirDir::top());
	to_return["addr"] = json::value::string("dir.html?dir=" + string(s));
	return to_return;
}
