cmake_minimum_required(VERSION 3.5.1)
project(highload)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES main.cpp web-server.h model/user.cpp model/user.h model/location.cpp model/location.h model/visit.cpp model/visit.h state.cpp state.h routes/base.h routes/get_entity.h model/entity.cpp model/entity.h utility.cpp utility.h routes/get_user_visits.h routes/get_location_avg.h routes/post_entity.h logger.h thread_pool.h json.h fast_hash_map.h)
add_executable(highload ${SOURCE_FILES})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(highload PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(highload "${CMAKE_THREAD_LIBS_INIT}")
endif()


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

#set(CMAKE_C_FLAGS "-DDEBUG")