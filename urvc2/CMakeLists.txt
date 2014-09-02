file(GLOB urvc2src *.c)
add_library(urvc2 STATIC ${urvc2src})
#install(TARGETS urvc2 ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/urvc2)

