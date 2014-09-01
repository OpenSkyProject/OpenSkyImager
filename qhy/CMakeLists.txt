FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.hex")

#set(QHY5_COMPAT_FW off CACHE BOOL "Use QHY5-II compatibility firmware (default: off, use high performance firmware)")

#set(QHY5II_FW /etc/qhyccd/qhy5ii_v3_hiperf.hex)
set(QHY5II_FW /etc/qhyccd/qhy5ii_1024140423_4p.hex)
#if(QHY5_COMPAT_FW)
#  set(QHY5II_FW /etc/qhyccd/qhy5ii_v3_compat.hex)
#endif(QHY5_COMPAT_FW)
configure_file(85-qhy.rules.in 85-qhy.rules @ONLY)

install(FILES ${files} DESTINATION /etc/qhyccd)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/85-qhy.rules DESTINATION /etc/udev/rules.d) 

