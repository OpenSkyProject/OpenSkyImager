set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS -std=c++0x)

#find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
#include_directories( ${CFITSIO_INCLUDE_DIR})

############# GENERIC CCD ###############
#if (CFITSIO_FOUND)

set(indi_osi_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/indi_ccd.cpp
)

add_executable(indi_osi_ccd ${core_SRCS} ${cpp_SRCS} ${indi_osi_SRCS})

target_link_libraries(indi_osi_ccd ${INDI_DRIVER_LIBRARIES} ${COMMON_LIBRARIES} ${CFITSIO_LIBRARIES} m ${ZLIB_LIBRARY})

install(TARGETS indi_osi_ccd RUNTIME DESTINATION bin)

#endif (CFITSIO_FOUND)

install(FILES indi_osi_ccd.xml DESTINATION ${INDI_DATA_DIR})
