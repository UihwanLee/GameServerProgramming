#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace std::chrono;

volatile int g_sum = 0;

void thread_worker(const int num_th)
{
	for (auto i = 0; i < 50000000 / num_th; ++i) 
		g_sum = g_sum + 2;
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
		for (int num_threads = 2; num_threads <= 16; num_threads *= 2) {
			auto start_t = high_resolution_clock::now();
			std::vector<std::thread> threads;
			for (int i = 0; i < num_threads; ++i) 
				threads.emplace_back(thread_worker, num_threads);

			for (auto& th : threads)
				th.join();

			auto end_t = high_resolution_clock::now();
			auto exec_t = end_t - start_t;
			auto exec_ms = duration_cast<milliseconds>(exec_t).count();

			std::cout << "2 Thread Sum = " << g_sum << "  time = " << exec_ms << "ms." << std::endl;
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