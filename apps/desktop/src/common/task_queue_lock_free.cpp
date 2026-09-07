#include "task_queue_lock_free.h"

#include <exception>
#include <new>
#include <stdexcept>
#include <utility>

namespace crossdesk {

TaskQueueLockFree::TaskQueueLockFree()
    : producer_(task_queue_), worker_([this] { WorkerThread(); }) {}

TaskQueueLockFree::~TaskQueueLockFree() { Stop(); }

std::future<void> TaskQueueLockFree::PostTask(std::function<void()> task) {
  TaskItem item;
  item.run = std::move(task);
  auto result = item.completion.get_future();
  {
    std::lock_guard lock(mutex_);
    if (stopping_) {
      throw std::runtime_error("Cannot post to a stopped task queue");
    }

    // Count before publishing: the worker may dequeue as soon as enqueue returns.
    ++pending_tasks_;
    if (!task_queue_.enqueue(producer_, std::move(item))) {
      --pending_tasks_;
      throw std::bad_alloc();
    }
  }
  wake_.notify_one();
  return result;
}

void TaskQueueLockFree::Stop() {
  // Concurrent Stop callers all wait for the same worker shutdown to finish.
  std::call_once(stop_once_, [this] {
    {
      std::lock_guard lock(mutex_);
      stopping_ = true;
    }
    wake_.notify_all();
    worker_.join();
  });
}

void TaskQueueLockFree::WorkerThread() {
  while (true) {
    TaskItem item;
    while (task_queue_.try_dequeue(item)) {
      --pending_tasks_;

      std::exception_ptr error;
      try {
        item.run();
      } catch (...) {
        error = std::current_exception();
      }

      // Release captured resources before marking the task complete.
      item.run = nullptr;
      if (error) {
        item.completion.set_exception(error);
      } else {
        item.completion.set_value();
      }
    }

    std::unique_lock lock(mutex_);
    wake_.wait(lock, [this] { return stopping_ || pending_tasks_ > 0; });
    // A stop request must not discard tasks submitted while the worker was idle.
    if (stopping_ && pending_tasks_ == 0) {
      return;
    }
  }
}

}  // namespace crossdesk
