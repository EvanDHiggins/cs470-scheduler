#include <iostream>
#include "scheduler.hpp"
#include "process.hpp"

using namespace std;

int main(int argc, char** argv) {
    if(argc != 3) {
        cout << "[ERROR]: Invalid number of arguments." << endl;
        cout << "[ERROR]: Expected: 3" << endl;
        cout << "[ERROR]: Found: " << argc << endl;
        exit(1);
    }

    Scheduler foo(argv[1], argv[2]);
    foo.run();
    return 0;
}

