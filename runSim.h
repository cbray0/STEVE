#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <ctime>
#include <limits.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int bash(std::string command,int nice);
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);
void regexReplace(string& input,const string threadnumber);
void runSimRegex(string regex, string num);
bool clean(bool autoClean, string cleanCMD);
bool directoryContains(string dir);
int runSim(string regex, string numSims, string output, string cleanCMD, bool autoClean, bool afterClean);
