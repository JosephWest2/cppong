cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(wscpp VERSION 1.0)
add_executable(server server.cpp)
add_executable(client client.cpp)
add_executable(test test.cpp)
target_include_directories(server
    PRIVATE external/json/include/nlohmann
)
target_include_directories(test
    PRIVATE external/json/include/nlohmann
)
target_include_directories(client
    PRIVATE external/json/include/nlohmann
)
# target name is nlohmann_json
add_subdirectory(external/json)
target_link_libraries(server
    PRIVATE nlohmann_json
)
target_link_libraries(test
    PRIVATE nlohmann_json
)
target_link_libraries(client
    PRIVATE nlohmann_json
)