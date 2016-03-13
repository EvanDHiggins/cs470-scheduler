#include "scheduler.hpp"
#include <sstream>

using namespace std;

Scheduler::Scheduler(string input_file, string output_file) :
    input_file(new ifstream(input_file)),
    output_file(new ofstream(output_file)),
    idle_process(new IdleProcess()),
    current_process(idle_process)
{
    if(!this->input_file->good()) {
        cout << "[ERROR]: Input file did not open correctly." << endl;
        exit(1);
    }

    if(!this->output_file->good()) {
        cout << "[ERROR]: Output file did not open correctly." << endl;
        exit(1);
    }
}

void Scheduler::print_state() const {
    *output_file << *get_running_process() << " running" << endl;
    *output_file << "Ready Queue: ";
    for(auto& process : ready_queue) {
        auto ptr = process.lock();
        if(ptr) {
            *output_file << *ptr << " ";
        }
    }
    *output_file << endl << "Wait Queue: ";
    for(auto& process : wait_queue) {
        auto ptr = process.lock();
        if(ptr) {
            *output_file << *ptr << " ";
        }
    }
    *output_file << endl;
}

void Scheduler::run() {
    print_state();
    while(true) {
        string next_action;
        getline(*input_file, next_action);
        parse_action(next_action);

        get_running_process()->tick();

        if(get_running_process()->no_burst_remaining()) {
            //terminate(get_running_process()->get_PID());
            terminate(get_running_process());
        } else if(get_running_process()->no_quantum_remaining()) {
            rotate_queue(ready_queue);
            get_running_process()->set_quantum(TIME_QUANTUM);
        }

        print_state();
    }
}

void Scheduler::terminate(shared_ptr<Process> process) {
    std::cout << process->get_PID() << " terminated" << std::endl;
    //The idle process should not be deleted. It also
    //should never have a request to delete it, but this
    //ensures that.
    if(process->is_idle())
        return;

    shared_ptr<Process> parent(process->get_parent());
    if(parent) {
        parent->remove_child(process);
    }
}

shared_ptr<Process> Scheduler::get_running_process() const {
    if(ready_queue.empty()) {
        return idle_process;
    }
    //Gets the first valid weak_ptr from the ready queue
    while(auto shared_proc = ready_queue.front().lock()) {
        return shared_proc;
    }

    return idle_process;
}

void Scheduler::create_process(int PID, int burst) {
    shared_ptr<Process> child(
            new Process(PID, burst, get_running_process(),
                [this](Process &a){*output_file << a << " terminated" << endl;}));
    get_running_process()->add_child(child);
    ready_enqueue(weak_ptr<Process>(child));
}

void Scheduler::ready_enqueue(weak_ptr<Process> proc) {
    auto shared_proc = proc.lock();
    if(shared_proc) {
        ready_queue.push_back(proc);
        *output_file << *shared_proc << " placed on Ready Queue" << endl;
    }
}

// ============================================================
// Function: parse_action
//
// Parses input and calls necessary functions. Scheduling logic
// is not contained here. It is entirely input validation. And
// control flow delegation.
// ============================================================
void Scheduler::parse_action(string action) {
    *output_file << action << endl;
    vector<string> tokens = split_on_space(action);
    if(tokens.size() < 1) {
        error_unrecognized_action(action);
        return;
    }

    if(tokens[0] == "C") {
        //Create action takes the form: "C # #"
        if(tokens.size() != 3) {
            error_unrecognized_action(action);
            return;
        }

        //Both arguments to "C" must be integers
        if(!is_number_str(tokens[1]) || !is_number_str(tokens[2])) {
            error_unrecognized_action(action);
            return;
        }

        create_process(stoi(tokens[1]), stoi(tokens[2]));
    } else if(tokens[0] == "D") {
        //Destroy action takes the form: "D #"
        if(tokens.size() != 2) {
            error_unrecognized_action(action);
            return;
        }

        //Argument to D must be integer
        if(!is_number_str(tokens[1])) {
            error_unrecognized_action(action);
            return;
        }

        //destroy_action(stoi(tokens[1]));
    } else if(tokens[0] == "I") {
        //Idle
    } else if(tokens[0] == "W") {
        //Wait
    } else if(tokens[0] == "E") {
        //Event
    } else if(tokens[0] == "X") {
        //Exit
        std::exit(0);
    } else {
        error_unrecognized_action(action);
    }
}

void Scheduler::error_unrecognized_action(string action) {
    std::cout << "[ERROR]: Unrecognized command: " << action << std::endl;
}

// ============================================================
// Function: split_on_space
// Returns:  vector<string>
//
// For reasons which I do not understand there is not a split
// function in the C++ standard library. This function is needed
// to tokenize the input actions for parsing.
// ============================================================
vector<string> split_on_space(string str) {
    vector<string> tokens;
    istringstream strstream(str);

    while(strstream.good()) {
        string next_token;
        strstream >> next_token;
        if(!next_token.empty())
            tokens.push_back(next_token);
    }

    return tokens;
}

bool is_number_str(string str) {
    for(int i = 0; i < str.size(); i++) {
        if(!isdigit(str[i]))
            return false;
    }
    return true;
}
