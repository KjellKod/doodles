/* 
 * File:   assorted_async_testing.h
 * Author: kjell
 *
 * Created on July 15, 2013, 8:09 PM
 */

#ifndef ASSORTED_ASYNC_TESTING_H
#define	ASSORTED_ASYNC_TESTING_H


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
namespace test {

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
#endif	/* ASSORTED_ASYNC_TESTING_H */

