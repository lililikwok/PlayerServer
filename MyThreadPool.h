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
	// ���캯��������ָ�������Ĺ����߳�
	ThreadPool(size_t threads);
	// ����������ֹͣ�������̳߳�
	~ThreadPool();

	// �ύ����������У�������һ�� std::future �Ի�ȡ���
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>;
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	int Size();
private:
	// �����̵߳ļ���
	std::vector<std::thread> workers;
	// �������
	std::queue<std::function<void()>> tasks;

	// ͬ������
	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop;
};

inline int ThreadPool::Size() {
	return workers.size();
}

// ���캯��ʵ��
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
	for (size_t i = 0; i < threads; ++i) {
		workers.emplace_back([this] {
			while (true) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					// �ȴ�ֱ����������̳߳�ֹͣ
					this->condition.wait(lock, [this] {
						//ֻҪ���㡰�̳߳�ֹͣ�� ���� ��������зǿա����ͻ��Ѽ������С�
						return this->stop || !this->tasks.empty();
						});
					// ����̳߳�ֹͣ���������Ϊ�գ����˳�ѭ��
					if (this->stop && this->tasks.empty()) {
						return;
					}
					// �����������ȡ��һ������
					//�����̰߳�ȫ��ȡ����
						task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				// ִ������
				task();
			}
			});
	}
}

// �����ύ�ӿ�ʵ��            ģ�庯�����Բ��ü�inline
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type> {
	using return_type = typename std::result_of<F(Args...)>::type;
	//����return_type ��ʾ�������ĺ������ͺͲ�������ָ��ķ���ֵ������
		//����make_shared<T>(args...)�� ���Ϸ��� һ������Ϊ T �Ķ��󣬲��� args... ���� T �Ĺ��캯�����г�ʼ����
		// ʹ�� std::packaged_task ��װ�����Ա��ȡ�䷵��ֵ
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
	//��������һ������ָ�룬ָ��һ������ֵ������return_type�� ��������
		std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		// ����̳߳���ֹͣ���������ύ������
		if (stop) {
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

		// ��������ӵ�������
		tasks.emplace([task]() { (*task)(); });
	}
	// ����һ���ȴ��Ĺ����߳�
	condition.notify_one();
	return res;
}

// ��������ʵ��
inline ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	// �������еȴ����̣߳��Ա����ǿ��Լ��ֹͣ��־���˳�
	condition.notify_all();
	// �ȴ����й����߳�ִ�����
	for (std::thread& worker : workers) {
		worker.join();
	}
}

//// ʾ���÷�
//int main() {
//	// ����һ������4�������̵߳��̳߳�
//	ThreadPool pool(4);
//
//	// �ύһЩ���񲢴洢���ǵ� future
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
//	// ��ȡ����ӡ����Ľ��
//	for (auto&& result : results) {
//		std::cout << "Result: " << result.get() << std::endl;
//	}
//
//	return 0;
//}