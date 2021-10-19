#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <sched.h>

namespace _567 {

    // 常规线程池
    template<typename T>
    class ThreadPool {
        using JT = std::function<void()>;

        std::vector<std::thread> threads;
        std::queue<JT> jobs;
        std::mutex mtx;
        std::condition_variable cond;
        bool stop = false;

        int runningJobs = 0;

    public:
        explicit ThreadPool(int const &numThreads = 4) {
            int i = 0;
            int cpus = sysconf(_SC_NPROCESSORS_ONLN);       // 获取当前设备cpu数量
            for ( ; i < numThreads; ++i) {
                int cpu = i % cpus;
                threads.emplace_back([this, cpu] {
                    cpu_set_t mask;
                    CPU_ZERO(&mask);                // 初始化set集，将set置为空
                    CPU_SET(cpu, &mask);
                    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1){
                        //std::cout << "Bind thread to cpu " << cpu << " failed." << std::endl;
                    }
                    else{
                        //std::cout << "Bind thread to cpu " << cpu << std::endl;
                    }
                    while (true) {
                        JT job;
                        {
                            std::unique_lock<std::mutex> lock(this->mtx);
                            this->cond.wait(lock, [this] {
                                return this->stop || !this->jobs.empty();
                            });
                            if (this->stop && this->jobs.empty()) return;
                            job = std::move(this->jobs.front());
                            this->jobs.pop();
                            runningJobs++;
                        }
                        job();
                        {
                            std::unique_lock<std::mutex> lock(this->mtx);
                            runningJobs--;
                        }
                    }
                });
            }
        }

        int Add(JT &&job) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::move(job));
            }
            cond.notify_one();
            return 0;
        }

        size_t JobsCount() const {
            return jobs.size() + runningJobs;
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(mtx);
                stop = true;
            }
            cond.notify_all();
            for (std::thread &worker : threads) {
                worker.join();
            }
        }
    };

    // 带执行环境的版本
    template<typename Env, size_t numThreads>
    struct ThreadPool2 {
        std::vector<Env> envs;
    protected:
        using JT = std::function<void(Env&)>;
        std::vector<std::thread> threads;
        std::queue<JT> jobs;
        std::mutex mtx;
        std::condition_variable cond;
        bool stop = false;

    public:
        explicit ThreadPool2() {
            envs.resize(numThreads);
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([this, i=i] {
                    auto& t = envs[i];
                    while (true) {
                        JT job;
                        {
                            std::unique_lock<std::mutex> lock(this->mtx);
                            this->cond.wait(lock, [this] {
                                return this->stop || !this->jobs.empty();
                            });
                            if (this->stop && this->jobs.empty()) return;
                            job = std::move(this->jobs.front());
                            this->jobs.pop();
                        }
                        t(job);
                    }
                    });
            }
        }

        int Add(JT&& job) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::move(job));
            }
            cond.notify_one();
            return 0;
        }

        size_t JobsCount(){
            return jobs.size();
        }

        ~ThreadPool2() {
            {
                std::unique_lock<std::mutex> lock(mtx);
                stop = true;
            }
            cond.notify_all();
            for (std::thread& worker : threads) {
                worker.join();
            }
        }
    };

    /*
    struct Env {
        // ...

        void operator()(std::function<void(Env&)>& job) {
            try {
                job(*this);
            }
            catch (int const& e) {
                xx::CoutN("catch throw int: ", e);
            }
            catch (std::exception const& e) {
                xx::CoutN("catch throw std::exception: ", e);
            }
            catch (...) {
                xx::CoutN("catch ...");
            }
        }
    };

    xx::ThreadPool2<Env, 4> tp;

    std::array<xx::ThreadPool2<Env, 1>, 4> tps;

    tp.Add([](Env& e) {
        // e.xxxxx
    });

    */
}
