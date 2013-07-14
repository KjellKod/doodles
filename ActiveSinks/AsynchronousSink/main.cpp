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

using namespace std;
namespace {

  void somePrint(string msg) {
    cout << msg << endl;
  }

  template<typename Args>
  void show(const Args& arg) {
    cout << arg << endl;
  }

  template<typename Head, typename... Args>
  void show(Head call, const Args&... args) {
    cout << call << endl;
    show(args...);
  }

  template<typename Call, typename... Args>
  void callAFunction(Call& call, const Args&... args) {
    function<void() > func = bind(call, args...);
    func();
  }

  template<typename Call, typename... Args>
  std::future<typename std::result_of<Call(Args...)>::type> TaskCall(Call call, Args&&... args) {
    typedef typename std::result_of < Call(Args...)>::type result_type;
    typedef std::packaged_task < result_type() > task_type;

    auto callback = bind(call, std::forward<Args>(args)...);
    auto f1 = async(callback); // this is put on a queue instead
    return f1;
  }

  auto print3 = [](string s1, string s2, string s3) ->void {
    cout << s1 << s2 << s3 << endl;
  };

  void call1() {
    show("K1-Hello", 1, 2, 3, string("World"));
    callAFunction(print3, "one", "two", "three");
  }

  void call2() {
    std::string str{"K2-future through task"};
    auto f3 = TaskCall([](string msg) {
      cout << msg << endl;
    }, str);
    f3.wait();
  }

  void call3() {
    auto res = TaskCall(print3, "K3-future: ", "I hope ", "this works!");
    res.wait();
  }

    template<typename TCheck, typename TFunction, typename... TArgs>
  struct async_sfinae_helper {
    typedef std::future<typename std::result_of < TFunction(TArgs...)>::type> type;
  };
  template<typename TClass, typename Call, typename... Args>
  auto async2(TClass* object, Call call, Args... args)-> decltype(async(bind(call, object, args...))) {
    auto callback = bind(call, object, args...);
    auto f1 = async(callback);
    return f1;
  }
  
  
} // a. namespace


namespace {
  struct sink {
    std::string pre;
    void addTextBeforePrint(std::string text) {  pre.append(text); }
    virtual void print(const std::string& text) { cout << pre << " " << text << endl; }
  };

  struct sink2 : public sink { 
    std::string post;
    void addTextAfterPrint(const std::string& text) {  post.append(text);  }
    virtual void print(const std::string& text) {cout << pre << " " << text << post << endl;}
    void operator()() {
      
    }
  };

  

  template<typename XSink>
  struct sink_handler {
    shared_ptr<XSink> sink;
    shared_ptr < shared_queue < Callback >> msgQ;

  public:
    sink_handler(shared_ptr<XSink> s, shared_ptr<shared_queue<Callback >> q) 
            : sink(s), msgQ(q) { }

    template<typename Call, typename... Args>
            void spawn_sink_task(Call call, Args&&... args) {
      typedef typename std::result_of < Call(Args...)>::type result_type;
      typedef std::packaged_task < result_type() > task_type;
      auto callback = bind(call, sink.get(), args...);
      auto f1 = async(callback);
      f1.wait();
    }

    template<typename Call, typename... Args> 
    auto sink_task(Call call, Args... args) -> decltype(async(bind(call, sink.get(), args...))) {    
      auto callback = bind(call, sink.get(), args...);     
      typedef decltype(callback) result_type;     
      typedef std::packaged_task < result_type() > task_type;     
      task_type task(std::move(callback));     
      std::future<result_type> result = task.get_future();     
      msgQ->push(PretendToBeCopyable<task_type>(std::move(task)));
      return result;  
    }
    
    
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
//    if(nullptr == sink.get()) {
//      return async([]{throw std::runtime_error("sink pointer not valid, deleted?");});
//    }
    return spawn_taskX(std::bind(call, sink.get(), args...));
  }
  };


  typedef unique_ptr<Active> activeptr;
  typedef shared_ptr<sink> sinkptr;
  typedef shared_ptr<sink_handler<sink >> sink_handler_ptr;

  struct ManySinks {
    map<sinkptr, activeptr> _sinks; // this should be in the workerpimpl which also has an activeptr

    sink_handler_ptr addSink(unique_ptr<sink> s) {
      auto x = s.release();
      sinkptr xptr;
      xptr.reset(x);
      _sinks[xptr] = Active::createActive();
      auto& a = _sinks[xptr];
      sink_handler_ptr handler = make_shared < sink_handler < sink >> (xptr, a->message_queue());
      return handler;
    }

    void print(std::string text) {
      for (auto& pair : _sinks) {
        auto& ptr = pair.first;
        auto& active = pair.second;
        active->send(bind(&sink::print, ptr.get(), text));
      }
    }
  };

} // namespace

int main() {
  call1();
  call2();
  call3();

  ManySinks sinks;
  auto s1_handler = sinks.addSink(unique_ptr<sink>(new sink));
  //auto done1 = async2(s1_handler->sink.get(), &sink::addTextBeforePrint, string("---"));
  auto done1 = s1_handler->async2(&sink::addTextBeforePrint, string("---"));
  auto done2 = s1_handler->async2(&sink::addTextBeforePrint, string("***"));
  done1.wait();
  done2.wait();
  
  sinks.print("Hello");



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
  return 0;
}

