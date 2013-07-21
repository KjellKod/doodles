/* 
 * File:   sink.h
 * Author: kjell
 *
 * Created on July 15, 2013, 8:38 PM
 */

#ifndef SINK_H
#define	SINK_H

#include <string>
#include <thread>
#include <iostream>
#include <sstream>

  struct sink1 {
    std::string pre;
    void addTextBeforePrint(std::string text) {  pre.append(text); }
    
    void print(const std::string& text) { 
      std::stringstream ss;
      ss << pre << " " << text << " : " << std::this_thread::get_id() << std::endl;
      std::cout << ss.str();
    }
  }; 

  struct sink2  { 
    std::string post;
    std::string addTextAfterPrint(const std::string& text) {  post.append(text);  return post;}
    void save(const std::string& text) {
      std::stringstream ss;
      ss << text << " " << post << " : " << std::this_thread::get_id() << std::endl;
      std::cout << ss.str();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  };

#endif	/* SINK_H */

