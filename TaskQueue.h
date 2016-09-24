#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class TaskQueue
{
private:
	std::queue<std::function<void(void)>> tasks;
	std::mutex m;
	std::atomic<bool> continue_on;
	std::vector<std::thread> pool;
	std::condition_variable taskAdded;
	std::vector<std::thread> thread_pool;
	int thread_count;
	int task_count;

public:
	TaskQueue(int num_threads) : pool(), tasks()
	{
		thread_count = num_threads;
		continue_on = true;
		task_count = 512 * 512;

		for (int i = 0; i < num_threads; i++)
		{
			pool.emplace_back([&]() {
				while (continue_on)
				{
					std::function<void(void)> func;
					{
						std::unique_lock<std::mutex> lock(m);
						while (tasks.empty())
						{
							taskAdded.wait(lock);
						}
						func = tasks.front();
						task_count--;
						tasks.pop();
					}
					func();
				}
			});
		}
	}

	~TaskQueue()
	{
		stop();
		for (int i = 0; i < pool.size(); i++)
		{
			std::cout << ">>" << std::endl;
			pool[i].detach();
			//if (pool[i].joinable())
			//{
			//	std::cout << "joining" << std::endl;
			//	pool[i].join();
			//}
			std::cout << i << std::endl;
		}
		std::cout << "should be done here" << std::endl;
	}

	void stop()
	{
		std::lock_guard<std::mutex> l(m);
		continue_on = false;
	}

	void set_task_size(int t)
	{
		task_count = t;
	}

	void add_task(std::function<void()> task)
	{
		std::lock_guard<std::mutex> l(m);
		tasks.push(task);
		taskAdded.notify_one();
	}

	void clear()
	{
		while (!tasks.empty())
		{
			tasks.pop();
		}
	}
};