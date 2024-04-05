#include <iostream>
#include <thread>
#include <vector>

void thread_worker(int th_id)
{
	std::cout << "Hello World From " 
		<< th_id << std::endl;
}

int main()
{
	std::vector<std::thread> my_threads;
	for (int i = 0; i < 10; ++i) {
		my_threads.emplace_back(thread_worker, i);
	}
	for (auto& th : my_threads) {
		th.join();
	}
}