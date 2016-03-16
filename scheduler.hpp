// File: scheduler.hpp
// --------------------------------------------------------
// Class: CS 470                      Instructor: Dr. Hwang
// Assignment: Process Scheduling     Date Assigned: 22 February 2016
// Programmer: Evan Higgins           Date Completed: 18 March 2016

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <deque>
#include <fstream>
#include "process.hpp"

#define TIME_QUANTUM 3

class Scheduler
{
public:
    Scheduler(std::string, std::string);

    void run();
private:


    std::ifstream input_file;
    std::ofstream output_file;

    //std::unique_ptr<std::ifstream> input_file;
    //std::unique_ptr<std::ofstream> output_file;

    //Using a shared_ptr for the current_process ensures that it
    //won't unexpectedly get destructed while it is running.
    std::shared_ptr<Process> current_process;

    std::shared_ptr<Process> idle_process;

    //Weak references are used to minimize list queue
    //searching during termination.
    std::deque< std::weak_ptr<Process> > ready_queue;
    std::deque< std::weak_ptr<Process> > wait_queue;

    void print_state();
    void parse_action(const std::string&);
    void attempt_halt();
    void update_current_process();
    std::shared_ptr<Process> get_next_process();

    void create_process(int, int);
    void wait_for_event(int);
    void signal_event(int);
    void destroy_by_pid(int);

    void cascading_terminate(Process&);
    void show_terminate_message(const Process&) ;

    void ready_enqueue(const std::weak_ptr<Process>&);
    void wait_enqueue(const std::weak_ptr<Process>&);

    void error_unrecognized_action(const std::string&);
};

std::vector<std::string> split_on_space(const std::string&);
bool is_number_str(const std::string&);

#endif //SCHEDULER_H
