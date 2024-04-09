#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace std::chrono;

int sum = 0;

void thread_worker(int th_id)
{
	
}

int main()
{
	volatile int sum = 0;

	auto start_t = high_resolution_clock::now();
	for (int i = 0; i < 50000000; ++i)
		sum = sum + 2;
	auto end_t = high_resolution_clock::now();

	auto exec_t = end_t - start_t;
	auto exec_ms = duration_cast<milliseconds>(exec_t);

	std::cout << "Sum = " << sum << "  duration = ";
		std::cout << duration_cast<milliseconds>(exec_t).count() << std::endl;

	/*std::vector<std::thread> my_threads;
	for (int i = 0; i < 10; ++i) {
		my_threads.emplace_back(thread_worker, i);
	}
	for (auto& th : my_threads) {
		th.join();
	}*/
}