// File: process.cpp
// --------------------------------------------------------
// Class: CS 470                      Instructor: Dr. Hwang
// Assignment: Process Scheduling     Date Assigned: 22 February 2016
// Programmer: Evan Higgins           Date Completed: 18 March 2016

#include <algorithm>
#include "process.hpp"

using namespace std;

Process::Process(int PID, int burst, weak_ptr<Process> parent) {
    this->parent = parent;
    this->remaining_burst = burst;
    this->PID = PID;
    this->remaining_quantum = 0;
}

// ============================================================
// Function: tick()
//
// Advances the processes time step by one unit.
// ============================================================
void Process::tick() {
    --remaining_burst;
    --remaining_quantum;
}

// ============================================================
// Function: add_child(shared_ptr<Process>
//
// Adds a new shared_ptr reference to the processes list of
// children.
// ============================================================
void Process::add_child(shared_ptr<Process> child) {
    if(child)
        children.push_back(child);
}

// ============================================================
// Function: remove_child(Process&)
//
// Removes all children with the same PID as child.
// ============================================================
void Process::remove_child(Process & child) {
    children.erase(
            remove_if(children.begin(), children.end(),
                        [&](shared_ptr<Process> p) {
                            return p->get_PID() == child.get_PID();
                        }),
            children.end());
}

// ============================================================
// Function: wait_on(int)
//
// Causes process to wait on the passed event_id.
// ============================================================
void Process::wait_on(int event_id) {
    this->event_id = event_id;
    waiting_for_event = true;
}

// ============================================================
// Function: receive_event
// Returns:  bool
//
// Returns true if this is waiting on the provided event_id.
// ============================================================
bool Process::receive_event(int event_id) {
    if(waiting_for_event && this->event_id == event_id) {
        waiting_for_event = false;
        return true;
    }
    return false;
}

// ============================================================
// Function: owns(Process&)
// Returns:  bool
//
// A process P 'owns' process Q iff:
//  P has the same PID as Q
//  OR
//  One of P's children 'owns' Q. 
// ============================================================
bool Process::owns(Process & p) const {
    if(get_PID() == p.get_PID())
        return true;

    for(auto &child : children) {
        if(child->owns(p))
            return true;
    }

    return false;
}

bool Process::owns(int PID) const {
    if(get_PID() == PID)
        return true;

    for(auto &child : children) {
        if(child->owns(PID))
            return true;
    }

    return false;
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
//
// This applies func to each child of this and recursively calls
// itself down the tree.
// ============================================================
void Process::for_each_child(function<void(Process&)> func) const {
    for(auto &child : children) {
        func(*child);
        child->for_each_child(func);
    }
}

void Process::terminate() {
    auto shared_parent = parent.lock();
    if(shared_parent) {
        shared_parent->remove_child(*this);
    }
}

ostream& operator<<(ostream & out, const Process & proc) {
    proc.print(out);
    return out;
}

void Process::print(ostream & out) const {
    out << "PID " << this->PID << " " << this->remaining_burst;
}

void IdleProcess::print(ostream & out) const {
    out << "PID " << IDLE_PID;
}
