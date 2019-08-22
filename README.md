
## Introduction
CScout is a source code analyzer and refactoring browser for collections
of C programs.  It can process workspaces of multiple projects (a project
is defined as a collection of C source files that are linked together)
mapping the complexity introduced by the C preprocessor back into
the original C source code files.  CScout takes advantage of modern
hardware (fast processors and large memory capacities) to analyze
C source code beyond the level of detail and accuracy provided
by  current compilers and linkers.  The analysis CScout performs takes
into account the identifier scopes introduced by the C preprocessor and
the C language proper scopes and namespaces.  CScout has already been
applied on projects of tens of thousands of lines to millions of lines,
like the Linux, OpenSolaris, and FreeBSD kernels, and the Apache web
server. 

## Project Goals
Although Cscout is great at analyzing C programs, it has the disadvantage of having an outdated web interface. Besides the outdated aesthetics the issue is that it does not take advantage of AJAX technologies.

During GSOC CScout will have these changes: 
* Use Microsoft’s C++ REST SDK to handle 
asynchronous requests from the interface and deliver JSON objects.
* ReactJs interactive and modern interface.
* NodeJs to deploy React's front on a local port.

## Timeline
The project's timeline is implemented following GSoC own timeline.

### Phase 1 (May 27 - June 28)
CScout main programm will use REST to deploy JSON objects on a local port. CScout's backend will be almost fully developed so it can handle local HTTP Requests and reply with the according object

### Phase 2 (June 29 - July 26)
At first frontend backend communication will be achieved(until 1st week of July). Web UI will start taking form and all global characteristics of the interface (Toolbars, Menus etc) will have a standard form. First pages of the app 
will be implemented and AJAX communication for those will be achieved.

### Phase 3 (July 27 - August 26)
Rest of the features will be implemented. There will be constant review  
and debug of UI. All features will be revisited and checked for optimal
user experience. On every step there will be adjustments based on the recieved
user feedback

## Building, Testing, Installing, Using
CScout has been compiled and tested on GNU/Linux (Debian jessie). In order to
build and use CScout you need a Unix (like) system
with a modern C++ compiler, GNU make, Perl and npm.
Building installs [Microsoft's C++ Rest SDK](https://github.com/microsoft/cpprestsdk). 
To test CScout you also need to be able to run Java from the command line,
in order to use the HSQLDB database.
To view CScout's diagrams you must have the
[GraphViz](http://www.graphviz.org) dot command in
your executable file path.

* To build run `make`. You can also use the `-j` make option to increase the build's speed.
* To build and test, run `make test`.
* To test Server, run `make testServ`.
* To install (typically after building and testing), run `sudo make install`.
* To see CScout in action run `make example`.

Under FreeBSD use `gmake` rather than `make`.

Testing requires an installed version of _HSQLDB_.
If this is already installed in your system, specify to _make_
the absolute path of the *hsqldb* directory, e.g.
`make HSQLDB_DIR=/usr/local/lib/hsqldb-2.3.3/hsqldb`.
Otherwise, _make_ will automatically download and unpack a local
copy of _HSQLDB_ in the current directory.

## Contributing
* You can contribute to any of the [open issues](https://github.com/dspinellis/cscout/issues) or you can open a new one describing what you want to di.
* For small-scale improvements and fixes simply submit a GitHub pull request.
Each pull request should cover only a single feature or bug fix.
The changed code should follow the code style of the rest of the program.
If you're contributing a feature don't forget to update the documentation.
If you're submitting a bug fix, open a corresponding GitHub issue,
and refer to the issue in your commit.
Avoid gratuitous code changes.
Ensure that the tests continue to pass after your change.
If you're fixing a bug or adding a feature related to the language, add a corresponding test case.
* Before embarking on a large-scale contribution, please open a GitHub issue.

## Links
For more details, examples, and documentation visit the project's
[web site](http://www.spinellis.gr/cscout).
[Original Repo](https://github.com/dspinellis/cscout).
