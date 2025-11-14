include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE CORE_SOURCES "*.cpp")

add_library(core SHARED ${CORE_SOURCES})

set_target_properties(core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS core DESTINATION lib)
message(STATUS "C++ core library will be built as 'core.so' or 'core.dll'")
