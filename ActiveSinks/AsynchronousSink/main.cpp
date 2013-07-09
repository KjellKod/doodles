/* 
 * File:   main.cpp
 * Author: kjell
 *
 * Created on July 4, 2013, 12:00 PM
 */

#include <iostream>
#include <functional>
#include <future>
#include <thread>
#include <memory>
#include <vector>
#include <map>  // change this to unordered_map
#include "shared_queue.h"

using namespace std;
typedef std::function<void() > Callback;

class Active {
   shared_queue<Callback> q;
   atomic<bool> go;
   std::thread thd;

   void flagToExit() {
      go = false;
   }

   void internal_run() {
      while (go) {
         Callback call;
         q.wait_and_pop(call);
         call();
      }
   }
public:

   Active() {
      go = true;
   }

   ~Active() {
      function<void() > quit_token = std::bind(&Active::flagToExit, this);
      q.push(quit_token);
      thd.join();
   }

   // Add asynchronously a work-message to queue

   void send(Callback msg_) {
      q.push(msg_);
   }

   static std::unique_ptr<Active> createActive() {
      std::unique_ptr<Active> ptr(new Active());
      ptr->thd = std::thread(&Active::internal_run, ptr.get());
      return ptr;
   }
};



namespace {

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


   auto print3 = [](string s1, string s2, string s3) ->void {
      cout << s1 << s2 << s3 << endl;
   };

   void call1() {
      show("Hello", 1, 2, 3, string("World"));
      cout << "NEXXT\n\n\n" << endl;

      callAFunction(print3, "one", "two", "three");
   }

   template<typename Call, typename... Args>
   std::future<typename std::result_of<Call(Args...)>::type> TaskCall(Call call, Args... args)
   //-> decltype(std::async(std::function<typename std::result_of<Call(Args...)>::type>))
   {
      typedef typename std::result_of < Call(Args...)>::type result_type;
      typedef std::packaged_task < result_type() > task_type;

      auto callback = bind(call, args...);
      auto f1 = async(callback); // this is put on a queue instead
      return f1;
   }


} // a. namespace

void call2() {
   auto f1 = [](std::string arg) {
      cout << "callX Hello: " << arg << endl;
   };
   f1("K1");
   auto f2 = async(f1, "K2-future");
   f2.wait();

   std::string str{"K3-future through task"};
   auto f3 = TaskCall(f1, str);
   f3.wait();
}

void call3() {
   auto res = TaskCall(print3, "future: ", "I hope ", "this works!");
   res.wait();
}


namespace {

   struct sink {
   protected:
      std::string pre;
   public:

      void addTextBeforePrint(const std::string& text) {
         pre.append(text);
      }

      virtual void print(const std::string& text) {
         cout << pre << " " << text << endl;
      }
   };
   
   struct sink2 : public sink 
   {
      std::string post;
      void addTextAfterPrint(const std::string& text) { post.append(text); }
      virtual void print(const std::string& text) {cout << pre << " " << text << post << endl; }
   };
   
   

   template<typename Call, typename... Args>
   std::future<typename std::result_of<Call(Args...)>::type> AsyncQCall(Call call, Args... args) {
      typedef typename std::result_of < Call(Args...)>::type result_type;
      typedef std::packaged_task < result_type() > task_type;

      auto callback = bind(call, args...);
      auto f1 = async(callback); // this is put on a queue insteayd
      return f1;
   }


   typedef unique_ptr<Active> activeptr;
   typedef unique_ptr<sink> sinkptr;

   struct ManySinks {
      // vector< pair<sinkptr, activeptr>>  --- handler can be 
      //         shared_ptr-pair?? Overkill???
      map<sinkptr, activeptr> _sinks;

      void addSink(unique_ptr<sink> sink) {
         _sinks[std::move(sink)] = Active::createActive();
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
   sinks.addSink(unique_ptr<sink>(new sink));
   auto s2 = unique_ptr<sink2>(new sink2);
   s2->addTextBeforePrint("---");
   s2->addTextAfterPrint("....");
   sinks.addSink(move(s2));
   sinks.print("Hello");
   
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

