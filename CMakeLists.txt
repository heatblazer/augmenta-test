cmake_minimum_required(VERSION 3.5)

project(OrderCacheTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#added for using main.cpp for small tests no impact on the original assignment
add_compile_options(-DNOTEST)

add_executable(OrderCacheTest
    TESTING.txt README.txt OrderCache.h OrderCache.cpp
    main.cpp
    OrderCacheTest.cpp)

find_package(OpenCL REQUIRED)
target_link_libraries(OrderCacheTest OpenCL::OpenCL)


include(GNUInstallDirs)
install(TARGETS OrderCacheTest
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
