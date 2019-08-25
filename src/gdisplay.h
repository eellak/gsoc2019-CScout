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

#ifndef GDISPLAY_
#define GDISPLAY_

#include <string>

using namespace std;

#include "call.h"
#include "html.h"
#include "version.h"
#include "option.h"


// Abstract base class, used for drawing
class GraphDisplay {
protected:
	ostringstream *fo;		// HTTP output file
	FILE *fdot;		// Dot output file
public:
	bool uses_swill = true;
	bool all = false;
	bool only_visited = false;
	char *gtype = NULL;
	char *ltype = NULL;

	GraphDisplay(ostringstream *f) : fo(f), fdot(NULL) {}
	virtual void head(const char *fname, const char *title, bool empty_node) {};
	virtual void subhead(const string &text) {};
	virtual void node(Call *p) {};
	virtual void node(Fileid f) {};
	virtual void edge(Call *a, Call *b) = 0;
	virtual void edge(Fileid a, Fileid b) = 0;
	virtual void error(const char *msg) = 0;
	virtual void tail() {};
	virtual ~GraphDisplay() {}
};

// HTML output
class GDHtml: public GraphDisplay {
public:
	GDHtml(ostringstream *f) : GraphDisplay(f) {}
	virtual void head(const char *fname, const char *title, bool empty_node) {
		*fo << "<table>";
	}

	virtual void subhead(const string &text) {
		*fo << "<h2>" << text << "</h2>\n";
	}

	virtual void edge(Call *a, Call *b) {
		*fo << "<tr><td align=\"right\">" << function_label(a, true) 
			<< "</td><td>&rarr;</td><td>" << function_label(b, true)
			<< "</td></tr>\n";
	}

	virtual void edge(Fileid a, Fileid b) {
		*fo << "<tr><td align=\"right\">" << file_label(b, true)
			<<"</td><td>&rarr;</td><td>" << file_label(a, true) 
			<< "</td></tr>\n";
	}

	virtual void error(const char *msg) {
		*fo << "<h2>"<< msg << "</h2>";
	}

	virtual void tail() {
		*fo << "</table>\n";
	}
	virtual ~GDHtml() {}
};

// Raw text output
class GDTxt: public GraphDisplay {
public:
	GDTxt(ostringstream*f) : GraphDisplay(f) {}
	virtual void edge(Call *a, Call *b) {
		*fo << function_label(a, false) << " " << function_label(b, false) << endl;
	}

	virtual void edge(Fileid a, Fileid b) {
		*fo << file_label(b, false) << " " << file_label(a, false) << endl;
	}

	virtual void error(const char *msg) {
		*fo << msg;
	}

	virtual ~GDTxt() {}
};

// AT&T GraphViz Dot output
class GDDot: public GraphDisplay {
public:
	GDDot(ostringstream*f) : GraphDisplay(f) {}
	virtual void head(const char *fname, const char *title, bool empty_node);
	virtual void node(Call *p) {
		fprintf(fdot, "\t_%p [label=\"%s\"", p, Option::cgraph_show->get() == 'e' ? "" : function_label(p, false).c_str());
		if (isHyperlinked())
			fprintf(fdot, ", URL=\"%p\"", p);
		fprintf(fdot, "];\n");
	}

	virtual void node(Fileid f) {
		fprintf(fdot, "\t_%d [label=\"%s\"", f.get_id(), Option::fgraph_show->get() == 'e' ? "" : file_label(f, false).c_str());
		if (isHyperlinked())
			fprintf(fdot, ", URL=\"file?id=%d\"", f.get_id());
		fprintf(fdot, "];\n");
	}

	virtual void edge(Call *a, Call *b) {
		fprintf(fdot, "\t_%p -> _%p;\n", a, b);
	}

	virtual void edge(Fileid a, Fileid b) {
		fprintf(fdot, "\t_%d -> _%d [dir=back];\n", a.get_id(), b.get_id());
	}

	virtual void error(const char *msg) {
		fprintf(fdot, "\tERROR [label=\"%s\" shape=plaintext];\n", msg);
	}

	virtual void tail() {
		fprintf(fdot, "}\n");
	}
	virtual bool isHyperlinked() { return Option::cgraph_dot_url->get(); }
	virtual ~GDDot() {}
};

// Generate a graph of the specified format by calling dot
class GDDotImage: public GDDot {
private:
	char dot_dir[256];	// Directory for input and output files
	char img[256];		// Absolute image file path
	char dot[256];		// Absolute dot file path
	char cmd[1024];		// dot command
	const char *format;	// Output format
	ostringstream *result;		// Resulting image
public:
	GDDotImage(ostringstream *f, const char *fmt) : GDDot(NULL), format(fmt), result(f) {}
	void head(const char *fname, const char *title, bool empty_node);
	virtual void tail();
	virtual ~GDDotImage() {}
};

// SVG via dot
class GDSvg: public GDDotImage {
public:
	GDSvg(ostringstream *f) : GDDotImage(f, "svg") {}
	virtual bool isHyperlinked() { return (true); }
	virtual ~GDSvg() {}
};

// GIF via dot
class GDGif: public GDDotImage {
public:
	GDGif(ostringstream *f) : GDDotImage(f, "gif") {}
	virtual ~GDGif() {}
};

// PNG via dot
class GDPng: public GDDotImage {
public:
	GDPng(ostringstream *f) : GDDotImage(f, "png") {}
	virtual ~GDPng() {}
};

// PDF via dot
class GDPdf: public GDDotImage {
public:
	GDPdf(ostringstream *f) : GDDotImage(f, "pdf") {}
	virtual bool isHyperlinked() { return (true); }
	virtual ~GDPdf() {}
};

#endif // GDISPLAY_

#ifndef GDARGSKEYS_
#define GDARGSKEYS_

// Static class for GraphDisplay Arguments Keys for parsing
class GDArgsKeys {
public:
	string CGRAPH = "cgraph.txt";
	string FGRAPH = "fgraph.txt";
	string ALL = "all";
	string ONLY_VISITED = "only_visited";
	string GTYPE = "gtype";
	string LTYPE = "n";
};

#endif // GDARGSKEYS_
