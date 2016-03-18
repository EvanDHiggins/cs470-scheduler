// File: scheduler.cpp
// --------------------------------------------------------
// Class: CS 470                      Instructor: Dr. Hwang
// Assignment: Process Scheduling     Date Assigned: 22 February 2016
// Programmer: Evan Higgins           Date Completed: 18 March 2016

#include "scheduler.hpp"
#include <sstream>

using namespace std;

// ============================================================
// Function: Scheduler(string, string)
//
// Constructs input files from passed strings and verifies that
// they opened correctly. Incorrectly opened files are an
// unrecoverable error and the scheduler will exit.
// ============================================================
Scheduler::Scheduler(int quantum,
                     string input_file_name,
                     string output_file_name) :
    input_file(input_file_name),
    output_file(output_file_name),
    idle_process(new IdleProcess())
{
    if(!this->input_file.good()) {
        cerr << "[ERROR]: Input file did not open correctly." << endl;
        exit(1);
    }

    if(!this->output_file.good()) {
        cerr << "[ERROR]: Output file did not open correctly." << endl;
        exit(1);
    }

    time_quantum = quantum;
    current_process = idle_process;
}


// ============================================================
// Function: run()
//
// run() is the input loop for a scheduler. It executes commands
// from input_file line by line and updates the state of all
// processes within it.
// ============================================================
void Scheduler::run() {
    print_state();
    while(true) {

        string next_action;
        getline(input_file, next_action);
        this->output_file << next_action << endl;

        if(next_action == "X") {
            this->output_file << "Current state of simulation:" << endl;
            print_state();
            break;
        }

        current_process->tick();

        parse_action(next_action);

        if(current_process->is_idle()) {
            current_process = get_next_process();
            current_process->set_quantum(time_quantum);
        } else if(current_process->is_exiting()) {
            cascading_terminate(*current_process);
            current_process = get_next_process();
            current_process->set_quantum(time_quantum);
        } else if(!current_process->quantum_remaining()) {
            ready_enqueue(current_process);
            current_process = get_next_process();
            current_process->set_quantum(time_quantum);
        }

        print_state();
    }

    //Close to prevent remaining processes from printing 
    //their terminate messages
    output_file.close();
}

// ============================================================
// Function: print_state()
//
// Prints the state of the scheduler. It lists the running
// process and its remaining quantum, all processes on the
// ready queue and all processes on the wait queue.
// ============================================================
void Scheduler::print_state() {
    output_file << *current_process
                 << " running";
    if(!current_process->is_idle()) {
        output_file << " with "
                     << current_process->get_remaining_quantum()
                     << " left";
    }
    output_file << endl;

    output_file << "Ready Queue: ";
    for(auto& process : ready_queue) {
        //Only valid references are printed.
        auto ptr = process.lock();
        if(ptr) {
            output_file << *ptr << " ";
        }
    }

    output_file << endl << "Wait Queue: ";
    for(auto& process : wait_queue) {
        //Only valid references are printed.
        auto ptr = process.lock();
        if(ptr) {
            output_file << *ptr << " " << ptr->get_waiting_on();
        }
    }

    output_file << endl;
}

// ============================================================
// Function: parse_action
// Return:   bool
//
// Parses input and calls necessary functions. Scheduling logic
// is not contained here. It is entirely input validation. And
// control flow delegation.
// ============================================================
void Scheduler::parse_action(const string & action) {
    vector<string> tokens = split_on_space(action);
    if(tokens.empty()) {
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

        destroy_by_pid(stoi(tokens[1]));

    } else if(tokens[0] == "I") {
        //Execute no action on idle

    } else if(tokens[0] == "W") {
        //Wait action takes the form: "W #"
        if(tokens.size() != 2) {
            error_unrecognized_action(action);
            return;
        }

        //Argument to W must be an integer
        if(!is_number_str(tokens[1])) {
            error_unrecognized_action(action);
            return;
        }

        wait_for_event(stoi(tokens[1]));

    } else if(tokens[0] == "E") {
        //Event action takes the form: "E #"
        if(tokens.size() != 2) {
            error_unrecognized_action(action);
            return;
        }

        //Argument to E must be an integer
        if(!is_number_str(tokens[1])) {
            error_unrecognized_action(action);
            return;
        }

        signal_event(stoi(tokens[1]));

    } else if(tokens[0] == "X") {
        return;

    } else {
        error_unrecognized_action(action);
        return;

    }
}

// ============================================================
// Function: get_next_process
// Returns:  shared_ptr<Process>
//
// Since ready_queue consists of weak_ptr<Process> getting the
// next process from the queue is not as simple as getting the
// queue's head. Instead it requires removing weak_ptrs from
// the head of the list until a valid pointer is found. If none
// are found the running process should be idle.
// ============================================================
shared_ptr<Process> Scheduler::get_next_process() {
    for(auto iter = ready_queue.begin(); iter != ready_queue.end();) {
        auto shared_p = iter->lock();
        if(shared_p) {
            ready_queue.erase(iter);
            return shared_p;
        } else {
            iter = ready_queue.erase(iter);
        }
    }
    return idle_process;
}

// ============================================================
// Function: create_process(int, int)
//
// New processes are implicitly children of the running process.
// Here a new process is initialized and added to the
// ready_queue. Processes are passed a closure which outputs
// a terminate message for their on_delete parameter.
// ============================================================
void Scheduler::create_process(int PID, int burst) {
    //An exiting process's children will die when it terminates so
    //there is no point in creating a new child.
    if(current_process->is_exiting())
        return;

    shared_ptr<Process> child(
            new Process(PID, burst, current_process,
                [this](Process & p) {
                    if(output_file.is_open())
                        output_file << p << " terminated" << endl;
                }));
    current_process->add_child(child);
    if(!current_process->quantum_remaining()) {
        ready_enqueue(current_process);
        current_process = idle_process;
    }
    ready_enqueue(weak_ptr<Process>(child));
}

// ============================================================
// Function: wait_for_event(int)
//
// Moves the currently running process to the wait queue and
// sets it to wait on event_id.
// ============================================================
void Scheduler::wait_for_event(int event_id) {
    if(current_process->is_idle())
        return;

    //An exiting process should not be placed on the wait queue.
    if(current_process->is_exiting())
        return;

    current_process->wait_on(event_id);

    wait_enqueue(weak_ptr<Process>(current_process));

    //Set to idle until next process switch occurs
    current_process = idle_process;
}

// ============================================================
// Function: signal_event(int)
//
// Finds all processes on the event queue which are waiting
// on event_id and are valid references. The whole wait_queue
// must be searched because multiple processes can respond to
// the same event. In this case the processes will be added to
// the ready_queue in the order in which they were placed on
// the wait_queue.
// ============================================================
void Scheduler::signal_event(int event_id) {
    if(!current_process->quantum_remaining()) {
        ready_enqueue(current_process);
        current_process = idle_process;
    }
    for(auto i = wait_queue.begin(); i != wait_queue.end();) {
        auto shared_proc = i->lock();
        if(shared_proc) {
            if(shared_proc->receive_event(event_id)) {
                wait_queue.erase(i);
                ready_enqueue(weak_ptr<Process>(shared_proc));
                return;
            } else {
                ++i;
            }
        } else {
            i = wait_queue.erase(i);
        }
    }
}

// ============================================================
// Function: destroy_by_pid(int)
//
// Recursively searches the Process graph for a matching PID
// and terminates that process. Ignores processes not owned
// by the currently running process.
// ============================================================
void Scheduler::destroy_by_pid(int pid) {
    if(current_process->is_exiting())
        return;

    if(!current_process->owns(pid))
        return;

    idle_process->search_children_until([pid, this](Process & p) {
                if(p.get_PID() == pid) {
                    cascading_terminate(p);
                    return true;
                }
                return false;
            });
}

// ============================================================
// Function: cascading_terminate(Process&)
//
// Processes are represented as a tree of references.
// A parent process P has a vector of shared_ptrs to its children
// and each child process has a weak_ptr reference to its parents.
//
// All references other than these must be weak_ptrs (or temporary
// shared_ptr accesses of these weak_ptrs). This rule ensures
// that once a parent's shared_ptr reference to a child process
// is destructed it will free that process, which will cascade
// down the tree terminating each process.
// ============================================================
void Scheduler::cascading_terminate(Process & process) {
    //The idle process should not be deleted. It also
    //should never have a request to delete it, but this
    //ensures that it won't be.
    if(process.is_idle())
        return;

    //If the current process is being deleted the shared_ptr
    //held by current_process must be deleted otherwise the
    //shared_ptr semantics of process destruction is ignored.
    if(*current_process == process) {
        current_process = idle_process;
    }

    //show_terminate_message(process);
    process.terminate();
}

// ============================================================
// Function: ready_enqueue(weak_ptr<Process>)
//
// Enqueues a process to ready_queue if it is a valid weak_ptr.
// ============================================================
void Scheduler::ready_enqueue(const weak_ptr<Process> & proc) {
    auto shared_proc = proc.lock();
    if(shared_proc) {
        output_file << *shared_proc << " placed on Ready Queue" << endl;
        ready_queue.push_back(proc);
    }
}

// ============================================================
// Function: wait_enqueue(weak_ptr<Process>)
//
// Enqueues a process to wait_queue if it is a valid weak_ptr.
// ============================================================
void Scheduler::wait_enqueue(const weak_ptr<Process> & proc) {
    auto shared_proc = proc.lock();
    if(shared_proc) {
        output_file << *shared_proc << " placed on Wait Queue" << endl;
        wait_queue.push_back(proc);
    }
}

void Scheduler::error_unrecognized_action(const string & action) {
    cerr << "[ERROR]: Unrecognized command: " << action << endl;
}

// ============================================================
// Function: split_on_space
// Returns:  vector<string>
//
// For reasons which I do not understand there is not a split
// function in the C++ standard library. This function is needed
// to tokenize the input actions for parsing.
// ============================================================
vector<string> split_on_space(const string & str) {
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

// ============================================================
// Function: is_number_st
// Returns:  bool
//
// Returns true if the string represents an integer.
// i.e. All of its chars are digits.
// ============================================================
bool is_number_str(const string & str) {
    for(auto &c : str) {
        if(!isdigit(c))
            return false;
    }
    return true;
}
