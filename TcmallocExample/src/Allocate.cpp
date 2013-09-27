
#include <string>
#include "Allocate.h"

namespace {
const std::string staticStr = Allocate::Create(500*1024*1024);
}

namespace Allocate {


std::string Create(size_t size) {
   std::string heapIncrease(size, 'a');
   return heapIncrease;
}

}
