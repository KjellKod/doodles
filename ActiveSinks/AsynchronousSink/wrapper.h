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
#include "unique_memory.h"
#include "g2future.h"
#include "sink.h"

namespace SinkWrapper {
  typedef std::function<void() > AsyncSinkCall;
  typedef std::string LogEntry;
  typedef std::function<void(LogEntry) > AsyncMessageCall;
  /// The wrapper represents any Sink<T> and can therefore be stored as a pointer
  /// in a standard container
  struct SinkWrapper {
    virtual ~SinkWrapper() {}
    virtual void send(LogEntry msg) = 0;
    virtual void send(AsyncSinkCall call) = 0; // honestly this one does not need to be abstract?
  };


  /// The Sink has an active object and owns the real object that will do the bg work
  template<class T>
  class Sink : public SinkWrapper {
    std::unique_ptr<Active> _bg;
    std::shared_ptr<T> _t; // behövs både _t och -sink-stoage???
    AsyncMessageCall _log_call; //the default call from worker->send(msg)
    //Callback _sink_storage_only; // not needed since we save T
  public:
    template<typename Call>
    Sink(std::shared_ptr<T> t, Call call)
    : SinkWrapper {},
            _t{t},
            _bg(Active::createActive()),    
            _log_call(std::bind(call, _t.get(), std::placeholders::_1)) /*_sink_storage_only([] {})*/ {
    }

    virtual ~Sink() {
      _bg.reset();
      std::cout << "Sink<T> exiting" << std::endl;
    }

    std::shared_ptr<T> ptr() { return _t.get(); }

    
    void send(LogEntry msg) override {  _bg->send([ = ]{_log_call(msg);});   }
    void send(AsyncSinkCall msg) { _bg->send(msg);  } // any type of call
  };

  //Sinkhandle is the client's access point to the specific sink instance
  // Only through the Sinkhandle can, and should, the sink specific API be called
  template<class T>
  class SinkHandle {
    std::weak_ptr<T> _handle;
  public:

    SinkHandle(std::shared_ptr<T> t) : _handle(t) { }
    ~SinkHandle() { }
  };
  
  

  typedef std::shared_ptr<SinkWrapper> SinkWrapperPtr;
  
  class Worker {
     std::vector<SinkWrapperPtr> _container; // should be hidden in a pimple with a bg active object
     std::unique_ptr<Active> _bg;
     
     void bgSave(LogEntry msg) {
       for(auto& sink: _container) {
         sink->send(msg);
       }
     }
     
  public:
    Worker() :_bg{Active::createActive()}{}
    ~Worker(){  _bg->send([this]{_container.clear();});  }
    void save(LogEntry msg) { _bg->send([this, msg]{bgSave(msg);});  } // will this be copied?
            //this is guaranteed to work std::bind(&Worker::bgSave, this, msg));   }
    
  
  template<typename T, typename DefaultLogCall>
  std::unique_ptr<SinkHandle<T>> addSink(std::unique_ptr<T> unique, DefaultLogCall call) {
    auto shared = std::shared_ptr<T>(unique.release());
    auto sink = std::make_shared<Sink<T>>(shared, call);
    auto add_sink_call = [this, sink]{_container.push_back(sink); };
    auto wait_result = g2::spawn_task(add_sink_call, _bg.get());
    wait_result.wait();
    
    auto handle = std2::make_unique<SinkHandle<T>>(shared);    
    return handle;
  }
  };
  
  
  /// The createHandle should reside nside the Worker and probably be called
  /// addSink instead. Here we get a Sink object and creates a handle for it that we store
  /// a weakptr to the Sink inside the SinkHandle object

//  template<typename TSink>
//  std::unique_ptr<SinkHandle<TSink >> createHandle(std::shared_ptr<TSink> ptr) {
//    auto weak = std::unique_ptr < SinkHandle < TSink >> (new SinkHandle<TSink>(ptr));
//    return weak;
//  }

//  typedef Sink<sink1> Sink1;
//  typedef Sink<sink2> Sink2;
//
//  void test2(int i, std::vector<SinkPtr>& container) {
//    {
//      for (auto& s : container)
//        s->send(std::to_string(i)); //if the destructor hits before 
//
//
//    }
//    std::cout << "added all jobs\n\n\n\n\n" << std::endl;
//
//    //    auto handle = createHandle(wrap1);
//    //    auto future_result = handle->async2(&sink1::addTextBeforePrint, "###");
//
//  }

  void test() {
    Worker worker;
    worker.save("Hello World!");
    auto handler_1 = worker.addSink(std2::make_unique<sink1>(), &sink1::print);
    worker.save("Hello again World!");
    
    
//      auto ptr1 = std::make_shared<sink1>(); // the real object wo will do the work
//      auto wrap1 = std::make_shared<Sink1>(ptr1, &sink1::print);
//
//      auto ptr2 = std::make_shared<sink2>();
//      auto wrap2 = std::make_shared<Sink2>(ptr2, &sink2::save);
//
//      container.push_back(wrap1);
//      container.push_back(wrap2); // Fake -- send() call at the g2logworker. Each sink in the queue gets a message
//      std::string msg = "XYZ";
//
//
//
//    std::vector<int> vec(300);
//    size_t count = 0;
//    for (auto i: vec)
//      test2(++count, container);
  }

} // sinkwrapper





//template <typename Func>
//std::future<typename std::result_of<Func()>::type> spawn_taskX(std::shared_ptr<T> sink, Func func)
//{
//  typedef typename std::result_of<Func()>::type result_type;
//  typedef std::packaged_task<result_type()> task_type;
//  task_type task(std::move(func));
//  std::future<result_type> result = task.get_future();
//  sink->send(PretendToBeCopyable<task_type>(std::move(task))); 
//  return std::move(result);
//}  



//    template<typename Call, typename... Args>
//    auto async2(Call call, Args... args)->decltype(_handle.lock()->async(call, args...)) {
//      std::shared_ptr<T> realptr(_handle);
//      return realptr->async(call, args...);
//    }
//    template<typename Call, typename... Args>
//    auto async2(Call call, Args... args)->decltype(async(bind(call, *(_handle.lock()), args...))) 
//    {
//      typedef decltype(async(bind(call, *(_handle.lock()), args...))) ReturnType;
//      
//      std::shared_ptr<T> realptr(_handle);
//      return spawn_taskX(realptr,   std::bind(call, realptr.get(), args...));

//      try
//      {
//        std::shared_ptr<T> realptr(_handle);
//        return spawn_taskX(realptr,   std::bind(call, realptr.get(), args...));
//      }
//      catch(const std::bad_weak_ptr& e) 
//      { // re-throw inside the expected futures
//        typedef std::promise<ReturnType> PromiseType;
//        auto promise = make_shared<PromiseType>();
//      }
//    }



//    template <typename Func>
//    std::future<typename std::result_of<Func()>::type> spawn_taskX(Func func) {
//      typedef typename std::result_of < Func()>::type result_type;
//      typedef std::packaged_task < result_type() > task_type;
//      task_type task(std::move(func));
//      std::future<result_type> result = task.get_future();
//      send(PretendToBeCopyable<task_type>(std::move(task)));
//      return std::move(result);
//    }
//
//
//    template<typename Call, typename... Args>
//    auto async(Call call, Args... args)->decltype(async(bind(call, ptr().get(), args...))) {
//      typedef decltype(async(bind(call, _t.get(), args...))) ReturnType;
//      return spawn_taskX(std::bind(call, ptr().get(), args...));
//    }





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

