#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>
#include <QWidget>

class protooth{              //abstract class for all classes to be in threads
    friend class base_othr;
public:
    virtual ~protooth(){}                     //if its virtual, destroying its pointer will call the derived class destructor
    std::atomic<bool> end{false};             //set this to true to externally signal the thread it should close. In each derived object periodically chech if this is true, and if so, exit.
    std::atomic<bool> done{false};            //this flag indicates whether the thread is done      //TODO calling this from mainwindow causes a segfault
    virtual void run()=0;

    // connection GUI widgets
    std::vector<QWidget*> connectionGUI;
};

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
