#include <string>
#include "types.h"

// exception-free waiting
#define WAIT(future)     try{ future.wait(); } catch(future_error& e){}

using namespace std;

#ifndef _POSIX_VERSION
// helper function for case insensitive string compare
// will workon all platforms, but probably slow
bool iEqualsUgly(string strFirst, string strSecond);
#endif

// helper function for case insensitive string compare
bool iEquals(const string& str1, const string& str2);

string getFileExtension(const string& filePath);

unsigned long parse_time_crap(const char *input);

frame_t msToFrames(const size_t& ms, const unsigned int& sampleRate);