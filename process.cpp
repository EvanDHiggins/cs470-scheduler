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
    this->on_delete = [](Process& p){};
}

Process::Process(int PID,
                 int burst,
                 weak_ptr<Process> parent,
                 function<void(Process&)> on_delete) :
    Process(PID, burst, parent)
{
    this->on_delete = on_delete;
}

Process::~Process() {
    on_delete(*this);
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
                            return *p == child;
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
    if(*this == p)
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
// itself down the tree until it finds a node that matches the
// predicate.
// ============================================================
void Process::search_children_until(function<bool(Process&)> pred) const {
    for(auto &child : children) {
        //Stop recursing when the predicate is met
        if(pred(*child))
            return;
        child->search_children_until(pred);
    }
}

// ============================================================
// Function: terminate();
//
// Deletes the shared_ptr to this held by parent. This results
// in a cascading termination of all of this's child processes.
// ============================================================
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

// ============================================================
// Function: operator==(Process, Process)
// Returns:  bool
//
// Returns true if both processes have the same PID.
// ============================================================
bool operator==(const Process & lhs, const Process & rhs) {
    return lhs.get_PID() == rhs.get_PID();
}
