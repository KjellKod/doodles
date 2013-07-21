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
    //virtual void send(AsyncSinkCall call) = 0; // honestly this one does not need to be abstract?
  };


  /// The Sink has an active object and owns the real object that will do the bg work
  template<class T>
  struct Sink : public SinkWrapper {
    std::unique_ptr<Active> _bg;
    std::shared_ptr<T> _t; // behövs både _t och -sink-stoage???
    AsyncMessageCall _log_call; //the default call from worker->send(msg)
  
    
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
    
    void send(LogEntry msg) override {  _bg->send([ = ]{_log_call(msg);});   }  // normal log messages
    //void send(AsyncSinkCall call) { _bg->send(call);  } //   When is this used???
      
    //auto sendXX(Call call, Args... args)->decltype(async(bind(call, _t.get(), args...))){
    template<typename Call, typename... Args>
    auto send(Call call, Args... args)-> std::future<  decltype(  bind(call, _t.get(), args...)())   > {
      return g2::spawn_task(std::bind(call, _t.get(), args...), _bg.get());
      } 
  };

  
  //Sinkhandle is the client's access point to the specific sink instance
  // Only through the Sinkhandle can, and should, the sink specific API be called
  template<class T>
  class SinkHandle {
    std::weak_ptr<T> _handle;
    std::weak_ptr<Sink<T>> _sink;
    
  public:
    SinkHandle(std::shared_ptr<Sink<T>> sink) : _sink(sink) { }
    ~SinkHandle() { }
    
    template<typename Call, typename... Args>
    auto call(Call call, Args... args) -> decltype(_sink.lock()->send(call, args...)) {
      try
      {
        std::shared_ptr < Sink < T >> sink(_sink); // might throw if sink is empty
        return sink->send(call, args...);
      }      
      catch (const std::bad_weak_ptr& e) 
      {
        T* t;
        typedef decltype(std::bind(call, t, args...)()) PromiseType;
        std::promise<PromiseType> promise;
        promise.set_exception(std::make_exception_ptr(e));
        return std::move(promise.get_future()); // needed w/ move? 
      }
    }
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
    
    auto handle = std2::make_unique<SinkHandle<T>>(sink); 
    return handle;
  }
  };
  
  

  void test() {
    auto worker = std::make_shared<Worker>();
    worker->save("Hello World!");
    auto handler_1 = worker->addSink(std2::make_unique<sink1>(), &sink1::print);
    
    worker->save("Hello again World!");
    auto handler_2 = worker->addSink(std2::make_unique<sink2>(), &sink2::save);
    worker->save("Hola Mundo, otra vez");
    
    
//    //    auto future_result = handle->async2(&sink1::addTextBeforePrint, "###");


    // Now call the handlers and give them specific commands
    // These commands will be sent STRAIGHT to the Sink's asynchronous dispatcher
    // therefore they CAN come before a worker.save() message
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = handler_1->call(&sink1::addTextBeforePrint, "-------"); 
    // should this block until finished? Or up to the sender?
    worker->save("Goodbye. See you later");
    
    
    worker.reset(); // this should block until all are finished
    auto result2 = handler_2->call(&sink2::addTextAfterPrint, "-------"); 
    try { std::cout << result2.get() << std::endl; }
    catch(std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  }

} // sinkwrapper







//  sinkhandler call 
//template<typename Call, typename... Args>
//    void sendX(Call call, Args... args){
//      auto bg_call = std::bind(call, _t.get(), args...);
//      typedef decltype(bg_call()) ResultType;
//      
//      std::future<ResultType> result00 = g2::spawn_task(bg_call, _bg.get()); 
//      std::future<decltype(bg_call())> result0 = g2::spawn_task(bg_call, _bg.get());  
//      auto result = g2::spawn_task(bg_call, _bg.get());  
//      result.wait(); 
//    } // any type of call
    


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

