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
#include "DynamicAddSinksWithDifferentTypes.h"

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
  SinkWrapper::test();
  DynamicAddSinksWithDifferentTypes::test();

  std::cout << "all tests finished" << std::endl;
  return 0;
}

