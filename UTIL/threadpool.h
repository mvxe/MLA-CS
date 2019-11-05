#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>
#include <thread>


class threadPool
{
public:
    threadPool(int threadNum);
    ~threadPool();
    void doJob(std::function <void (void)> func);

private:
    void threadEntry(int i);

    std::mutex lock;
    std::queue <std::function <void(void)>> jobs;
    std::vector <std::thread> threads;
    std::condition_variable cv;
    bool shutdown{false};
};

#endif // THREADPOOL_H
