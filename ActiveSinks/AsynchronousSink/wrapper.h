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
#include <atomic>
#include <chrono>
#include <thread>
#include <list>
#include <cassert>
#include "unique_memory.h"
#include "g2future.h"
#include "sink.h"


namespace SinkWrapper {
  typedef std::string LogEntry;
  typedef std::function<void(LogEntry) > AsyncMessageCall;
  /// The wrapper represents any Sink<T> and can therefore be stored as a pointer
  /// in a standard container

  struct SinkWrapper {
    virtual ~SinkWrapper() {}
    virtual void send(LogEntry msg) = 0;
  };


  /// The Sink has an active object and owns the real object that will do the bg work

  template<class T>
  struct Sink : public SinkWrapper {
    std::shared_ptr<T> _t; // behövs både _t och -sink-stoage???
    std::unique_ptr<Active> _bg;
    AsyncMessageCall _log_call; //the default call from worker->send(msg)

    template<typename Call >
            Sink(std::shared_ptr<T> t, Call call)
    : SinkWrapper {}, _t{t}
,    _bg(Active::createActive()),
    _log_call(std::bind(call, _t.get(), std::placeholders::_1)) /*_sink_storage_only([] {})*/ {
    }

    virtual ~Sink() {
      _bg.reset();
    }

    void send(LogEntry msg) override {
      _bg->send([this, msg]{_log_call(msg);});
    } // normal log messages
    //void send(AsyncSinkCall call) { _bg->send(call);  } //   When is this used???

    //auto sendXX(Call call, Args... args)->decltype(async(bind(call, _t.get(), args...))){
    template<typename Call, typename... Args>
            auto send(Call call, Args... args)-> std::future < decltype(bind(call, _t.get(), args...)()) > {
              return g2::spawn_task(std::bind(call, _t.get(), args...), _bg.get());
            }
  };


  //Sinkhandle is the client's access point to the specific sink instance
  // Only through the Sinkhandle can, and should, the sink specific API be called

  template<class T>
  class SinkHandle {
    std::weak_ptr<Sink<T >> _sink;

  public:

    SinkHandle(std::shared_ptr<Sink<T >> sink) : _sink(sink) {
    }

    ~SinkHandle() {
    }

    template<typename Call, typename... Args>
    auto call(Call call, Args... args) -> decltype(_sink.lock()->send(call, args...)) {
      try {
        std::shared_ptr < Sink < T >> sink(_sink); // might throw if sink is empty
        return sink->send(call, args...);
      } catch (const std::bad_weak_ptr& e) {
        T* t;
        typedef decltype(std::bind(call, t, args...)()) PromiseType;
        std::promise<PromiseType> promise;
        promise.set_exception(std::make_exception_ptr(e));
        return std::move(promise.get_future()); // needed w/ move? 
      }
    }
  };

  typedef std::shared_ptr<std::atomic<bool>> AtomicBoolPtr;
  typedef std::shared_ptr<std::atomic<int>> AtomicIntPtr;
  struct ScopedSetTrue {
    AtomicBoolPtr  _flag;
    AtomicIntPtr _count;

    explicit ScopedSetTrue(AtomicBoolPtr flag, AtomicIntPtr count)
    : _flag(flag), _count(count) {
    }

    void ReceiveMsg(std::string message) {
      std::chrono::milliseconds wait{100};
      std::this_thread::sleep_for(wait);
      ++(*_count);
    }

    ~ScopedSetTrue() {
      (*_flag) = true;
    }
  };









  typedef std::shared_ptr<SinkWrapper> SinkWrapperPtr;

  class Worker {
    std::vector<SinkWrapperPtr> _container; // should be hidden in a pimple with a bg active object
    std::unique_ptr<Active> _bg;

    void bgSave(LogEntry msg) {
      for(int i = 0; i < 100;  ++i){ 
         for (auto& sink : _container) {
            sink->send(msg);
         }
      }
    }
    

  public:

    Worker() : _bg {
      Active::createActive()
    }
    {
    }

    ~Worker() {
      _bg->send([this] {
        _container.clear();
      });
    }

    void save(LogEntry msg) {
      _bg->send([this, msg] {  bgSave(msg); });
    } // will this be copied?
    //this is guaranteed to work std::bind(&Worker::bgSave, this, msg));   }

    template<typename T, typename DefaultLogCall>
    std::unique_ptr<SinkHandle<T >> addSink(std::unique_ptr<T> unique, DefaultLogCall call) {
      auto shared = std::shared_ptr<T>(unique.release());
      auto sink = std::make_shared < Sink < T >> (shared, call);
      auto add_sink_call = [this, sink] {
        _container.push_back(sink);
      };
      auto wait_result = g2::spawn_task(add_sink_call, _bg.get());
      wait_result.wait();

      auto handle = std2::make_unique< SinkHandle<T> >(sink);
      return handle;
    }
  };

  void test1() {
    auto worker = std::make_shared<Worker>();
    worker->save("Hello World!");
    auto handler_1 = worker->addSink(std2::make_unique<sink1>(), &sink1::print);

    worker->save("Hello again World!");
    auto handler_2 = worker->addSink(std2::make_unique<sink2>(), &sink2::save);
    worker->save("Hola Mundo, otra vez");



    // Now call the handlers and give them specific commands
    // These commands will be sent STRAIGHT to the Sink's asynchronous dispatcher
    // therefore they CAN come before a worker.save() message
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = handler_1->call(&sink1::addTextBeforePrint, "-------");


    auto result2 = handler_2->call(&sink2::addTextAfterPrint, "-------");
    result2.get();
    // should this block until finished? Or up to the sender?
    worker->save("Goodbye. See you later");



    worker.reset(); // this should block until all are finished
    std::cout << "triggering a bad_ptr exception on purpose: ";
    auto result3 = handler_2->call(&sink2::addTextAfterPrint, "-------");
    try {
      std::cout << result3.get() << std::endl;
    }    catch (std::exception& e) {
      std::cout << e.what() << std::endl;
    }

  }

  void test2_one_sink() {
    using namespace std;
    AtomicBoolPtr flag = make_shared<atomic<bool>>(false);
    AtomicIntPtr count = make_shared<atomic<int>>(0);
    {

      auto worker = std::make_shared<Worker>();
      auto handle = worker->addSink(std2::make_unique<ScopedSetTrue>(flag, count), &ScopedSetTrue::ReceiveMsg);
      assert(false == flag->load());
      assert(0 == count->load());
      worker->save("this message should trigger an atomic increment at the sink");
    }
    assert(flag->load() == true);
    assert(100 == count->load());
    cout << "test2_one_sink finished\n";
  }

typedef vector<AtomicBoolPtr> BoolList;
typedef vector<AtomicIntPtr> IntVector;
void test_3_OneHundredSinks(){
  BoolList flags;
  IntVector counts;
  
  size_t NumberOfItems = 100;
  for(size_t index = 0; index < NumberOfItems; ++index) {
    flags.push_back(make_shared<atomic<bool>>(false));
    counts.push_back(make_shared<atomic<int>>(0));
  }
  
  { auto worker = std::make_shared<Worker>();
  size_t index = 0;
    for(auto& flag: flags) { 
      auto& count = counts[index++];
      // ignore the handle
      worker->addSink(std2::make_unique<ScopedSetTrue>(flag, count), &ScopedSetTrue::ReceiveMsg);
    }
    worker->save("Hello to 100 receivers :)");
    worker->save("Hello to 100 receivers :)");
  }
  // at the curly brace above the ScopedLogger will go out of scope and all the 
  // 100 logging receivers will get their message to exit after all messages are
  // are processed
   size_t index = 0;
    for(auto& flag: flags) { 
      auto& count = counts[index++];
      assert(flag->load());
      assert(2 == count->load());
  }
  
  cout << "test one hundred sinks is finished finished\n";
} 
  
  
  void test() {
    test1();
    test2_one_sink();
    test_3_OneHundredSinks();
  }
} // SinkWrapper





#endif	/* WRAPPER_H */

