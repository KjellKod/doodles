/* 
 * File:   herb_test.h
 * Author: kjell
 *
 * Created on July 14, 2013, 9:30 PM
 */

#ifndef HERB_TEST_H
#define	HERB_TEST_H

#include <future>
#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>

namespace herb {
  typedef std::function<void() > Callback;

  /** Multiple producer, multiple consumer thread safe queue
   * Since 'return by reference' is used this queue won't throw */
  template<typename T>
  class shared_queue {
    mutable std::queue<T> queue_;
    mutable std::mutex m_;
    mutable std::condition_variable data_cond_;

    shared_queue& operator=(const shared_queue&) = delete;
    shared_queue(const shared_queue& other) = delete;

  public:

    shared_queue() {
    }

    void push(T item) const {
      std::lock_guard<std::mutex> lock(m_);
      queue_.push(item);
      data_cond_.notify_one();
    }

    /// \return immediately, with true if successful retrieval

    bool try_and_pop(T& popped_item) const {
      std::lock_guard<std::mutex> lock(m_);
      if (queue_.empty()) {
        return false;
      }
      popped_item = queue_.front();
      queue_.pop();
      return true;
    }

    /// Try to retrieve, if no items, wait till an item is available and try again

    void wait_and_pop(T& popped_item) const {
      std::unique_lock<std::mutex> lock(m_); // note: unique_lock is needed for std::condition_variable::wait
      while (queue_.empty()) { //                       The 'while' loop below is equal to
        data_cond_.wait(lock); //data_cond_.wait(lock, [](bool result){return !queue_.empty();});
      }
      popped_item = queue_.front();
      queue_.pop();
    }

    bool empty() const {
      std::lock_guard<std::mutex> lock(m_);
      return queue_.empty();
    }

    unsigned size() const {
      std::lock_guard<std::mutex> lock(m_);
      return queue_.size();
    }
  };


#include <thread>
#include <iostream>
#include <ostream>
#include <memory>

  template<typename Fut, typename F, typename T>
  void set_value(std::promise<Fut>& p, F& f, T&t) {
    p.set_value(f(t));
  }

  template<typename F, typename T>
  void set_value(std::promise<void>& p, F& f, T&t) {
    f(t);
    p.set_value();
  }

  template <class T> class concurrent {
    mutable T t;
    mutable shared_queue<Callback> q;
    bool done = false;
    std::thread thd;

    void run() const {
      Callback call;
      while (!done) {
        q.wait_and_pop(call);
        call();
      }
    }

  public:
    concurrent(T t_) 
    : t(t_), thd([ = ]{Callback call; while (!done) {q.wait_and_pop(call); call();}}) 
    {}

    ~concurrent() { q.push([ = ]{done = true;}); thd.join();
    }


    template<typename F>
    auto operator()(F f) const -> std::future<decltype(f(t))> {
      auto p = std::make_shared < std::promise < decltype(f(t)) >> ();
      auto future_result = p->get_future();
      q.push([ = ]{
        try {
          set_value(*p, f, t); }        catch (...) {
          p->set_exception(std::current_exception()); }
      });
      return future_result;
    }
  };

  void main2() {
    using namespace std;
    concurrent<ostream&> ccout{cout};
    string yalla = "yalla";
    auto call = [=](ostream& oss){ oss << 1234 << "1234" << yalla << endl; };
    auto res_0 = ccout(call);
    
    auto res_1 = ccout([ = ](ostream & oss){oss << 1234 << "1234" << yalla << endl;});
    res_1.wait();
    res_0.wait();
  }

} // namespace herb


#endif	/* HERB_TEST_H */

