#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
public:
	// 构造函数，创建指定数量的工作线程
	ThreadPool(size_t threads);
	// 析构函数，停止并销毁线程池
	~ThreadPool();

	// 提交任务到任务队列，并返回一个 std::future 以获取结果
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>;
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	int Size();
private:
	// 工作线程的集合
	std::vector<std::thread> workers;
	// 任务队列
	std::queue<std::function<void()>> tasks;

	// 同步机制
	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop;
};

inline int ThreadPool::Size() {
	return workers.size();
}

// 构造函数实现
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
	for (size_t i = 0; i < threads; ++i) {
		workers.emplace_back([this] {
			while (true) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					// 等待直到有任务或线程池停止
					this->condition.wait(lock, [this] {
						//只要满足“线程池停止” 或者 “任务队列非空”，就唤醒继续运行。
						return this->stop || !this->tasks.empty();
						});
					// 如果线程池停止且任务队列为空，则退出循环
					if (this->stop && this->tasks.empty()) {
						return;
					}
					// 从任务队列中取出一个任务
					//、、线程安全的取出的
						task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				// 执行任务
				task();
			}
			});
	}
}

// 任务提交接口实现            模板函数可以不用加inline
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type> {
	using return_type = typename std::result_of<F(Args...)>::type;
	//、、return_type 表示传进来的函数类型和参数类型指向的返回值的类型
		//、、make_shared<T>(args...)在 堆上分配 一个类型为 T 的对象，并用 args... 调用 T 的构造函数进行初始化；
		// 使用 std::packaged_task 包装任务，以便获取其返回值
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
	//、、定义一个共享指针，指向一个返回值类型是return_type的 函数对象
		std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		// 如果线程池已停止，则不允许提交新任务
		if (stop) {
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

		// 将任务添加到队列中
		tasks.emplace([task]() { (*task)(); });
	}
	// 唤醒一个等待的工作线程
	condition.notify_one();
	return res;
}

// 析构函数实现
inline ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	// 唤醒所有等待的线程，以便它们可以检查停止标志并退出
	condition.notify_all();
	// 等待所有工作线程执行完毕
	for (std::thread& worker : workers) {
		worker.join();
	}
}

//// 示例用法
//int main() {
//	// 创建一个包含4个工作线程的线程池
//	ThreadPool pool(4);
//
//	// 提交一些任务并存储它们的 future
//	std::vector<std::future<int>> results;
//	for (int i = 0; i < 8; ++i) {
//		results.emplace_back(pool.enqueue([i] {
//			std::cout << "hello " << i << std::endl;
//			std::this_thread::sleep_for(std::chrono::seconds(1));
//			std::cout << "world " << i << std::endl;
//			return i * i;
//			}));
//	}
//
//	// 获取并打印任务的结果
//	for (auto&& result : results) {
//		std::cout << "Result: " << result.get() << std::endl;
//	}
//
//	return 0;
//}