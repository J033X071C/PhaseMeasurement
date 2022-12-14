cmake_minimum_required(VERSION 3.0)
project(vx2740)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   # in standalone mode, install under <midas>/bin
   set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../bin CACHE PATH "" FORCE)
else()
   # there is an user choice or a higher level project
   # specifying the install path - keep that
   set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

# Check for MIDASSYS environment variable
if (NOT DEFINED ENV{MIDASSYS})
   message(SEND_ERROR "MIDASSYS environment variable not defined.")
endif()

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    message(STATUS "Setting default build type to \"RelWithDebInfo\"")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

# See where to find CAEN libraries.
# Priority is:
# 1) Environment variable $CAEN_INSTALL (then sub-dirs /lib and /include)
# 2) Command-line args -DCAEN_FELIB_LIBDIR=/path/to/libdir and -DCAEN_FELIB_INCDIR=/path/to/include
# 3) Hoping everything is already available via other means (probably a global install)
if (NOT $ENV{CAEN_INSTALL} STREQUAL "")
    message(STATUS "Using environment variable $CAEN_INSTALL to find CAEN FELib.")
    set(CAEN_FELIB_LIBDIR $ENV{CAEN_INSTALL}/lib)
    set(CAEN_FELIB_INCDIR $ENV{CAEN_INSTALL}/include)
endif()

if (NOT CAEN_FELIB_LIBDIR OR CAEN_FELIB_LIBDIR STREQUAL "")
    message(STATUS "Assuming CAEN FELib is already in path. Override with -DCAEN_FELIB_LIBDIR=/path/to/libdir if incorrect")
    set(CAEN_LIBS -lCAEN_FELib)
else()
    set(CAEN_LIBS -L${CAEN_FELIB_LIBDIR} -lCAEN_FELib)
endif()

if (NOT CAEN_FELIB_INCDIR OR CAEN_FELIB_INCDIR STREQUAL "")
    message(STATUS "Assuming CAEN FELib headers are already in path. Override with -DCAEN_FELIB_INCDIR=/path/to/include if incorrect")
endif()

set(CMAKE_CXX_STANDARD 11)
set(MIDASSYS $ENV{MIDASSYS})
set(LIBS -ldl -lpthread -lutil ${CAEN_LIBS})

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    set(LIBS ${LIBS} -lrt -ldl)
endif()

set(LIBSRC_VX2740
  caen_device.cxx
  caen_parameters.cxx
  caen_data.cxx
  caen_commands.cxx
  caen_event.cxx
  odb_wrapper.cxx
  fe_utils.cxx
  fe_settings_strategy.cxx
  fe_settings.cxx
  vx2740_wrapper.cxx
  vx2740_fe_class.cxx
)

set(INCDIRS
  ${MIDASSYS}/include
  ${MIDASSYS}/mxml
  ${CAEN_FELIB_INCDIR}
)

add_library(objlib_vx2740 OBJECT ${LIBSRC_VX2740})
set_property(TARGET objlib_vx2740 PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(static_vx2740 STATIC $<TARGET_OBJECTS:objlib_vx2740>)

add_executable(vx2740_single_fe vx2740_single_fe.cxx)
add_executable(vx2740_group_fe vx2740_group_fe.cxx)
add_executable(vx2740_group_fe_no_odb vx2740_group_fe_no_odb.cxx)
add_executable(vx2740_test vx2740_test.cxx)
add_executable(vx2740_readout_test vx2740_readout_test.cxx)
add_executable(vx2740_dump_params vx2740_dump_params.cxx)
add_executable(vx2740_dump_user_regs vx2740_dump_user_regs.cxx)
add_executable(vx2740_poke vx2740_poke.cxx)

install(TARGETS vx2740_single_fe DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_group_fe DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_group_fe_no_odb DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_test DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_readout_test DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_dump_params DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_dump_user_regs DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS vx2740_poke DESTINATION ${CMAKE_SOURCE_DIR}/bin)
install(TARGETS static_vx2740 DESTINATION ${CMAKE_SOURCE_DIR}/lib)

target_compile_options(vx2740_single_fe PRIVATE -DUNIX)
target_compile_options(vx2740_group_fe PRIVATE -DUNIX)

target_include_directories(objlib_vx2740 PUBLIC ${INCDIRS})
target_include_directories(static_vx2740 PUBLIC ${INCDIRS})
target_include_directories(vx2740_single_fe PRIVATE ${INCDIRS})
target_include_directories(vx2740_group_fe PRIVATE ${INCDIRS})
target_include_directories(vx2740_group_fe_no_odb PRIVATE ${INCDIRS})
target_include_directories(vx2740_test PRIVATE ${INCDIRS})
target_include_directories(vx2740_readout_test PRIVATE ${INCDIRS})
target_include_directories(vx2740_dump_params PRIVATE ${INCDIRS})
target_include_directories(vx2740_dump_user_regs PRIVATE ${INCDIRS})
target_include_directories(vx2740_poke PRIVATE ${INCDIRS})

target_link_libraries(vx2740_single_fe static_vx2740 ${MIDASSYS}/lib/libmfe.a ${MIDASSYS}/lib/libmidas.a ${LIBS})
target_link_libraries(vx2740_group_fe static_vx2740 ${MIDASSYS}/lib/libmfe.a ${MIDASSYS}/lib/libmidas.a ${LIBS})
target_link_libraries(vx2740_group_fe_no_odb static_vx2740 ${MIDASSYS}/lib/libmfe.a ${MIDASSYS}/lib/libmidas.a ${LIBS})
target_link_libraries(vx2740_test ${LIBS})
target_link_libraries(vx2740_readout_test static_vx2740 ${MIDASSYS}/lib/libmidas.a ${LIBS})
target_link_libraries(vx2740_dump_params static_vx2740 ${LIBS})
target_link_libraries(vx2740_dump_user_regs static_vx2740 ${LIBS})
target_link_libraries(vx2740_poke static_vx2740 ${LIBS})