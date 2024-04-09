#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>

using namespace std::chrono;

volatile int g_sum = 0;
std::mutex sum_lock;

void thread_worker(const int num_th)
{
	volatile int local_sum = 0;
	for (auto i = 0; i < 50000000 / num_th; ++i)
	{
		local_sum = local_sum + 2;
	}
	sum_lock.lock();
	g_sum = g_sum + local_sum;
	sum_lock.unlock();
}

int main()
{
	{
		volatile int sum = 0;

		auto start_t = high_resolution_clock::now();

		for (auto i = 0; i < 50000000; ++i) sum = sum + 2;

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		std::cout << "sum = " << sum << "  time = " << exec_ms << "ms." << std::endl;
	}

	{
		for (int num_threads = 1; num_threads <= 16; num_threads *= 2) {
			g_sum = 0;
			auto start_t = high_resolution_clock::now();
			std::vector<std::thread> threads;
			for (int i = 0; i < num_threads; ++i) 
				threads.emplace_back(thread_worker, num_threads);

			for (auto& th : threads)
				th.join();

			auto end_t = high_resolution_clock::now();
			auto exec_t = end_t - start_t;
			auto exec_ms = duration_cast<milliseconds>(exec_t).count();

			std::cout << num_threads << " Thread Sum = " << g_sum << "  time = " << exec_ms << "ms." << std::endl;
		}

		/*std::vector<std::thread> my_threads;
		for (int i = 0; i < 10; ++i) {
			my_threads.emplace_back(thread_worker, i);
		}
		for (auto& th : my_threads) {
			th.join();
		}*/
	}
}