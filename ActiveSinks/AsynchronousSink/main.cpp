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

using namespace std;




namespace {
   void somePrint(string msg){ cout << msg << endl;}
   
   template<typename Args>
   void show(const Args& arg) {cout << arg << endl; }

   template<typename Head, typename... Args>
   void show(Head call, const Args&... args) { cout << call << endl;        show(args...); }

   template<typename Call, typename... Args>
   void callAFunction(Call& call, const Args&... args) { function<void() > func = bind(call, args...);          func();   }
   
   
   template<typename Call, typename... Args>
   std::future<typename std::result_of<Call(Args...)>::type> TaskCall(Call call, Args&&... args) {
      typedef typename std::result_of < Call(Args...)>::type result_type;
      typedef std::packaged_task < result_type() > task_type;
      
      auto callback = bind(call, std::forward<Args>(args)...);
      auto f1 = async(callback); // this is put on a queue instead
      return f1;
   }

   
   
   
   auto print3 = [](string s1, string s2, string s3) ->void {cout << s1 << s2 << s3 << endl; };
   void call1() {show("K1-Hello", 1, 2, 3, string("World")); callAFunction(print3, "one", "two", "three");}
   void call2() {std::string str{"K2-future through task"}; auto f3 = TaskCall([](string msg){cout << msg << endl;}, str); f3.wait();}
   void call3() {auto res = TaskCall(print3, "K3-future: ", "I hope ", "this works!");res.wait();}
   
} // a. namespace


namespace {

   struct sink {
      std::string pre;
      void addTextBeforePrint(std::string text) {pre.append(text);}
      virtual void print(const std::string& text) {cout << pre << " " << text << endl;}
   };
   
   struct sink2 : public sink {
      std::string post;
      void addTextAfterPrint(const std::string& text) { post.append(text); }
      virtual void print(const std::string& text) {cout << pre << " " << text << post << endl; }
   };
   
   


 
   
   
    template<typename TCheck, typename TFunction, typename... TArgs>
    struct async_sfinae_helper
    {
      typedef std::future<typename std::result_of<TFunction(TArgs...)>::type> type;
    };

     //typename async_sfinae_helper<typename std::decay<Call>::type, Call, Args...>::type
     template<typename TClass, typename Call, typename... Args>
     auto async2(TClass* object, Call call, Args... args)-> decltype(async(bind(call, object, args...)))
      {
         auto callback = bind(call, object, args...); 
         auto f1 = async(callback);
         //f1.wait();
         return f1;
      } 
     //std::future<typename std::result_of<Func()>::type> spawn_task(Func func, kjellkod::Active* worker)

     
           
     
//       template<typename Call, typename... Args>
//      //std::future<typename std::result_of<Call(Args...)>::type> 
//      auto sink_task(Call call, Args... args) -> decltype(bind(call, sink.get(), args...)) { // YES
//         decltype(bind(call, sink.get(), args...)) callback = bind(call, sink.get(), args...);    
//         auto f1 = async(callback);
//         //return std::move(f1);
//          typedef decltype((sink->addTextBeforePrint(args...))) yalla; // funkar
//          typedef typename std::result_of<decltype(callback)()>::type result_type0; // funckar
//      }
//           
           
     
   template<typename XSink>
   struct sink_handler{
   //private:
      shared_ptr<XSink> sink;
      shared_ptr<shared_queue<Callback>> msgQ;

   public:
            
      sink_handler(shared_ptr<XSink> s, shared_ptr<shared_queue<Callback>> q) : sink(s), msgQ(q){}
              
              
      template<typename Call, typename... Args>
      //std::future<typename std::result_of<Call(Args...)>::type> 
      void spawn_sink_task(Call call, Args&&... args) {
      typedef typename std::result_of < Call(Args...)>::type result_type;
      typedef std::packaged_task < result_type() > task_type;
    
      //auto callback = bind(call, sink.get(), args...);
      auto callback2 = bind(&somePrint, args...);
      auto f1 = async(callback2);
      f1.wait();
      //return std::move(f1);
//        auto callback = bind(call, sink.get(), args...);
//        task_type task(std::move(callback));
//      auto future_result = task.get_future();
//      msgQ->push(PretendToBeCopyable<task_type>(std::move(task)));
//      return future_result;
   }
      

              
                 
         //todo kan jag skriva om så at det framgår i argumenten att det är en funktionspekare?
         //se även: http://stackoverflow.com/questions/2689709/difference-between-stdresult-of-and-decltype?rq=1
          // fnkar inte: typedef typename std::result_of<Call(Args...)>::type yalla;
         //decltype(sink.get()->(typename call)(args...)) yalla;
         //typedef typename std::remove_pointer<Call>::type yalla2;
         //typedef typename function<typename std::remove_pointer<Call>::type>::result_type yalla4;
  //       typedef decltype(yalla2(args...)) yalla3; 
         //typedef typename result_of<decltype(&asd::f)(asd)>::type result_mem;
         //typedef typename std::result_of<decltype(callback)()>::type result_type1;
         //typedef typename function<sink:: typename Call>::result_type aType;
         //typedef typename std::result_of<typename Call(Args...)>::type result_type2;
         //typedef typename std::result_of<Call(Args...)>::type result_type2;
         //return spawn_sink_task(call, args...);        
         //return f1
              
      template<typename Call, typename... Args>
      //std::future<typename std::result_of<Call(Args...)>::type> 
      auto sink_task(Call call, Args... args) -> decltype(bind(call, sink.get(), args...)) { // YES
         decltype(bind(call, sink.get(), args...)) callback = bind(call, sink.get(), args...);    
         auto f1 = async(callback);
         //return std::move(f1);
          typedef decltype((sink->addTextBeforePrint(args...))) yalla; // funkar
          typedef typename std::result_of<decltype(callback)()>::type result_type0; // funckar
      }
   };
   
   typedef unique_ptr<Active> activeptr;
   typedef shared_ptr<sink> sinkptr;
   typedef shared_ptr<sink_handler<sink>>  sink_handler_ptr;
   
   
   struct ManySinks {
      map<sinkptr, activeptr> _sinks;
      
     sink_handler_ptr      addSink(unique_ptr<sink> s) {
         auto x = s.release(); sinkptr xptr; xptr.reset(x); 
         _sinks[xptr] = Active::createActive();  
         auto& a = _sinks[xptr];
         sink_handler_ptr handler = make_shared<sink_handler<sink>>(xptr, a->message_queue());
         return handler;
      }
      
      void print(std::string text) {
         for (auto& pair : _sinks) {
            auto& ptr = pair.first; auto& active = pair.second;
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
    //auto done1 = async2(&sink::addTextBeforePrint, string("---"));
   auto done1 = async2(s1_handler->sink.get(), &sink::addTextBeforePrint, string("---"));
   done1.wait();
   //auto done1 = s1_handler->sink_task(&sink::addTextBeforePrint, string("---"));
   //future<void> done1 = s1_handler->sink_task(&sink::addTextBeforePrint, string("---"));
   //s1_handler->spawn_sink_task(&sink::addTextBeforePrint, string("---"));
   sinks.print("Hello");
   
//     ManySinks sinks;
//   sinks.addSink(unique_ptr<sink>(new sink));
//   auto s2 = unique_ptr<sink2>(new sink2);
//   s2->addTextBeforePrint("---");
//   s2->addTextAfterPrint("....");
//   sinks.addSink(move(s2));
//   sinks.print("Hello");
   
   
   
   
   
   
  // What I want to do is this
  // Interface:
  // void save(msg)
  // handler<XXX> addSink(XXX) -- should only be OK t startup not afterwards
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

