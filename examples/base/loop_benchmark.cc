#include <thread>
#include <time.h>
#include "base/time/time_utils.h"
#include "base/message_loop/message_loop.h"

std::atomic_int g_flag;
std::atomic_int64_t g_post_counter;
std::atomic_int64_t thread_end;

int64_t start_at;

base::MessageLoop g_loop;

const static  int max_task_per_producer = 10000000;
static int64_t finished_task_counter = 0;
int system_concurrency = std::thread::hardware_concurrency();

void TaskProducer(int i) {

  while(g_flag == 0) { //when g_flag !=  0, start run
      sleep(0);
  }

  int64_t total = 0;
  for (int i = 0; i < max_task_per_producer; i++) {

    bool ok = g_loop.PostTask(NewClosure([&]() {
      finished_task_counter++;
      if (max_task_per_producer * system_concurrency == finished_task_counter) {
        int64_t end = base::time_ms();
        LOG(INFO) << " all task finished @" << end << " total spend:" << end - start_at << "(ms)";
        g_loop.QuitLoop();
      }
    }));

    if (ok) {
      total++;
      g_post_counter++;
    }
  }
  thread_end++;
  LOG(INFO) << " thread end, success total " << total << " task send, g_post_counter:" << g_post_counter;
}

int main() {
  g_flag.store(0);
  thread_end.store(0);
  g_post_counter.store(0);
  system_concurrency = std::min(system_concurrency, 4);

  g_loop.SetLoopName("target");
  g_loop.Start();
  sleep(1);

  std::thread* threads[system_concurrency];
  for (int i = 0; i < system_concurrency; i++) {
    threads[i] = new std::thread(std::bind(&TaskProducer, i));
  }

  start_at = base::time_ms();
  LOG(INFO) << " all task start @" << start_at;
  g_flag.store(1);

  g_loop.WaitLoopEnd();

  for (auto& t : threads) {
    t->join();
  }
  for (auto t : threads) {
    delete t;
  }
  LOG(INFO) << "post " << g_post_counter << " success";
  return  0;
}
