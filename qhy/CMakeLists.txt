FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.hex")
install(FILES ${files} DESTINATION /etc/qhyccd)
install(FILES 85-qhy.rules DESTINATION /etc/udev/rules.d) 

