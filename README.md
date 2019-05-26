[![Build Status](https://travis-ci.org/dspinellis/cscout.svg?branch=master)](https://travis-ci.org/dspinellis/cscout)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/8463/badge.svg)](https://scan.coverity.com/projects/dspinellis-cscout)

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

## Links
For more details, examples, and documentation visit the project's
[web site](http://www.spinellis.gr/cscout).
Original Repo
[Original Repo](https://github.com/dspinellis/cscout).
