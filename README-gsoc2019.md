More info about the project can be found in this project's [wiki](https://github.com/eellak/gsoc2019-CScout/wiki) and the original project's [page](https://www.spinellis.gr/cscout/).

This project has a [gist](https://gist.github.com/dimstil/3a65a18b675235e367eeefee2e554ccb).

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
## Contributors
Mentors: Diomidis Spinellis [dspinellis](https://github.com/dspinellis)  
Mentors: ΑΝΑΣΤΑΣΙΑ ΔΕΛΙΓΚΑ  
GSOC-2019 Participant: Dimitrios Styliaras [dimstil](https://github.com/dimstil)  
Orginization: Open Technologies Alliance - GFOSS  

## Links
For more details, examples, and documentation visit the project's
[web site](http://www.spinellis.gr/cscout).
[Original Repo](https://github.com/dspinellis/cscout).
