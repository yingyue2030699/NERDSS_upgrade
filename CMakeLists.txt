cmake_minimum_required(VERSION 3.5)

project(nerdss)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-omit-frame-pointer")

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(NERDSS_ENABLE_ASAN "Build the serial executable with AddressSanitizer." OFF)
option(NERDSS_ENABLE_UBSAN "Build the serial executable with UndefinedBehaviorSanitizer." OFF)

set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to default, '${default_build_type}'.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please use a different directory (e.g. 'build').")
endif()

# Get system information
cmake_host_system_information(RESULT NUM_CPUS QUERY NUMBER_OF_PHYSICAL_CORES)
cmake_host_system_information(RESULT NUM_THREADS QUERY NUMBER_OF_LOGICAL_CORES)

# Print system information
message(STATUS "We are on a ${CMAKE_SYSTEM_NAME} system")
message(STATUS "The host processor is ${CMAKE_HOST_SYSTEM_PROCESSOR} with ${NUM_CPUS} physical cores and ${NUM_THREADS} logical cores.")

include_directories(include)

include(CTest)

#add_definitions(-D_DEBUG) # comment this line if no need _DEBUG

set(SOURCE_DIRS
    boundary_conditions
    classes
    error
    math
    parser
    reactions
    system_setup
    trajectory_functions
    io
)

foreach(SOURCE_DIR ${SOURCE_DIRS})
    file(GLOB DIR_SOURCES "src/${SOURCE_DIR}/*.cpp")
    list(APPEND SOURCES ${DIR_SOURCES})
endforeach()

add_executable(nerdss ${SOURCES} EXEs/nerdss.cpp)

set(NERDSS_SANITIZER_FLAGS "")
if(NERDSS_ENABLE_ASAN)
    list(APPEND NERDSS_SANITIZER_FLAGS -fsanitize=address)
endif()
if(NERDSS_ENABLE_UBSAN)
    list(APPEND NERDSS_SANITIZER_FLAGS -fsanitize=undefined)
endif()
if(NERDSS_SANITIZER_FLAGS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(WARNING "Sanitizer flags are only validated with Clang and GCC.")
    endif()
    list(APPEND NERDSS_SANITIZER_FLAGS -fno-omit-frame-pointer)
    target_compile_options(nerdss PRIVATE ${NERDSS_SANITIZER_FLAGS})
    target_link_libraries(nerdss PRIVATE ${NERDSS_SANITIZER_FLAGS})
endif()

# Set up external libraries
find_package(GSL REQUIRED)
target_link_libraries(nerdss PRIVATE GSL::gsl GSL::gslcblas)

# Set up header directories
include_directories(include $(GSL_INCLUDE_DIR))

if(BUILD_TESTING)
    add_executable(nerdss_unit_tests
        tests/unit/test_vector_coord.cpp
        src/classes/class_Coord.cpp
        src/classes/class_Vector.cpp
        src/math/Faddeeva.cpp
        src/math/matrix_functions.cpp
        src/reactions/angleSignIsCorrect.cpp
        src/reactions/areParallel.cpp
        src/reactions/areSameAngle.cpp
        src/reactions/passocF.cpp
        src/reactions/passocF_1D.cpp
        src/reactions/pirr_pfree_ratio_psF.cpp
        src/reactions/pirr_pfree_ratio_psF_1D.cpp
        src/reactions/functions_implicitlipid.cpp
        src/reactions/get_geodesic_distance.cpp
        src/reactions/norm_function.cpp
        src/reactions/pir_function.cpp
        src/reactions/survival_function.cpp
    )
    target_include_directories(nerdss_unit_tests PRIVATE include)
    target_link_libraries(nerdss_unit_tests PRIVATE GSL::gsl GSL::gslcblas)
    add_test(NAME nerdss_unit_tests COMMAND nerdss_unit_tests)
endif()
