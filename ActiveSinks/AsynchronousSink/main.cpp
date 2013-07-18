/* 
 * File:   main.cpp
 * Author: kjell
 *
 * Created on July 4, 2013, 12:00 PM
 */

#include <iostream>
#include <functional>
#include <future>
#include <utility>
#include <thread>
#include <memory>
#include <type_traits>
#include <vector>
#include <map>  // change this to unordered_map
#include "Active.h"
#include "wrapper.h"
#include "assorted_async_testing.h"
#include "sink.h"
using namespace std;
using namespace test;

namespace {


  

  template<typename XSink>
  struct sink_handler {
    shared_ptr<XSink> sink;
    shared_ptr < shared_queue < Callback >> msgQ;

  public:
    sink_handler(shared_ptr<XSink> s, shared_ptr<shared_queue<Callback >> q) 
            : sink(s), msgQ(q) { }

    
 template <typename Func>
std::future<typename std::result_of<Func()>::type> spawn_taskX(Func func)
{
  typedef typename std::result_of<Func()>::type result_type;
  typedef std::packaged_task<result_type()> task_type;
  task_type task(std::move(func));
  std::future<result_type> result = task.get_future();
  msgQ->push(PretendToBeCopyable<task_type>(std::move(task))); 
  return std::move(result);
}  

    template<typename Call, typename... Args>
    auto async2(Call call, Args... args)->decltype(async(bind(call, sink.get(), args...))) {
    return spawn_taskX(std::bind(call, sink.get(), args...));
    }
  };


  typedef unique_ptr<Active> activeptr;
  typedef shared_ptr<sink1> sinkptr;
  typedef shared_ptr<sink_handler<sink1 >> sink_handler_ptr;

  
  
  struct Worker {
    map<sinkptr, activeptr> _sinks; // this should be in the workerpimpl which also has an activeptr

    sink_handler_ptr addSink(unique_ptr<sink1> s) {
      auto x = s.release();
      sinkptr xptr;
      xptr.reset(x);
      _sinks[xptr] = Active::createActive();
      auto& a = _sinks[xptr];
      sink_handler_ptr handler = make_shared < sink_handler < sink1 >> (xptr, a->message_queue());
      return handler;
    }

    void print(std::string text) {
      for (auto& pair : _sinks) {
        auto& ptr = pair.first;
        auto& active = pair.second;
        active->send(bind(&sink1::print, ptr.get(), text));
      }
    }
  };

} // namespace

#include "herb_test.h"
int main() {
//  call1();
//  call2();
//  call3();
//
//  Worker sinks;
//  auto s1_handler = sinks.addSink(unique_ptr<sink1>(new sink1));
//  //auto done1 = async2(s1_handler->sink.get(), &sink::addTextBeforePrint, string("---"));
//  auto done1 = s1_handler->async2(&sink1::addTextBeforePrint, string("---"));
//  auto done2 = s1_handler->async2(&sink1::addTextBeforePrint, string("***"));
//  done1.wait();
//  done2.wait();
//  
//  sinks.print("Hello");
//
//  
//  herb::main2();
  
  SinkWrapper::test();
  std::cout << "all tests finished" << std::endl;
  return 0;
}

  /*
  Next step. Take another main (keep this file)
  And make it cleaner. OR!!! Even better. Just fork
  G2log and put it in the right place. Important. It SHOULD be forked since
  I want to work with this for quite some time. I can merge it with g2log later
           
          Question: Is it OK that the sink_handler goes straigt to its 
          internal msgQ in front of any main msgQ enqueued messages?
              
             If we want it in truly fifo order how would we solve it then?
                I think it is FINE that it will happen asynchronously and not
          in FIFO order to the log messages.
   */
  // handler->addJob(&XXX::someJobToDo);
  //     0) addJob is similar to the  varags AsyncQCall(Call call, Args... args) above
  //     1) find the real sink, find its corresponding active
  //     2) active(bind(&XXX::Call, sinkptr.get(), args...));
  //     3) using make_shared and the technique below the handler should be pretty easy to do.
  //     struct handler {
  //        private: unique_ptr<pimple<XXX>>   xxx;
  //                 //shared_ptr<Active> active;
  //       future?/void doJobInSinkInFifoOrder(Call, Args...   args) 
  //                     pimple->send(bind(&XXX::Call, sinkptr.get(), args...));
  //
  // OBS: This is how ANY object can be stored. I.e the same technique I used before for signals-and-slots
  // but described very shortly and to the point!
  // http://stackoverflow.com/questions/12994416/generic-container-to-store-objects-of-a-particular-template-in-c





//    template<typename Call, typename... Args>
//            void spawn_sink_task(Call call, Args&&... args) {
//      typedef typename std::result_of < Call(Args...)>::type result_type;
//      typedef std::packaged_task < result_type() > task_type;
//      auto callback = bind(call, sink.get(), args...);
//      auto f1 = async(callback);
//      f1.wait();
//    }
//
//    template<typename Call, typename... Args> 
//    auto sink_task(Call call, Args... args) -> decltype(async(bind(call, sink.get(), args...))) {    
//      auto callback = bind(call, sink.get(), args...);     
//      typedef decltype(callback) result_type;     
//      typedef std::packaged_task < result_type() > task_type;     
//      task_type task(std::move(callback));     
//      std::future<result_type> result = task.get_future();     
//      msgQ->push(PretendToBeCopyable<task_type>(std::move(task)));
//      return result;  
//    }