#include <algorithm>
#include "process.hpp"

using namespace std;

Process::Process(int PID, int burst, weak_ptr<Process> parent) {
    this->parent = parent;
    this->remaining_burst = burst;
    this->PID = PID;
    this->remaining_quantum = 0;
    destructor_callback = [](Process a){};
}

Process::Process(int PID, int burst, weak_ptr<Process> parent,
        function<void(Process&)> callback) : 
    Process(PID, burst, parent)
{
    this->destructor_callback = callback;
}

Process::~Process() {
    destructor_callback(*this);
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

void Process::remove_child(shared_ptr<Process> child) {
    children.erase(
            remove(children.begin(), children.end(), child), children.end());
}

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
