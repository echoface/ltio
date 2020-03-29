#ifndef _COMPONENT_ACCUMULATE_QUEUE_H
#define _COMPONENT_ACCUMULATE_QUEUE_H

#include <vector>
#include <thread>
#include <inttypes.h>
#include <functional>
#include <unordered_map>
#include <base/memory/spin_lock.h>

namespace component {

/**
 * usage: define your struct Data
 *
 * impliment operator+ for your Data struct
 *
 * component::AccumulatorCollector<Data> DataCollector;
 * regist a commit handler with:
 * DataCollector.SetHandler(callback);
 * DataCollector.Start();
 *
 * collect(sumup) data in you code:
 * DataCollector.Collect(key, value);
 *
 * when callback invoke, handle your data
 *
 * StopCollector manually or stop with desctructor
 * **/

template<class T>
class Accumulator {
  public:
    typedef std::unordered_map<std::string, T> AccumulationMap;
    Accumulator() {}

    void Acc(const std::string& key, const T& data) {

      base::SpinLockGuard guard(lock_);

      auto iter = datas_.find(key);
      if (iter == datas_.end()) {
        datas_[key] = data;
        return;
      }
      iter->second += data;
    }
    /*return the data count, and clear data*/
    uint32_t SwapOut(AccumulationMap& out) {
      lock_.lock();
      out = std::move(datas_);
      datas_.clear();
      lock_.unlock();
      return out.size();
    }
  private:
    base::SpinLock lock_;
    AccumulationMap datas_;
};

/* suguest count use your producer count, for defualt
 * it will use system thread::hardwareconcurrency()*/

template<class T>
class AccumulatorCollector {
public:
  typedef Accumulator<T> AccumulatorType;
  typedef std::unordered_map<std::string, T> AccumulationMap;
  typedef std::function<void(const AccumulationMap&)> CallBack;

  ~AccumulatorCollector() {

    if (thread_) {Stop();}

    for (auto p : accumulators_) {
      delete p;
    }
  }
  AccumulatorCollector(uint32_t submit_interval_ms)
    : submit_interval_us_(submit_interval_ms * 1000) {
    for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++) {
      accumulators_.push_back(new AccumulatorType());
    }
  }

  AccumulatorCollector(uint32_t submit_interval_ms, uint32_t count)
    : submit_interval_us_(submit_interval_ms * 1000) {
    uint32_t c = count > 0 ? count : std::thread::hardware_concurrency();
    for (uint32_t i = 0; i < c; i++) {
      accumulators_.push_back(new AccumulatorType());
    }
  }

  void SetHandler(CallBack handler) {
    handler_ = handler;
  }

  void Start() {
    running_ = true;
    thread_ = new std::thread(&AccumulatorCollector::ContentMain, this);
  }

  void Stop() {
    running_ = false;
    if (thread_ && thread_->joinable()) {
      thread_->join();
    }
    delete thread_;
    thread_ = nullptr;
  }

  void Collect(const std::string& key, const T& data) {
    AccumulatorType* accumulator = hash_accumulator();
    accumulator->Acc(key, data);
  }
private:
  void ContentMain() {

    while(running_) {
      usleep(submit_interval_us_);
      AccumulationMap all_data;

      for (auto& accumulator : accumulators_) {
        AccumulationMap part;
        accumulator->SwapOut(part);
        for (auto& pair : part) {
          all_data[pair.first] += pair.second;
        }
      }
      handler_(all_data);
    }
  }

  AccumulatorType* hash_accumulator() {
    uint32_t n = hasher_(std::this_thread::get_id()) % accumulators_.size();
    return accumulators_[n];
  }

  bool running_;
  CallBack handler_;
  std::thread* thread_;
  uint32_t submit_interval_us_; //in second
  std::hash<std::thread::id> hasher_;
  std::vector<AccumulatorType*> accumulators_;
};

}
#endif
