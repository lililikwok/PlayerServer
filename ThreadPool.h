//#ifndef THREADPOOL_H
//#define THREADPOOL_H
//
//#include <vector>
//#include <queue>
//#include <memory>
//#include <thread>
//#include <mutex>
//#include <future>
//#include <condition_variable>
//#include <atomic>
//#include <functional>
//#include <iostream>
//#include <unordered_map>
//
//const int TASK_MAX_THRESHHOLD = 1024;   // INT32_MAX;//���������ֵ
//const int THREAD_MAX_THRESHHOLD = 1024; // �߳���ֵ
//const int THREAD_MAX_IDLE_TIME = 60;    // ������ʱ��
//
//// �̳߳�֧�ֵ�ģʽ
//enum class PoolMode
//{
//	MODE_FIXED,  // �̶��߳��������̳߳�
//	MODE_CACHED, // �߳�������̬�����̳߳�
//};
//
//class Thread
//{
//public:
//	// �̺߳�����������
//	using ThreadFunc = std::function<void(int)>;
//
//	Thread(ThreadFunc func)
//		: func_(func),
//		threadId_(generateId_++)
//	{
//	}
//	~Thread() = default;
//
//	int getId() const
//	{
//		return threadId_;
//	}
//	// �����߳�
//	void start()
//	{
//		// ����һ���߳���ִ���̺߳���
//		std::thread t(func_, threadId_);
//		t.detach();
//	}
//
//private:
//	ThreadFunc func_;       // �̺߳���
//	inline static int generateId_; // �������߳�id
//	int threadId_;          // �����߳�id
//};
//
//// �̳߳�
//class ThreadPool
//{
//public:
//	ThreadPool()
//		: initThreadSize_(0),
//		curThreadSize_(0),
//		threadSizeThreadHold_(THREAD_MAX_THRESHHOLD),
//		idleThreadSize_(0),
//		taskSize_(0),
//		taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD),
//		poolMode_(PoolMode::MODE_FIXED),
//		isPoolRuning_(false)
//
//	{
//	}
//	~ThreadPool()
//	{
//		isPoolRuning_ = false;
//		// �ȴ��̳߳��������е��̷߳��� ������״̬������ & ����ִ��������
//		std::unique_lock<std::mutex> lock(taskQueMtx_);
//		notEmpty_.notify_all();
//		exitCond_.wait(lock, [&]() -> bool
//			{ return threads_.size() == 0; });
//	}
//
//	// ��ֹ��������͸�ֵ����
//	ThreadPool(const ThreadPool&) = delete;
//	ThreadPool& operator=(const ThreadPool&) = delete;
//
//	int Close()
//	{
//		isPoolRuning_ = false;
//		// �ȴ��̳߳��������е��̷߳��� ������״̬������ & ����ִ��������
//		std::unique_lock<std::mutex> lock(taskQueMtx_);
//		notEmpty_.notify_all();
//		exitCond_.wait(lock, [&]() -> bool
//			{ return threads_.size() == 0; });
//		return 0;
//	}
//
//	// �����̳߳ع���ģʽ
//	void setMode(PoolMode mode)
//	{
//		if (checkRuningState())
//			return; // �̳߳�������������������
//		poolMode_ = mode;
//	}
//
//	// �����������������ֵ
//	void setTaskQueMaxThreadHold(size_t threadhold)
//	{
//		taskQueMaxThreshHold_ = threadhold;
//	}
//
//	// �����̳߳�cachedģʽ���߳���ֵ
//	void setThreadSizeThreshHold(size_t threadhold)
//	{
//		if (checkRuningState())
//			return; // �̳߳�������������������
//		if (poolMode_ == PoolMode::MODE_CACHED)
//		{
//			threadSizeThreadHold_ = threadhold;
//		}
//	}
//
//	// ���̳߳��ύ����
//	// ʹ�ÿɱ��ģ���̣���submitTask���Խ������������������������Ĳ���
//	// Result submitTask(std::shared_ptr<Task> sp);
//	template <typename Func, typename... Args>
//	auto submitTask(Func&& func, Args &&...args) -> std::future<decltype(func(args...))>
//	{
//		// ������񣬷��������������
//		using Rtype = decltype(func(args...));
//		auto task = std::make_shared<std::packaged_task<Rtype()>>(
//			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
//		std::future<Rtype> result = task->get_future();
//
//		// ��ȡ��
//		std::unique_lock<std::mutex> lock(taskQueMtx_);
//		// �߳�ͨ�� �ȴ��ȴ���������п���
//		//  while(taskQue_.size() == taskQueMaxThreshHold_)
//		//  {
//		//      notFull_.wait(lock);
//		//  }
//		// �û��ύ�����������������1s�������ж������ύʧ��
//
//		bool flag = notFull_.wait_for(lock,
//			std::chrono::seconds(1),
//			[&]() -> bool
//			{ return taskQue_.size() < taskQueMaxThreshHold_; });
//		if (!flag)
//		{
//			// ��ʾnotFull_�ȴ�1s�ӣ�������Ȼû������
//			std::cerr << "task queue is full, submit task fail." << std::endl;
//			auto task = std::make_shared<std::packaged_task<Rtype()>>(
//				[]() -> Rtype
//				{ return Rtype(); });
//			(*task)();
//			return task->get_future();
//		}
//
//		// ����п��࣬��������з������������
//		// taskQue_.emplace(sp);
//		taskQue_.emplace([task]()
//			{ (*task)(); });
//		taskSize_++;
//
//		// ��Ϊ�·�������������п϶�������,notEmpty_�Ͻ���֪ͨ,�Ͽ�����߳�ִ������
//		notEmpty_.notify_all();
//
//		// cachedģʽ��������ȽϽ��� ������С��������� ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����µ��̳߳���
//		if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < (int)threadSizeThreadHold_)
//		{
//			// std::cout<<"<<<<<<<<<<<<<<<<<<<<<<<<���������߳�<<<<<<<<<<<<<<<<<<<<<<<<<<"<<std::endl;
//			// �������߳�
//			std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
//			// threads_.emplace_back(ptr->getId(),std::move(ptr));
//			int threadId = ptr->getId();
//			threads_.emplace(threadId, std::move(ptr));
//			// �����߳�
//			threads_[threadId]->start();
//			// �మ�̸߳�����صı���
//			curThreadSize_++;
//			idleThreadSize_++;
//		}
//
//		// ���������Result����
//		return result;
//	}
//
//	// �����̳߳�
//	void start(size_t initThreadSize = std::thread::hardware_concurrency()) // ��ʼ�߳�����
//	{
//		// �����̳߳ص�����״̬
//		isPoolRuning_ = true;
//		// ��ʼ�̸߳���
//		initThreadSize_ = initThreadSize;
//		curThreadSize_ = initThreadSize;
//
//		// �����̶߳���
//		for (int i = 0; i < (int)initThreadSize_; i++)
//		{
//
//			std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
//			// std::unique_ptr<Thread> ptr(new Thread(std::bind(&ThreadPool::threadFunc,this)));
//			// threads_.emplace_back(std::move(ptr));//unique_ptr��������ֵ���ÿ������죬����ֵ���ÿ�������-
//			threads_.emplace(ptr->getId(), std::move(ptr));
//		}
//
//		// ���������߳�
//		for (int i = 0; i < (int)initThreadSize_; i++)
//		{
//			threads_[i]->start(); // ��Ҫȥִ��һ���̺߳���
//			idleThreadSize_++;    // ��¼��ʼ�����߳�����
//		}
//	}
//
//private:
//	// �����̺߳���
//	void threadFunc(int threadid)
//	{
//		auto lastTime = std::chrono::high_resolution_clock().now();
//
//		for (;;)
//		{
//			Task task;
//			{ // ��֤ȡ�����������ͷ������ñ���߳�ȥȡ���񣬶����ǵȵ�����ִ�н������ͷ�
//				// �Ȼ�ȡ��
//				std::unique_lock<std::mutex> lock(taskQueMtx_);
//				// std::cout<<"�߳�"<<std::this_thread::get_id()<<" ���Ի�ȡ����"<<std::endl;
//				// cachedģʽ�£��п����Ѿ������˺ܶ���̣߳����ǿ���ʱ�䳬��60s,Ӧ�ðѶ�����߳̽������յ�
//				//(����initThreadSize�������߳�Ҫ���л���)
//				// ��ǰʱ�� - ��һ���߳�ִ�е�ʱ�� > 60s
//				// ÿһ���з���һ�� ��ô���֣���ʱ���أ���������ִ�з���
//				while (taskQue_.size() == 0)
//				{
//					if (!isPoolRuning_)
//					{
//						threads_.erase(threadid);
//						exitCond_.notify_all();
//						return; // �̺߳��������߳̽���
//					}
//					if (poolMode_ == PoolMode::MODE_CACHED)
//					{
//						// ������������ʱ������
//						if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
//						{
//							auto now = std::chrono::high_resolution_clock().now();
//							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
//							if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > (int)initThreadSize_) // ���ж��г�ʱ
//							{
//								// ���յ�ǰ�߳�
//								// ��¼�߳�������ر�����ֵ�޸�
//								// ���̶߳�����߳��б�������ɾ�� threadid => thread���� => ɾ��
//								threads_.erase(threadid);
//								curThreadSize_--;
//								idleThreadSize_--;
//								return;
//							}
//						}
//					}
//					else if (poolMode_ == PoolMode::MODE_FIXED)
//					{
//						// �ȴ�notEmpty����
//						notEmpty_.wait(lock);
//					}
//				}
//
//				idleThreadSize_--;
//
//				// �����������ȡһ���������
//				task = taskQue_.front();
//				taskQue_.pop();
//				taskSize_--;
//				// std::cout<<"�߳�"<<std::this_thread::get_id()<<" ��ȡ����ɹ�"<<std::endl;
//			}
//
//			// �����Ȼ��ʣ�����񣬼���֪ͨ�������߳�ִ������
//			if (taskQue_.size() > 0)
//			{
//				notEmpty_.notify_all();
//			}
//			// ȡ��һ�����񣬽���֪ͨ�����Լ����ύ��������
//			notFull_.notify_all();
//
//			// ��ǰ�̸߳���ִ���������
//			if (task != nullptr)
//			{
//				// task->exec();
//				task();
//			}
//
//			idleThreadSize_++;
//			lastTime = std::chrono::high_resolution_clock().now();
//		}
//	}
//
//	// ���pool������״̬
//	bool checkRuningState() const
//	{
//		return isPoolRuning_;
//	}
//
//public:
//	int Size()
//	{
//		return initThreadSize_;
//	}
//
//private:
//	// std::vector<std::unique_ptr<Thread>> threads_; //�߳��б�
//	std::unordered_map<int, std::unique_ptr<Thread>> threads_; // �߳��б�
//	std::size_t initThreadSize_;                               // ��ʼ���߳�����
//	std::atomic_int curThreadSize_;                            // ��¼��ǰ�̳߳������̵߳�������
//	std::size_t threadSizeThreadHold_;                         // �߳�������������ֵ
//	std::atomic_int idleThreadSize_;                           // ��¼�����̵߳�����
//
//	// Task���� =�� ��������
//	using Task = std::function<void()>;
//	std::queue<Task> taskQue_;    // �������
//	std::atomic_int taskSize_;    // ���������
//	size_t taskQueMaxThreshHold_; // �����������������ֵ
//
//	// �߳�ͨ��
//	std::mutex taskQueMtx_;            // ��֤��������̰߳�ȫ
//	std::condition_variable notFull_;  // ��ʾ������в���
//	std::condition_variable notEmpty_; // ��ʾ������в���
//	std::condition_variable exitCond_; // �ȴ��߳���Դȫ������
//
//	PoolMode poolMode_;             // ��ǰ�̳߳صĹ���ģʽ
//	std::atomic_bool isPoolRuning_; // ��ʾ��ǰ�̳߳ص�����״̬
//};
//
//#endif