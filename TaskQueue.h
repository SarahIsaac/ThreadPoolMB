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
							if (!continue_on) break;
						}
						if (!continue_on || tasks.empty()) break;
						if (!tasks.empty())
						{
							func = tasks.front();
							task_count--;
							tasks.pop();
						}
					}
					if (!tasks.empty())func();
				}
			});
		}
	}

	void join()
	{
		for (int i = 0; i < pool.size(); i++)
		{
			if (pool[i].joinable())
			{
				stop();
				pool[i].join();
			}
		}
	}

	~TaskQueue()
	{

	}

	void stop()
	{
		std::lock_guard<std::mutex> l(m);
		continue_on = false;
		taskAdded.notify_all();
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
};
