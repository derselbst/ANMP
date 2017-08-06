
#include "types.h"

#include <string>


// exception-free waiting
// gnu's stdc++ throws an exception if future not valid; llvm's libc++ needs the valid() call
#define WAIT(future)     try{ if(future.valid()) future.wait(); } catch(future_error& e){}

using namespace std;

#ifndef _POSIX_VERSION
// helper function for case insensitive string compare
// will work on all platforms, but probably slow
bool iEqualsUgly(string strFirst, string strSecond);
#endif

// helper function for case insensitive string compare
bool iEquals(const string& str1, const string& str2);

unsigned long parse_time_crap(const char *input);

string framesToTimeStr(frame_t f, const unsigned int& sampleRate);

frame_t msToFrames(const size_t& ms, const unsigned int& sampleRate);

string mybasename(const string&);
string mydirname(const string&);
bool myExists(const string& name);
string myHomeDir();

string getUniqueFilename(const string& path);
string getFileExtension(const string& filePath);
size_t getFileSize(FILE* f);
size_t getFileSize(int fd);


Nullable<string> findSoundfont(string midFile);
