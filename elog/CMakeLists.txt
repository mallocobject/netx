# file(GLOB LIB_SOURCES CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# add_library(elog STATIC ${LIB_SOURCES})

add_library(elog INTERFACE)

target_include_directories(elog 
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(elog PROPERTIES DEBUG_POSTFIX "_d")
endif()