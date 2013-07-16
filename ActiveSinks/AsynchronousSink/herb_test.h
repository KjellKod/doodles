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
#include "shared_queue.h"
#include <thread>
#include <iostream>
#include <ostream>
#include <memory>


namespace herb {
  typedef std::function<void() > Callback;

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

    ~concurrent() { q.push([ = ]{done = true;}); thd.join();    }

    template<typename F>
    void fireAndForget(F f) { q.push([=]{f(t);}); }
    
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

  
  concurrent<ostream&> ccout{cout};
  std::future<void> doCall(string str) {  
    auto call = [=](ostream& oss){ oss << 1234 << "1234" << str << endl; };
    return ccout(call);
  }
  
  void main2() {
    using namespace std;
    string yalla = "yalla";
    auto res_0 = doCall(yalla);
    auto res_1 = ccout([ = ](ostream & oss){oss << 1234 << "1234" << yalla << endl;});
    res_1.wait();
    res_0.wait();
  }

} // namespace herb


#endif	/* HERB_TEST_H */

