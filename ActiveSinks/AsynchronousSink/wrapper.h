/* 
 * File:   wrapper.h
 * Author: kjell
 *
 * Created on July 14, 2013, 10:05 AM
 */

#ifndef WRAPPER_H
#define	WRAPPER_H

#include <future>
#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>
#include "shared_queue.h"
#include "Active.h"
#include <thread>
#include <iostream>
#include <ostream>
#include <memory>
#include "sink.h"

namespace SinkWrapper {
  
typedef std::function<void() > Callback;
typedef std::string LogMessage;
typedef std::function<void(LogMessage)> MessageCall;

struct SinkWrapper {
  virtual ~SinkWrapper(){}
  virtual void send(LogMessage msg) = 0;
};

template<class T>
class Sink : public SinkWrapper {
  std::unique_ptr<Active> _bg;
  std::shared_ptr<T> _t;
  MessageCall  _log_call;
  Callback     _sink_storage_only;
  
public:
  template<typename Call>
  Sink(std::shared_ptr<T> t, Call call) 
  : SinkWrapper{}, _t(t), 
          _bg(Active::createActive()), 
          _log_call(std::bind(call, _t.get(), std::placeholders::_1)),
          _sink_storage_only([]{}){}
  //obs sink måste äga shared_ptr. den måste sparas också
  //även om det bara sparas i en dummmy struct
  
  virtual ~Sink(){ _bg.reset(); std::cout << "exiting" << std::endl;}
  
  void send(LogMessage msg) override {
    _bg->send([=]{_log_call(msg);});
  }
};

template<class T>
class SinkHandle {
  std::weak_ptr<T> _handle;
  
//template <typename Func>
//std::future<typename std::result_of<Func()>::type> spawn_taskX(Func func)
//{
//  typedef typename std::result_of<Func()>::type result_type;
//  typedef std::packaged_task<result_type()> task_type;
//  task_type task(std::move(func));
//  std::future<result_type> result = task.get_future();
//  msgQ->push(PretendToBeCopyable<task_type>(std::move(task))); s
//  return std::move(result);
//}  
  
public:
  SinkHandle(std::shared_ptr<T> t) : _handle(t){}

//    template<typename Call, typename... Args>
//    auto async2(Call call, Args... args)->decltype(async(bind(call, sink.get(), args...))) {
//    return spawn_taskX(std::bind(call, sink.get(), args...));
//    }
  
  
};


template<typename T>
std::unique_ptr<SinkHandle<T>>  createHandle(std::shared_ptr<T> ptr) {
  auto weak = std::unique_ptr<SinkHandle<T>>(new SinkHandle<T>(ptr));
  return weak;
}



  
//      template<typename Call, typename... Args>
//    auto async2(Call call, Args... args)->decltype(async(bind(call, sink.get(), args...))) {
//    return spawn_taskX(std::bind(call, sink.get(), args...));
//    }
//  };
  typedef std::shared_ptr<SinkWrapper> SinkPtr;
  typedef Sink<sink1> Sink1;
  typedef Sink<sink2> Sink2;
  
void test2() {
  std::vector<SinkPtr> sinks;
  
  auto ptr1 = std::make_shared<sink1>();
  auto wrap1 = std::make_shared<Sink1>(ptr1, &sink1::print); 
 
  auto ptr2 = std::make_shared<sink2>();
  auto wrap2 = std::make_shared<Sink2>(ptr2, &sink2::save); 

  sinks.push_back(wrap1);
  sinks.push_back(wrap2);// Fake -- send() call at the g2logworker. Each sink in the queue gets a message
  std::string msg = "XYZ";
  for(auto& s: sinks)
    s->send(msg);
    
  
  std::weak_ptr<Sink1> weak_1 = wrap1;
    
  }

void test(){
  for(int i = 0; i < 100000; ++i) 
    test2();
}
  
  
  // SinkPtr_1 ptr1 = std::make_shared<Sink1>(Sink1{sink1{}});
    
  //auto ptr2 = std::make_shared<Sink<ink2>>(std::unique_ptr<sink2>(new sink2));
  //sinks.push_back(ptr1);


} // sinkwrapper





//template<typename Fut, typename F, typename T>
//void set_value(std::promise<Fut>& p, F& f, T&t) {
//  p.set_value(f(t));
//}
//
//template<typename F, typename T>
//void set_value(std::promise<void>& p, F& f, T&t) {
//  f(t);
//  p.set_value();
//}
//
//template <class T> class concurrent {
//  mutable T t;
//  mutable shared_queue<Callback> q;
//  bool done = false;
//  std::thread thd;
//
//  void run() const {
//    Callback call;
//    while (!done) {
//      q.wait_and_pop(call);
//      call();
//    }
//  }
//
//public:
//  concurrent(T t_)
//  : t(t_), thd([ = ]{Callback call; while (!done) {
//q.wait_and_pop(call); call();
//    }}) {
//  }
//
//  ~concurrent() {
//    q.push([ = ]{done = true;});
//    thd.join();
//  }
//
//  template<typename F>
//  void fireAndForget(F f) {
//    q.push([ = ]{f(t);});
//  }
//
//  template<typename F>
//  auto operator()(F f) const -> std::future<decltype(f(t))> {
//    auto p = std::make_shared < std::promise < decltype(f(t)) >> ();
//    auto future_result = p->get_future();
//    q.push([ = ]{
//      try {
//        set_value(*p, f, t); } catch (...) {
//        p->set_exception(std::current_exception()); }
//    });
//    return future_result;
//  }
//};
#endif	/* WRAPPER_H */

