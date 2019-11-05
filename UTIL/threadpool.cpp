#include "threadpool.h"

threadPool::threadPool(int threadNum){
    threads.reserve(threadNum);
    for(int i=0;i!=threadNum;i++)
        threads.emplace_back(std::bind(&threadPool::threadEntry,this,i));
}

threadPool::~threadPool(){
    {std::lock_guard <std::mutex>_lock(lock);
        shutdown = true;
        cv.notify_all();
    }
    for (auto& thread : threads)
        thread.join();
}

void threadPool::doJob(std::function <void (void)> func){
    std::lock_guard <std::mutex>_lock(lock);
    jobs.emplace(std::move(func));
    cv.notify_one();
}

void threadPool::threadEntry (int i){
    std::function <void (void)> job;
    for(;;){
        { std::unique_lock <std::mutex>_lock(lock);
            while(!shutdown && jobs.empty())
                cv.wait (_lock);

            if (jobs.empty())return;
            job = std::move (jobs.front ());
            jobs.pop();
        }
        job();
    }
}
