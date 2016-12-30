cmake_minimum_required(VERSION 3.6)
project(FreeRTOS_Sim)

set(CMAKE_CXX_STANDARD 11)

include_directories(Project)
include_directories(Source/include)
include_directories(Source/portable/GCC/POSIX)

set(SOURCE_FILES
        Project/main.c
        Source/portable/GCC/POSIX/port.c
        Source/portable/MemMang/heap_3.c
        Source/croutine.c
        Source/event_groups.c
        Source/list.c
        Source/queue.c
        Source/tasks.c
        Source/timers.c)

add_executable(FreeRTOS_Sim ${SOURCE_FILES})