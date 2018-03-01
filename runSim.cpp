/**
@file runSim.cpp

@author Connor Bray

@date 28 Feb. 2018

@brief Simulation automation tools repo

## Multithreading workaround code

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

#include "runSim.h"

using namespace std;
using namespace simulation;
/**
 @brief Reads in arguments, then calls runSim()
*/
int main(int argc,char** argv){
    string numSims = "48"; // Default values for all arguments
    bool autoClean = 0;
    bool afterClean = 0;
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
    if(regex=="") {
        cout << "Simulation command required. Exiting." << endl;
        return 0;
    }
    if(!autoClean&&directoryContains("/home/data")) return 1; // Check if directory is in /home/data or for user override.
    return runSim(regex,numSims,output,cleanCMD,autoClean,afterClean);
}
