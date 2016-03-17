#include <iostream>
#include "scheduler.hpp"
#include "process.hpp"

using namespace std;

int main(int argc, char** argv) {
    if(argc != 4) {
        cout << "[ERROR]: Invalid number of arguments." << endl;
        cout << "[ERROR]: Expected: 4" << endl;
        cout << "[ERROR]: Found: " << argc << endl;
        cout << "Execute with: \"./out time_quantum input_file output_file\"" << endl;
        exit(1);
    }

    Scheduler foo(atoi(argv[1]), argv[2], argv[3]);
    foo.run();
    return 0;
}

