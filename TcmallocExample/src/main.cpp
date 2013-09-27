

#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include "Allocate.h"
#include <gperftools/malloc_extension.h>

void mainX() {  
//http://gperftools.googlecode.com/svn/trunk/src/tests/frag_unittest.cc
  
  size_t heap_before;
  MallocExtension::instance()->GetNumericProperty("generic.heap_size", &heap_before);
  std::cout << "heap before: " << heap_before/(1024*1024) << " MB" << std::endl; 
  

  auto str =  Allocate::Create(1000*1024*1024); // 1GB
  size_t heap_after;  
  MallocExtension::instance()->GetNumericProperty("generic.heap_size", &heap_after);
  std::cout << "heap after: " << heap_after/(1024*1024) << " MB" << std::endl; 

  std::vector<char> buffer(2000, '0');
  MallocExtension::instance()->GetStats(&buffer[0], 2000);
  std::cout << &buffer[0] << std::endl;
}


int main() {
   std::thread t1(mainX);
   t1.join();
   return 0;
}
