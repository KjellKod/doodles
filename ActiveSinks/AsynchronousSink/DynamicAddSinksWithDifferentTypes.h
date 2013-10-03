#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace DynamicAddSinksWithDifferentTypes {


using namespace std;

struct Message {
   string mess;
   int value;

   string toString() const {
      std::string ret(mess);
      return ret.append(" ").append(to_string(value));
   }
};

struct MsgCaller {

   void CallY(Message mess) {
      cout << "MsgCaller: " << mess.toString();
   }
};

struct ConstMsgCaller {

   void CallY(const Message& mess) {
      cout << "ConstMsgCaller: " << mess.toString();
   }
};

struct StringCaller {

   void CallX(string str) {
      cout << "StringCaller: " << str;
   }
};

struct SinkWrapper {

   virtual ~SinkWrapper() {
   }
   virtual void send(Message msg) = 0;
};

typedef std::function<void(Message) > AsyncMessageCall;

template<class T>
struct Sink : public SinkWrapper {
   std::shared_ptr<T> _real_sink;
   AsyncMessageCall _default_log_call;

   template<typename DefaultLogCall >
   Sink(T* sink, DefaultLogCall call) // option two constructors, one where the DefaultLog call takes a string input T*
   : SinkWrapper {
   },
   _real_sink{sink}
,
   _default_log_call(std::bind(call, _real_sink.get(), std::placeholders::_1)) {
   }

   virtual ~Sink() {
   }

   void send(Message msg) override {
      _default_log_call(msg);
   }
};

template<typename T>
struct SinkAdapter : public SinkWrapper {
   std::function<void(string) > _string_call;
   shared_ptr<T> _ptr;

   template<typename StringCall >
   SinkAdapter(T* p, StringCall call)
   : _string_call(std::bind(call, p, std::placeholders::_1))
   , _ptr(p) {
   }

   void send(Message msg) {
      _string_call(msg.toString());
   }
};

vector<std::shared_ptr<SinkWrapper >> gObjects;

struct Worker {

   template<typename T, typename DefaultLogCall>
   void addSink(T* real_sink, DefaultLogCall call) {
      auto ptr = std::make_shared < Sink < T >> (real_sink, call);
      cout << "\nvoid addSink(T* real_sink, DefaultLogCall call) {" << std::endl;
      gObjects.push_back(ptr);
   }

      /* 
    // this one works too but is not necessary the templated one fixes it
    // the trick is to have a wrapper object that forwards the call. 
    // would it be possible to have the wrapper object as a
    // std::function? with () as the object? and the std::function taking a lambda and the lambda keeps the object?
    // answer is of course YES,.. but the code won't be any clearer I think
   template<typename T>
     void addSink(T* real_sink,void (T::*Call)(Message)) {
       auto ptr = std::make_shared<Sink<T>>   (real_sink, Call);
       cout << "\nvoid addSink(T* real_sink,void (T::*Call)(Message))" << std::endl;
       gObjects.push_back(ptr);
     }
    */
   template<typename T>
   void addSink(T* real_sink, void (T::*Call)(string)) {
      auto ptr = std::make_shared < SinkAdapter < T >> (real_sink, Call);
      cout << "\nvoid addSink(T* real_sink, void (T::*Call)(string) )" << std::endl;
      gObjects.push_back(ptr);
   }

};
//void (T::*)()

void test() {
   Message msg{
      {"Hello"}, 123};
   cout << msg.toString();

   Worker worker;
   worker.addSink(new StringCaller, &StringCaller::CallX);
   worker.addSink(new MsgCaller, &MsgCaller::CallY);
   worker.addSink(new ConstMsgCaller, &ConstMsgCaller::CallY);

   for (auto& o : gObjects) {
      cout << "\n";
      o->send(msg);
   }
}
}

/*using namespace std;
struct Message {
  string mess;
  int value;
  
  Message(const Message&) = default;
  Message& operator=(const Message&) = default;
  
  string operator()() {
           return std::string{mess + " " + to_string(value)};	  
  }
  
    Message operator()() {
           return Message(*this);	  
  }
};


something like this
Registrer(&T::ReceiverFunc, T* obje){
        __ enable_if ReceiverFunc takes a Message
               std::function<void(Message)>  just pass the Message to the Object::Func
	
        __ enable_if ReceiverFunc takes a string
               std::function<void(Message)>  wrap the object::receiver in a function that 
                   takes the Message, converts it to a string and then passes it to Object::Func
	           
}

Maybe easiest is to have a helper struct? From which you can get the constructed "function"
which is either the raw function or the 
convert_function which passes to raw

http://stackoverflow.com/questions/14926482/const-and-non-const-template-specialization
http://stackoverflow.com/questions/2537229/how-can-i-write-a-function-template-for-all-types-with-a-particular-type-trait/2537330#2537330
http://stackoverflow.com/questions/14600201/why-to-avoid-stdenable-if-in-function-signatures/14623831#14623831
http://stackoverflow.com/questions/3076206/enable-if-and-conversion-operator
http://stackoverflow.com/questions/14011399/why-am-i-getting-the-error-operator-cannot-be-overloaded
http://stackoverflow.com/questions/10283610/how-to-change-a-template-method-based-on-whether-the-type-is-an-integral-or-floa

int main() {
   Message m{{"Hello World"}, 123};
   Message m2(m); // copy constructor
   Message m3 = m2; // copy constructor
   Message m4;
   m4 = m3; // assignment operator
   
   string s;
   s = m2(); 
  
        // your code goes here
        return 0;
} */