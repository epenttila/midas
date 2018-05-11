cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0003)
    cmake_policy(SET CMP0003 NEW)
endif()

if(POLICY CMP0020)
    cmake_policy(SET CMP0020 NEW)
endif()

project(midas)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Werror -pedantic -Wall -Wextra -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -march=native")

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_definitions(-DBOOST_ALL_DYN_LINK)
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /wd4503 /wd4251 /wd4714 /bigobj /DNOMINMAX")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /Oy /GL /fp:fast /MD")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF,ICF")

    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Ob2 /Oi /Ot /Oy /GL /fp:fast /MD")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /INCREMENTAL:NO /OPT:REF,ICF")

    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)

    add_definitions(-DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_VARIADIC_MAX=10 -D_WIN32_WINNT=0x0601)
endif()

option(BUILD_TEST "Build test" ON)
set(TEST_DATA_PATH data CACHE PATH "Test data location")

find_package(OpenMP)

if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

find_package(Boost 1.54.0 COMPONENTS program_options timer system chrono filesystem log_setup log thread iostreams regex REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/src)

find_package(Git)

find_package(PythonLibs 3.5 REQUIRED)

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/ext/pybind/include)

add_custom_target(check)

add_subdirectory(src)

if(BUILD_TEST)
    enable_testing() 
    set(GTEST_ROOT "ext/gtest")
    set(GTEST_INCLUDE_DIR "${GTEST_ROOT}/include")
    set(gtest_force_shared_crt ON)
    include_directories(${GTEST_INCLUDE_DIR}) 
    add_subdirectory(ext)
    add_subdirectory(test)
endif()
