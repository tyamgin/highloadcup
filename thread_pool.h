#ifndef HIGHLOAD_THREAD_POOL_H
#define HIGHLOAD_THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>

using namespace std;

class ThreadPool
{
    typedef function<void()> Job;

    mutex _queue_mutex;
    condition_variable _condition;
    queue<Job> _queue;
    vector<thread*> _threads;
    int _num_threads;

    void _infinite_loop_function()
    {
        while (true)
        {
            Job job;
            {
                unique_lock<mutex> lock(_queue_mutex);
                _condition.wait(lock, [&] {return !_queue.empty(); });
                job = _queue.front();
                _queue.pop();
            }
            job();
        }
    };

public:

    ThreadPool(int num_threads)
    {
        this->_num_threads = num_threads;
    }

    void add_job(Job job)
    {
        {
            unique_lock<mutex> lock(_queue_mutex);
            _queue.push(job);
        }
        _condition.notify_one();
    }

    void start()
    {
        for (int i = 0; i < _num_threads; i++)
            _threads.push_back(new thread(&ThreadPool::_infinite_loop_function, this));
    }

    void join()
    {
        for (auto &thr : _threads)
            if (thr->joinable())
                thr->join();
    }
};

#endif //HIGHLOAD_THREAD_POOL_H
