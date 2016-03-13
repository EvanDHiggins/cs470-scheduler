#ifndef PROCESS_H
#define PROCESS_H

#define IDLE_PID 0

#include <iostream>
#include <vector>

class Process
{
public:
    Process(int, int, std::weak_ptr<Process>);
    Process(int, int, std::weak_ptr<Process>, std::function<void(Process&)>);
    ~Process();

    void set_termination_callback(std::function<void(Process&)> callback);

    int get_PID() const {return PID;}
    std::weak_ptr<Process> get_parent() const {return parent;}

    void remove_child(std::shared_ptr<Process>);

    virtual void tick();
    virtual bool is_idle() const {return false;}
    virtual bool no_burst_remaining() const {return remaining_burst == 0;}
    bool no_quantum_remaining() const {return remaining_quantum == 0;}

    void set_quantum(int);

    void add_child(std::shared_ptr<Process>);

    friend std::ostream& operator<<(std::ostream&, const Process&);
private:
    int PID;
    int remaining_burst;
    int remaining_quantum;
    std::function<void(Process&)> destructor_callback;

    std::weak_ptr<Process> parent;
    std::vector< std::shared_ptr<Process> > children;

    virtual void print(std::ostream &) const;
};

class IdleProcess : public Process
{
public:
    IdleProcess() : Process(0, 0, std::weak_ptr<Process>()){}
    void tick() {}
    bool is_idle() const {return true;}
    bool no_burst_remaining() const {return false;}
private:
    void print(std::ostream &) const;
};

#endif //PROCESS_H
