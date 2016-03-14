#include "scheduler.hpp"
#include <sstream>

using namespace std;

Scheduler::Scheduler(string input_file, string output_file) :
    input_file(new ifstream(input_file)),
    output_file(new ofstream(output_file)),
    idle_process(new IdleProcess())
{
    if(!this->input_file->good()) {
        cout << "[ERROR]: Input file did not open correctly." << endl;
        exit(1);
    }

    if(!this->output_file->good()) {
        cout << "[ERROR]: Output file did not open correctly." << endl;
        exit(1);
    }

    current_process = idle_process;
}

void Scheduler::print_state() const {
    *output_file << *current_process
                 << " running";
    if(!current_process->is_idle()) {
        *output_file << " with " 
                     << current_process->get_remaining_quantum()
                     << " left";
    }
    *output_file << endl;

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
            *output_file << *ptr << " " << ptr->get_waiting_on();
        }
    }
    *output_file << endl;
}

void Scheduler::run() {
    print_state();
    bool scheduler_done = false;
    while(!scheduler_done) {
        string next_action;
        getline(*input_file, next_action);
        *output_file << next_action << endl;

        current_process->tick();

        update_current_process();

        scheduler_done = parse_action(next_action);
        if(scheduler_done) {
            *output_file << "Current state of simulation:" << endl; 
            print_state();
            break;
        }

        update_current_process();
        print_state();
    }
}


void Scheduler::update_current_process() {
    if(current_process->is_idle()) {
        current_process = next_process();
        current_process->set_quantum(TIME_QUANTUM);

    } else if(!current_process->burst_remaining()) {
        terminate(*current_process);
        current_process = next_process();
        current_process->set_quantum(TIME_QUANTUM);

    } else if(!current_process->quantum_remaining()) {
        ready_enqueue(weak_ptr<Process>(current_process));
        current_process = next_process();
        current_process->set_quantum(TIME_QUANTUM);
    }
}

shared_ptr<Process> Scheduler::next_process() {
    drop_dead_refs(ready_queue);

    if(ready_queue.empty()) {
        return idle_process;
    }

    auto ret = ready_queue.front();
    ready_queue.pop_front();
    return ret.lock();
}

void Scheduler::terminate(Process & process) {
    //The idle process should not be deleted. It also
    //should never have a request to delete it, but this
    //ensures that it won't.
    if(process.is_idle())
        return;

    if(current_process->get_PID() == process.get_PID()) {
        current_process = idle_process;
    }

    auto parent = process.get_parent().lock();
    if(parent) {
        show_terminate_message(process);
        parent->remove_child(process);
    }
}

// ============================================================
// Function: show_terminate_message
//
// Recursively walks the process graph from parent to child and
// outputs a terminate message.
// ============================================================
void Scheduler::show_terminate_message(Process & process) {
    *output_file << process << " terminated" << endl;
    process.for_each_child([this](Process& p){
                show_terminate_message(p);
            });
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
    shared_ptr<Process> child(new Process(PID, burst, current_process));
    current_process->add_child(child);
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
// Return:   bool
//
// Parses input and calls necessary functions. Scheduling logic
// is not contained here. It is entirely input validation. And
// control flow delegation. 
//
// Returns whether the scheduler should cease operation.
// ============================================================
bool Scheduler::parse_action(string action) {
    vector<string> tokens = split_on_space(action);
    if(tokens.size() < 1) {
        error_unrecognized_action(action);
        return true;
    }

    if(tokens[0] == "C") {
        //Create action takes the form: "C # #"
        if(tokens.size() != 3) {
            error_unrecognized_action(action);
            return true;
        }

        //Both arguments to "C" must be integers
        if(!is_number_str(tokens[1]) || !is_number_str(tokens[2])) {
            error_unrecognized_action(action);
            return true;
        }

        create_process(stoi(tokens[1]), stoi(tokens[2]));
    } else if(tokens[0] == "D") {
        //Destroy action takes the form: "D #"
        if(tokens.size() != 2) {
            error_unrecognized_action(action);
            return true;
        }

        //Argument to D must be integer
        if(!is_number_str(tokens[1])) {
            error_unrecognized_action(action);
            return true;
        }

        destroy_by_pid(stoi(tokens[1]));
    } else if(tokens[0] == "I") {
        //Idle
    } else if(tokens[0] == "W") {
        if(tokens.size() != 2) {
            error_unrecognized_action(action);
            return true;
        }

        if(!is_number_str(tokens[1])) {
            error_unrecognized_action(action);
            return true;
        }

        wait_for_event(stoi(tokens[1]));
    } else if(tokens[0] == "E") {
        //Event
        if(tokens.size() != 2) {
            error_unrecognized_action(action);
            return true;
        }

        if(!is_number_str(tokens[1])) {
            error_unrecognized_action(action);
            return true;
        }

        signal_event(stoi(tokens[1]));
    } else if(tokens[0] == "X") {
        //Exit
        return true;
    } else {
        error_unrecognized_action(action);
        return true;
    }
    return false;
}

void Scheduler::wait_for_event(int event_id) {
    if(current_process->is_idle())
        return;

    current_process->wait_on(event_id);

    wait_enqueue(weak_ptr<Process>(current_process));

    //Set to idle until next process switch occurs
    current_process = idle_process;
}

void Scheduler::wait_enqueue(weak_ptr<Process> proc) {
    *output_file << *current_process << " placed on Wait Queue" << endl;
    wait_queue.push_back(proc);
}

void Scheduler::signal_event(int event_id) {
    for(auto i = wait_queue.begin(); i != wait_queue.end();) {
        auto shared_proc = i->lock();
        if(shared_proc) {
            if(shared_proc->receive_event(event_id)) {
                wait_queue.erase(i);
                ready_enqueue(weak_ptr<Process>(shared_proc));
            }
        } else {
            i = wait_queue.erase(i);
        }
    }
}

void Scheduler::destroy_by_pid(int pid) {
    idle_process->for_each_child([pid, this](Process & p) {
                if(p.get_PID() == pid) {
                    terminate(p);
                }
            });
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

