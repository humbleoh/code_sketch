#include <atomic>
#include <cstddef>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>
#include <queue>

class thread_pool
{
public:
  ///
  explicit thread_pool(std::size_t capacity)
  {
    assert(capacity >= 1u);
    threads_.reserve(capacity);
    for (int i = 0; i < capacity; ++i) {
      threads_.emplace_back(std::bind_front(&thread_pool::scheduled_run, this));
      // alternative:
      //   threads_.emplace_back(std::bind(&thread_pool::scheduled_run, this, std::placeholders::_1))
      // alternative:
      //   threads_.emplace_back([this](std::stop_token s) { scheduled_run(s); });
    }
  }

  ///
  ~thread_pool()
  {
    stop();
    join();
  }

  ///
  void stop()
  {
    for (auto&& t: threads_) {
      t.request_stop();
    }
    condvar_.notify_all();
  }

  ///
  void join()
  {
    for (auto&& t: threads_) {
      if (t.joinable()) {
        t.join();
      }
    }
  }

  /// Wait all pending jobs to be completed.
  /**
   * 算是基本操作，等待队列中所有的工作执行完毕
   * @code
   * while (!tasks_.empty()) {} // 应该避免 busy-loop
   * join();
   * @endcode
   */
  void wait()
  {
  }
  
  /// Stop thread pool manager
  /**
   * 停止接收新工作
   */
  void terminate()
  {
  }

  /// Cancel all pending jobs
  /** */
  void cancel()
  {
  }

  ///
  template<typename Fn, typename... Args>
  auto submit(Fn&& f, Args... args)
  {
    using return_type = std::result_of_t<Fn(Args...)>;
    // 这里使用 std::bind 是因为没法直接使用闭包捕获来完美转发；
    // 另一个方法是通过 reference wrapper 来实现捕获的完美转发。
    // std::function 要求 Callable 对象是 CopyConstructible 的，
    // 但 std::packaged_task 并不满足 CopyConstructible 的要求，
    // 故通过 std::shared_ptr 来满足此的限制。
    auto ptask = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<Fn>(f), std::forward<Args>(args)...));
    auto future = ptask->get_future();
    auto lock = std::lock_guard {mutex_};
    tasks_.emplace([ptask]() { (*ptask)(); });
    // 先通知，后释放锁。目的是保证公平性和避免优先级倒置，因为
    // 互斥锁一般有较完善的阻塞线程调度算法，会按照线程优先级调
    // 度，相同优先级按照 FIFO 调度。
    // 理想的调度是 LIFO
    condvar_.notify_one();
    // 返回的 future 不能直接 get()，应当先 wait_for(timeout)。
    // 使用超时机制是因为工作是投递到队列中，该工作可能不会立刻执行。
    // 若在执行之前，线程池管理器终止所有线程，造成队列中的工作就不会
    // 执行返回结果，那么 get() 的后果就是无限等待。
    return future;
  }

private:
  ///
  void scheduled_run(std::stop_token stop)
  {
    // 有看到线程池实现把 stop_token 当作一个工作投递给线程。
    // 这样做有问题因为这不是有效的广播行为，投递n次无法保证
    // n个不同的线程都收到工作。
    while (!stop.stop_requested()) {
      auto lock = std::unique_lock {mutex_};
      condvar_.wait(lock, 
        [this, &stop]() { return !tasks_.empty() || stop.stop_requested(); });
      if (stop.stop_requested()) {
        break;
      }

      auto t = tasks_.front();
      tasks_.pop();
      lock.unlock();
      t();
    }
  }

private:
  // 还有另一种做法是封装线程，然后维护两个队列，一个是idle线程
  // 队列，另一个是busy线程队列。把队列中的工作直接投递到idle线程。
  // 需要benchmark一番，但内存开销肯定比较大。
  std::vector<std::jthread> threads_;
  std::queue<std::function<void()>> tasks_;
  // 这里有一个难点: 要如何解耦队列锁和条件变量?
  // 要基于什么一般抽象?
  std::mutex mutex_;
  std::condition_variable condvar_;
};

int main(int argc, char *argv[])
{
  thread_pool pool {10};
  std::atomic_int i {0};
  int j = 0;
  pool.submit([&i]() { for (int j = 0; j < 10000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  pool.submit([&i]() { for (int j = 0; j < 1000; ++j) ++i; });
  sleep(7); // Workaround for wait()
  // pool.wait();
  pool.stop();
  pool.join();
  std::cout << i << std::endl;
  return 0;
}
