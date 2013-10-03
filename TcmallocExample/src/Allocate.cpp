
#include <string>
#include "Allocate.h"

namespace {
static const std::string staticStr = Allocate::Create(size_t(2000)*1024*1024);
}

namespace Allocate {


std::string Create(size_t size) {
   std::string heapIncrease(size, 'a');
   return heapIncrease;
}

}
