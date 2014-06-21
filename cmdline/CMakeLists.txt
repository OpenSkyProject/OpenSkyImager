set(CMAKE_CXX_FLAGS -std=c++0x)
add_executable(cmdline ${core_SRCS} main.cpp ${cpp_SRCS} )
target_link_libraries(cmdline ${COMMON_LIBRARIES} ${Boost_LIBRARIES} )

