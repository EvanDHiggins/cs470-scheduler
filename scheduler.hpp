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
    bool parse_action(std::string);

    void terminate(Process&);

    void ready_enqueue(std::weak_ptr<Process>);
    void wait_enqueue(std::weak_ptr<Process>);

    void show_terminate_message(Process&);

    void destroy_by_pid(int);

    void update_current_process();
    std::shared_ptr<Process> next_process();

    void wait_for_event(int);
    void signal_event(int);
};

std::vector<std::string> split_on_space(std::string);
bool is_number_str(std::string);

// ============================================================
// Function: drop_while
//
// Removes elements from the head of q until an element fails
// the predicate.
// ============================================================
template<typename T, typename UnaryPredicate>
void drop_while(std::deque<T> & q, UnaryPredicate pred) {
    for(auto i = q.begin(); i != q.end() && pred(*i);) {
        i = q.erase(i);
    }
}

// ============================================================
// Function: delete_dead_refs
//
// Drops items from the front of q until it finds a valid
// reference.
// ============================================================
template<typename T>
void drop_dead_refs(std::deque< std::weak_ptr<T> > & q) {
    drop_while(q,
            [](std::weak_ptr<T> & p) {
                auto shared_p = p.lock();
                return !bool(shared_p);
            });
}

#endif //SCHEDULER_H


