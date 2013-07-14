/* 
 * File:   Active.h
 * Author: kjell
 *
 * Created on July 9, 2013, 9:41 PM
 */

#ifndef ACTIVE_H
#define	ACTIVE_H

#include <functional>
#include <future>
#include <thread>
#include <memory>
#include "shared_queue.h"

typedef std::function<void() > Callback;
using namespace std;


// A straightforward technique to move around packaged_tasks.
//  Instances of std::packaged_task are MoveConstructible and MoveAssignable, but
//  not CopyConstructible or CopyAssignable. To put them in a std container they need
//  to be wrapped and their internals "moved" when tried to be copied.
template<typename Moveable>
struct PretendToBeCopyable
{
  explicit PretendToBeCopyable(Moveable&& m)  : move_only_(std::move(m)) {}
  PretendToBeCopyable(PretendToBeCopyable& p)	: move_only_(std::move(p.move_only_)){} 
  PretendToBeCopyable(PretendToBeCopyable&& p) : move_only_(std::move(p.move_only_)){} // = default; // so far only on gcc
  void operator()() { move_only_(); } // execute
private:
  Moveable move_only_;
};



class Active {
   std::shared_ptr<shared_queue<Callback>> q;
   atomic<bool> go;
   std::thread thd;

   void internal_run() {
      Callback call;
      while (go) { 
         q->wait_and_pop(call);
         call();
      }
   }
   
public:

   Active(): q(make_shared<shared_queue<Callback>>()){go = true;}

   ~Active() {
      q->push([this]{go=false;});
      thd.join();
   }

   // Add asynchronously a work-message to queue

   void send(Callback msg_) {q->push(msg_);}
   shared_ptr<shared_queue<Callback>> message_queue(){return q;}

   static std::unique_ptr<Active> createActive() {
      std::unique_ptr<Active> ptr(new Active());
      ptr->thd = std::thread(&Active::internal_run, ptr.get());
      return ptr;
   }
};

#endif	/* ACTIVE_H */

