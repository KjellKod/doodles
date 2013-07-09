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
#include "shared_queue.h"

using namespace std;
typedef std::function<void() > Callback;
class Active {
   shared_queue<Callback> q;
   atomic<bool> go;
   std::thread thd;
   void flagToExit() {  go = false; }
   void internal_run() {
      while (go) {
         Callback call;
         q.wait_and_pop(call);
         call;
      }
   }
public:
   Active() { go = true;}
   ~Active() {
      function<void() > quit_token = std::bind(&Active::flagToExit, this);
      q.push(quit_token);
      thd.join();
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


   //template<typename Call, typename... Args>
   // void callAFunction(Call& call, const Args&... args) {
   //    function<void() > func = bind(call, args...);
   //    func();
   // }



   //template<typename Call, typename... Args>
   //std::future<typename std::result_of<Call()>::type>  
   //       TaskCall(Call& call, Args&... args) {

   template<typename Call, typename... Args>
   std::future<void> TaskCall(Call call, Args... args) {

      typedef typename std::result_of < Call(Args...)>::type result_type;
      typedef std::packaged_task < result_type() > task_type;

      auto callback = bind(call, args...); 
      auto f1 = async(callback); // this is put on a queue instead
      return f1;
   }
} // a. namespace



struct sink {
   std::string pre;

   void addTextBeforePrint(const std::string& text) {
      pre.append(text);
   }

   void print(const std::string& text) {
      cout << pre << " " << text << endl;
   }
};




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

int main() {
   call1();
   call2();

   //callAFunction(print3, "one", "two", "three");

   auto res = TaskCall(print3, "future: ", "I hope ", "this works!");

   return 0;
}

