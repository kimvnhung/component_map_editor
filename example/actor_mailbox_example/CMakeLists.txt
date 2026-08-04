cmake_minimum_required(VERSION 3.10)
project(actor_mailbox_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp
    Actor.h
    Actor.cpp
    Mailbox.h Mailbox.cpp
    Message.h Message.cpp
    ClockActor.h ClockActor.cpp
    ActorSystem.h ActorSystem.cpp
    GraphExecutionSandboxSim.h GraphExecutionSandboxSim.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE base_log)