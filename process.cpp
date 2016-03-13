#include <algorithm>
#include "process.hpp"

using namespace std;

Process::Process(int PID, int burst, weak_ptr<Process> parent) {
    this->parent = parent;
    this->remaining_burst = burst;
    this->PID = PID;
    this->remaining_quantum = 0;
}

Process::~Process() {
}

// ============================================================
// Function: for_each_child
//
// I didn't want to provide an accessor to this->children because
// the lifetime of processes are dependent on the shared_ptr held
// by their parents. So it opens up possibilities for a memory
// leak if these references are external to Process. So
// for_each_child gives me the ability to operate on the process
// graph without the possibility of a memory leak.
// ============================================================
void Process::for_each_child(function<void(Process&)> func) const {
    for(auto child : children) {
        func(*child);
    }
}

void Process::tick() {
    --remaining_burst;
    --remaining_quantum;
}

void Process::set_quantum(int quantum) {
    remaining_quantum = quantum;
}

void Process::add_child(shared_ptr<Process> child) {
    children.push_back(child);
}

void Process::remove_child(Process & child) {
    children.erase(
            remove_if(children.begin(), children.end(),
                        [&](shared_ptr<Process> p) {
                            return p->get_PID() == child.get_PID();
                        }),
            children.end());
}

//void Process::remove_child(shared_ptr<Process> child) {
    //children.erase(
            //remove_if(children.begin(), children.end(), child), children.end());
//}

void Process::print(ostream & out) const {
    out << "PID " << this->PID << " " << this->remaining_burst;
}

void IdleProcess::print(ostream & out) const {
    out << "PID " << IDLE_PID;
}

ostream& operator<<(ostream & out, const Process & proc) {
    proc.print(out);
    return out;
}
