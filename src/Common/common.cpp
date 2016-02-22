#include <cctype>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;

#ifndef _POSIX_VERSION
// helper function for case insensitive string compare
// will workon all platforms, but probably slow
bool iEqualsUgly(string strFirst, string strSecond)
{
  // convert strings to upper case before compare
  transform(strFirst.begin(), strFirst.end(), strFirst.begin(), toupper);
  transform(strSecond.begin(), strSecond.end(), strSecond.begin(), toupper);
  return strFirst == strSecond;
}
#endif

// helper function for case insensitive string compare
bool iEquals(const string& str1, const string& str2)
{
#ifdef _POSIX_VERSION
  return strncasecmp(str1.c_str(), str2.c_str(), min(str1.size(),str2.size())) == 0;  
#else
  return iEqualsUgly(str1, str2);
#endif
}

string getFileExtension(const string& filePath)
{
    return filePath.substr(filePath.find_last_of(".") + 1);
}