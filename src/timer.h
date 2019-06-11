#include <stdio.h>
#include<string>

class Timer {
private:
	void *storage;
public:
	Timer();
	~Timer();
	std::string print_elapsed();
};
