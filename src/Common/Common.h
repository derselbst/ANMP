
#include "types.h"

#include <string>


// exception-free waiting
// gnu's stdc++ throws an exception if future not valid; llvm's libc++ needs the valid() call
#define WAIT(future)         \
    try                      \
    {                        \
        if (future.valid())  \
            future.wait();   \
    }                        \
    catch (future_error & e) \
    {                        \
    }

using namespace std;

#ifndef _POSIX_VERSION
// helper function for case insensitive std::string compare
// will work on all platforms, but probably slow
bool iEqualsUgly(std::string strFirst, std::string strSecond);
#endif

// helper function for case insensitive std::string compare
bool iEquals(const std::string &str1, const std::string &str2);

unsigned long parse_time_crap(const char *input);

string framesToTimeStr(frame_t frames, const unsigned int &sampleRate);

frame_t msToFrames(const size_t &ms, const unsigned int &sampleRate);
size_t framesToMs(const frame_t &frames, const unsigned int &sampleRate);

string mybasename(const std::string &);
string mydirname(const std::string &);
bool myExists(const std::string &name);
string myHomeDir();

string getUniqueFilename(const std::string &path);
string getFileExtension(const std::string &filePath);
size_t getFileSize(FILE *f);
size_t getFileSize(int fd);


Nullable<string> findSoundfont(std::string midFile);

bool PageLockMemory(void* ptr, size_t bytes);
void PageUnlockMemory(void* ptr, size_t bytes);
