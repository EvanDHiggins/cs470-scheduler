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

    std::unique_ptr<std::ifstream> input_file;
    std::unique_ptr<std::ofstream> output_file;

    std::shared_ptr<Process> current_process;
    std::shared_ptr<Process> idle_process;
    std::deque< std::weak_ptr<Process> > ready_queue;
    std::deque< std::weak_ptr<Process> > wait_queue;

    std::shared_ptr<Process> get_running_process() const;

    void print_state() const;
    void create_process(int, int);

    void error_unrecognized_action(std::string);
    void parse_action(std::string);

    void terminate(Process&);

    void ready_enqueue(std::weak_ptr<Process>);
    void wait_enqueue(std::weak_ptr<Process>);

    void show_terminate_message(Process&);

    void destroy_by_pid(int);

    void update_current_process();
    std::shared_ptr<Process> next_process();
};

std::vector<std::string> split_on_space(std::string);
bool is_number_str(std::string);

template<typename T>
void rotate_queue(std::deque<T> & queue) {
    if(queue.empty())
        return;
    std::rotate(queue.begin(), queue.begin()+1, queue.end());
}

#endif //SCHEDULER_H


