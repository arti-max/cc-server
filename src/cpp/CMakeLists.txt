include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE CORE_SOURCES "*.cpp")

add_library(core SHARED ${CORE_SOURCES})

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_libraries(core PRIVATE -static-libstdc++ -static-libgcc)
endif()

set_target_properties(core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS core DESTINATION lib)
message(STATUS "C++ core library will be built as 'core.so' or 'core.dll'")
