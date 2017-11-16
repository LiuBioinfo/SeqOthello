/*
 * This file is modified from https://github.com/progschj/ThreadPool
 * -----------------------------------------------------------------
 *Copyright (c) 2012 Jakob Progsch, Václav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
-------------------------------------
*/
    

#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t, size_t);
    template<class F, class... Args>
    auto enqueue(int x, F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
    using PIF = std::pair<int, std::function<void()> >;
    class Compare {
        public: bool operator()(PIF& a , PIF& b) {
                    return a.first > b.first;
        }
    };
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue

    std::priority_queue< 
        PIF,
        std::vector<PIF>,
        Compare
        > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition, condition_resource;
    int resource;
    bool stop;
};
 
