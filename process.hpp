#ifndef PROCESS_H
#define PROCESS_H

#define IDLE_PID 0

#include <iostream>
#include <vector>

// ============================================================
//
// The model I settled on to represent parent/child relationships
// between processes leverages std::shared_ptr and std::weak_ptr.
// Each process maintains a vector of shared_ptrs to its children
// and each child maintains a weak_ptr to its parent. Any external
// references to processes should be kept as weak_ptrs. As long
// as this rule is followed all that is necessary to terminate a
// process and its children is to eliminate the shared_ptr the
// process's parent holds. By the semantics of shared_ptr the
// process will then be destructed, which calls this->children's
// destructor which in turn destructs all the shared_ptr to its
// children and so on. The termination cascades down the object
// graph terminating all child processes.
//
// I arrived at this model after realizing that it made the most
// sense to use heap-allocated processes. It
// is doable to maintain a vector of Process objects in the
// scheduler and keep a list of child pids within a process, but
// that requires some linear time (with respect to # of processes)
// pid_lookup(int) function. And if Processes are stored by value
// in a vector then processes can't maintain direct references
// to their children/parent.
//
// ============================================================

class Process
{
public:
    Process(int, int, std::weak_ptr<Process>);
    ~Process();

    int get_PID() const { return PID; }
    int get_remaining_quantum() const { return remaining_quantum; }

    std::weak_ptr<Process> get_parent() const { return parent; }

    //void remove_child(std::shared_ptr<Process>);
    void remove_child(Process &);

    virtual void tick();
    virtual bool is_idle() const { return false; }
    virtual bool burst_remaining() const { return remaining_burst > 0; }
    virtual bool quantum_remaining() const { return remaining_quantum > 0; }

    void set_quantum(int);

    void add_child(std::shared_ptr<Process>);

    void for_each_child(std::function<void(Process&)>) const;

    friend std::ostream& operator<<(std::ostream&, const Process&);
private:
    int PID;
    int remaining_burst;
    int remaining_quantum;

    std::weak_ptr<Process> parent;
    std::vector< std::shared_ptr<Process> > children;

    virtual void print(std::ostream &) const;
};

// ============================================================
//
// This is essentially the Null Object pattern. IdleProcess
// overrides methods to generally return some value meaning
// the process is not complete.
//
// ============================================================
class IdleProcess : public Process
{
public:
    IdleProcess() : Process(0, 0, std::weak_ptr<Process>()){}
    void tick() {}
    bool is_idle() const { return true; }
    bool burst_remaining() const { return true; }
    bool quantum_remaining() const { return true; }
private:
    void print(std::ostream &) const;
};

#endif //PROCESS_H
