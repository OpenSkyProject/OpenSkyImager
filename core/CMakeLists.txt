include_directories(${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB sources "*.c" "*.cpp")
set(core_SRCS ${sources} PARENT_SCOPE)
