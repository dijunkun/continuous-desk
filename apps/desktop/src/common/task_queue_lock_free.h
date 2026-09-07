/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-07
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _TASK_QUEUE_LOCK_FREE_H_
#define _TASK_QUEUE_LOCK_FREE_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <thread>

#include <concurrentqueue.h>

namespace crossdesk {

// A single worker executes tasks in submission order. The concurrent queue
// stores tasks; a mutex coordinates submissions, worker wakeups and shutdown.
class TaskQueueLockFree {
 public:
  TaskQueueLockFree();
  ~TaskQueueLockFree();

  // Returns completion or the task's exception. Throws if the queue is stopped.
  std::future<void> PostTask(std::function<void()> task);

  // Rejects new tasks, finishes accepted tasks and joins the worker.
  // May be called repeatedly, but never from the queue's own tasks.
  void Stop();

 private:
  struct TaskItem {
    std::function<void()> run;
    std::promise<void> completion;
  };

  void WorkerThread();

  moodycamel::ConcurrentQueue<TaskItem> task_queue_;
  // All submissions share one producer so different caller threads keep FIFO.
  moodycamel::ProducerToken producer_;
  std::atomic<std::size_t> pending_tasks_{0};
  std::mutex mutex_;
  std::condition_variable wake_;
  bool stopping_ = false;
  std::once_flag stop_once_;
  std::thread worker_;
};

}  // namespace crossdesk

#endif