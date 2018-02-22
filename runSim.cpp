/**
    Multithreading workaround code

    @author Connor Bray
*/
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <ctime>
#include <limits.h>
#include <unistd.h>

using namespace std;
/// To prevent multiple threads from using stdout at the same time, lock this mutex, then print, then unlock it to let another thread use it.
mutex printing;

int bash(std::string command,int nice=0){
    string shell = "\
    #!/bin/bash \n\
    source ~/.bashrc\n\
    ";
    int i, ret = system((shell+command).c_str());
    i=WEXITSTATUS(ret); // Get return value.
    return i;
}

/**
## Replace all instances of a string in another string with a third string. Credit to Czarek Tomczak on stack overflow for supplying the code in response to a question asked by NullVoxPopuli.
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
## Run a thread of the simulation

### Arguments:
* `string regex` - Command to run for each string, where every instance of "%n" will be replaced with the thread number.

* `string num` - Thread number

*/
void runSimRegex(string regex, string num){
    regexReplace(regex,num);
    printing.lock(); // Lock printing to cout without other threads interrupting, then unlock it
    cout << regex << endl;
    printing.unlock();
    bash(regex.c_str()); // Run simulations in bash
}

/**
## Clean the current directory

### Arguments:
* `bool autoClean` - Bool of whether or not to automatically clean the directory. It defualts to zero, which will prompt the user first, but if set to one the program cleans the directory without outside input.

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
##Check that the current directory is in /home/data

### Notes:
If it is, it returns zero, otherwise it prompts the user if they want to procede anyway or not. Returns 0 if they want to procede and 1 otherwise.
*/
bool directoryCheck(){
    char result[ PATH_MAX ]; // Warn user if run from a directory that does not begin with /home/data/
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    string st = std::string( result, (count > 0) ? count : 0 ) ;
    if(st.find("/home/data")==string::npos){
        cout << "Warning, not running in /home/data, which may cause out of storage issues. Please consider running from /home/data. Press enter to quit or c then enter to continue." << endl;
        char input[2];
        cin.getline(input,2);
        if(input[0]!='c'&&input[0]!='C') return 1;
    }
    return 0;
}

/**
## Multithreading workaround

### Arguments:
* `-t threads` - Number of threads to run

*  `-y` - Automatically cleans directory and skips directory check without user input.

*  `--clean` - Supply your own clean command. If not included, default clean command is `rm -f *.root`. This can reference a file if the command executes a file.

* `--remove` - Cleans directory after simulation completion by adding .keep to the end of the output file, then running the clean function, which is by defailt `rm -f *.root`, then moving the output back to the original filename.

* `--regex` - Runs the simulation from the string provided once all instances of "%n" are replaced by the thread number. It allows for more flexibility in command execution as the singlethreaded command for any simulation (that has the two modifications described below) can easily be multithreaded. Make sure to redirect stdout and stderr to a file or /dev/null to prevent simultaneous writing to the console. Note that the string should be surrounded in quotes as to not specify additional arguments.

* `--output` - Final combined output .root filename. Defaults to `g4out.root`.

## Notes:

* The program also wans you if you are running it from a directory not inside of /home/data because of my specific use case.

Changes needed in simulation for runSim to work:
1. Simulation seed must pull from /dev/random
Reference implementation:

\code{.cpp}
#include <fstream>
G4int random_seed(){ // Credit to posop on stackoverflow for code that I adapted. Reading an int from /dev/random is a better random seed than time(NULL) because it is cryptographically secure and protects against multiple instances loading in same second, which was a problem before.
    G4int seed;
    std::ifstream file("/dev/random",std::ios::binary);
    if(file.is_open()){
        char *memblock;
        G4int size=sizeof(G4int);
        memblock=new char [size];
        file.read(memblock,size);
        file.close();
        seed=*reinterpret_cast<G4int*>(memblock);
        delete[] memblock;
        return seed;
    }else{return random_seed();} // Continually retry until /dev/random is free
}
\endcode

Then in int main

\code{.cpp}
G4int seed = random_seed();
G4Random::setTheSeed( seed );
\endcode

2. Simulation must take an argument to change output filename by adding characters (eg. adding the supplied string between the `g4out` and the `.root`). Note that the resulting files must still end in `.root`.
Reference implmentation:

\code{.cpp}
// In main executable, eg. Griffinv10.cc
// In global scope:
G4String file_name="";
// In argument parsing:
if(argc>2) file_name=argv[2];
// In Histomanager.cc
// In HistoManager::HistoManager()
// Replacing fFileName[0] = "g4out";
extern G4String file_name;
fFileName[0] = "g4out"+file_name;
\endcode

## Building
To build runSim.cpp, use
\code{.cpp}
g++ -Wall -std=c++11 -pthread -Ofast -o runSim.o runSim.cpp
\endcode

*/
int main(int argc,char** argv){
    string numSims = "48"; // Default values for all arguments
    bool autoClean = 0;
    bool afterClean = 0;
    string test = "";
    string regex = "";
    string output = "g4out.root";
    string cleanCMD="rm -f *.root";
    for(int i=0;i<argc;i++){ // Assign arguments to values
        if(i<argc-1){
            if(string(argv[i])=="-t") numSims = argv[++i];
            if(string(argv[i])=="--regex") regex = argv[++i];
            if(string(argv[i])=="--output") output = argv[++i];
            if(string(argv[i])=="--clean") cleanCMD = argv[++i];
        }
        if(string(argv[i])=="-y") autoClean = 1;
        if(string(argv[i])=="--remove") afterClean = 1;
    }
    if(!autoClean&&directoryCheck()) return 1; // Check if directory is in /home/data or for user override.
    if(!clean(autoClean,cleanCMD)) return 2; // Cleans the directory of g4out*.root files
    regexReplace(output); // Format the output filename
    vector<thread> threadpool;
    for(int i=0;i<stoi(numSims);i++) threadpool.push_back(thread(runSimRegex, regex, to_string(i))); // Start each numbered simulation in a thread
    for(int i=0;i<stoi(numSims);i++) threadpool[i].join(); // Wait for all simulations to finish
    cout << output << endl; // TODO
    system(("hadd "+output+" *.root").c_str()); // Merge all histograms
    if(afterClean)system(("mv "+output+" "+output+".keep;"+cleanCMD+";mv "+output+".keep "+output).c_str());
    return 0;
}
