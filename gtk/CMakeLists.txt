file(GLOB gtk_SRCS *.c)
add_executable(gtkImager ${core_SRCS} ${gtk_SRCS})
target_link_libraries(gtkImager ${GTK3_LIBRARIES} ${GTK2_LIBRARIES} ${COMMON_LIBRARIES})
install(TARGETS gtkImager RUNTIME DESTINATION bin/${PROJECT_NAME})
