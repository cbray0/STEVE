#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <ctime>
#include <limits.h>
#include <unistd.h>
#include <cstring>
#include <atomic>

using namespace std;

namespace simulation{
std::atomic<int> returnVal(0);
/**
 @brief Call the given command in bash

 ## Call the given command in bash

 ### Arguments
 * `std::string command` - Command to run. Note that non-interpreters wrappers like nice may not work with aliased commands.

 * `int nice` - nice value to run command at (added to avoid issues with nice not working with aliases). Defualts to zero.

  ### Returns the return value of the given function
*/
int bash(std::string command,int nice=0){
    string shell = "\
    #!/bin/bash \n\
    source ~/.bashrc\n\
    renice -n "+to_string(nice)+" -p $$\n\
    ";
    int i,ret = system((shell+command).c_str());
    i=WEXITSTATUS(ret); // Get return value.
    return i;
}

/**
 @brief Replace all instances of a string in another string with a third string.

 ## Replace all instances of a string in another string with a third string. Credit to Czarek Tomczak on stack overflow for supplying the code in response to a question asked by NullVoxPopuli.

 ### Arguments:
 * `std::string subject` - String to do replacements in.

 * `const std::string& search` - Substring to replace with the `replace` string.

 * `const std::string& replace` - Substring to replace the `search` string with.
*/
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

/**
 @brief Check current directory contains the string provided/

 ## Check current directory contains the string provided/

 ### Arguments:
 * `string dir` - Directory to check if contained

 ### Notes:
 If it is, it returns zero, otherwise it prompts the user if they want to procede anyway or not. Returns 0 if they want to procede and 1 otherwise.
*/
bool directoryContains(string dir){
    char run_path[256];
    getcwd(run_path, 255);
    string path = run_path;
    if(path.find(dir)==string::npos){
        cout << "Warning, not running in "+dir+", which may cause out of storage issues. Please consider running from "+dir+". Press enter to quit or c then enter to continue." << endl;
        char input[2];
        cin.getline(input,2);
        if(input[0]!='c'&&input[0]!='C') return 1;
    }
    return 0;
}


/**
 @brief Apply regex transformations to input string.

 ## Apply regex transformations to input string.

 ### Arguments:
 * `string& input` - Input string reference to modify in place.

 * `const string threadnumber` - Threadnumber string to replace %n with. Only valid for threads, so it defaults to "".

 ### Transformations:
 * "%d" -> date

 * "%t" -> time

 * "%u" -> unix time

 * "%n" -> thread number
*/
void regexReplace(string& input,const string threadnumber=""){
    time_t t = time(NULL);
    char buffer [80];
    strftime (buffer,80,"%F",localtime(&t));
    input=ReplaceString(input,"%d",string(buffer));
    strftime (buffer,80,"%X",localtime(&t));
    input=ReplaceString(input,"%t",string(buffer));
    input=ReplaceString(input,"%u",to_string((int)t));
    input=ReplaceString(input,"%n",threadnumber);
}

/**
 @brief Run a thread of the simulation

 ## Run a thread of the simulation

 ### Arguments:
 * `string regex` - Command to run for each string, where every instance of "%n" will be replaced with the thread number.

 * `string num` - Thread number

*/
void runSimRegex(string regex, string num){
    regexReplace(regex,num);
    cout << regex+"\n";
    int status = bash(regex,11); // Run simulations in bash
    if(status) returnVal = status;
}

/**
 @brief Clean the current directory

 ## Clean the current directory

 ### Arguments:
 * `bool autoClean` - Bool of whether or not to automatically clean the directory. It defualts to zero, which will prompt the user first, but if set to one the program cleans the directory without outside input.

 * `string cleanCMD` - Sh (bourne shell) command used to clean directory. Defaults to `rm -f *.root`.

 ### Notes:
 Ensures there are no files matching g4out*.root in the current directory. If there are, it offers to automatically remove them, or to exit the program to allow the user to save and remove them manually.
 Also note that this assumes that all output files end in `.root`, and no other files end in `.root`. If you want to preserve these files, you should move them to a differnt directory or change their filename (as an example, `g4out.root`->`g4out.root.keep`).
*/
bool clean(bool autoClean, string cleanCMD="rm -f *.root;"){
    if(autoClean){
        bash(cleanCMD.c_str()); // Clean directory
        return 1;
    }
    int i, ret = system(("echo \"There may be conflicting files in your current directory. If there are none, then press s then enter to skip cleaning and continue execution. If you wish to exit the program, press enter to exit. Otherwise, press c then enter to clean up.\" \n read cleanup \n if [ \"$cleanup\" == \"c\" ]; then \n "+cleanCMD+" && exit 1 \n fi \n if [ \"$cleanup\" == \"s\" ]; then \n exit 1 \n fi \nexit 0 \n").c_str()); // Ask user if they want to run the clean command, then run it if necessary
    i=WEXITSTATUS(ret); // Get return value: 1 if clean, 0 if not
    return i;
}

/**
 @brief Runs the multithreading workaround. Exists so that it can be easily called from another program.

 ## Runs the multithreading workaround. Exists so that it can be easily called from another program.

 ### Arguments:
 * `string regex` - Runs the simulation from the string provided once all instances of "%n" are replaced by the thread number. It allows for more flexibility in command execution as the singlethreaded command for any simulation (that has the two modifications described below) can easily be multithreaded. Make sure to redirect stdout and stderr to a file or /dev/null to prevent simultaneous writing to the console. Note that the string should be surrounded in quotes as to not specify additional arguments.

 * `string numSims` - Number of threads to run. Defaults to 48.

 * `string output` - Final combined output .root filename. Defaults to `g4out.root`.

 * `string cleanCMD` - Supply your own clean command (bourne shell only). If not included, default clean command is `rm -f *.root`. This can reference a file if the command executes a file.

 * `bool autoClean` - Automatically cleans directory and skips directory check without user input.

 * `bool afterclean` - Cleans directory after simulation completion by adding .keep to the end of the output file, then running the clean function, which is by defailt `rm -f *.root`, then moving the output back to the original filename.

*/
int runSim(string regex, string numSims = "48", string output = "g4out.root", string cleanCMD = "rm -f *.root", bool autoClean = 0, bool afterClean = 0){
    if(!clean(autoClean,cleanCMD)) return 2; // Cleans the directory of g4out*.root files
    regexReplace(output); // Format the output filename
    vector<thread> threadpool;
    for(int i=0;i<stoi(numSims);i++) threadpool.push_back(thread(runSimRegex, regex, to_string(i))); // Start each numbered simulation in a thread
    for(int i=0;i<stoi(numSims);i++) threadpool[i].join(); // Wait for all simulations to finish
    if(returnVal!=0) return returnVal; // Exit if there are problems
    int i,ret = system(("hadd "+output+" *.root").c_str()); // Merge all histograms
    i=WEXITSTATUS(ret);
    if(i) return i;
    if(afterClean) bash("mv "+output+" "+output+".keep;"+cleanCMD+";mv "+output+".keep "+output);
    return 0;
}
}
