
cmake_minimum_required (VERSION 2.8)
ENABLE_LANGUAGE(CXX)
set(CMAKE_BUILD_TYPE Release)

project (TCMallocExample)
set(SRC ${TCMallocExample_SOURCE_DIR}/src)


# Detect 64 or 32 bit
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
   # 64-bit project
   SET(64_BIT_OS TRUE)
     MESSAGE("A 64-bit OS detected")
 else()
   SET(64_BIT_OS FALSE)
   MESSAGE("A 32-bit OS detected")
endif()


set(PLATFORM_LINK_LIBRIES rt)
set(CMAKE_CXX_FLAGS "-Wall -rdynamic -Wunused -std=c++11 ${CMAKE_CXX_FLAGS_DEBUG} -pthread -D_GLIBCXX_USE_NANOSLEEP")

   # add a Allocator library
   add_library(lib_allocator ${SRC}/Allocate.cpp ${SRC}/Allocate.h)
   set_target_properties(lib_allocator PROPERTIES LINKER_LANGUAGE CXX)

# tcmalloc
#include(cmake/FindTcmalloc.cmake)
#if(${TCMALLOC_FOUND})
#  set(EXTRA_LIBS ${EXTRA_LIBS} ${TCMALLOC_LIBRARIES})
#endif()
set( Tcmalloc_DIR  ${TCMallocExample_SOURCE_DIR})
find_package(Tcmalloc)

  # 1. create the the example EXECUTABLE - hook in the test_example's CMakeLists.txt file
 add_executable(mallocLinkTooLate ${SRC}/main.cpp)
 target_link_libraries(mallocLinkTooLate lib_allocator tcmalloc_minimal )
# ${Tcmalloc_LIBRARIES}

 add_executable(mallocLink ${SRC}/main.cpp)
 target_link_libraries(mallocLink tcmalloc_minimal lib_allocator )
#${Tcmalloc_LIBRARIES}


# commandline creation of static and dynamic libraries 
# and linking them with a binary 
# http://www.iram.fr/~roche/code/c++/AddNumbers.html

# STATIC
# g++ -I../src -c ../src/Allocate.cpp -o AllocateStatic.o
# ar rcs libAllocateStatic.a AllocateStatic.o
# g++ -std=c++11 -I ../src -L ./ ../src/main.cpp -ltcmalloc_minimal -lAllocateStatic -o AllocateStaticEarly
# g++ -std=c++11 -I ../src -L ./ ../src/main.cpp  -lAllocateStatic -ltcmalloc_minimal -o AllocateStaticLate

# DYNAMIC
# g++ -I ../src -fpic -c ../src/Allocate.cpp -o Allocate.o
# g++ -shared -o libAllocate.so Allocate.o
# g++ -std=c++11 -I ../src -L ./ ../src/main.cpp -ltcmalloc_minimal -lAllocate -o Allocate
# export LD_LIBRARY_PATH=/home/kjell/GitHub/doodles/TcmallocExample/build


# g++ -I ../src -fpic -c ../src/Allocate.cpp -o AllocateLate.o
# g++ -shared -o libAllocateLate.so AllocateLate.o
# g++ -std=c++11 -I ../src -L ./ ../src/main.cpp -lAllocateLate -ltcmalloc_minimal -o AllocateLate
# export LD_LIBRARY_PATH=/home/kjell/GitHub/doodles/TcmallocExample/build
