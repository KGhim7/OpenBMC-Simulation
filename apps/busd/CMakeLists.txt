add_executable(busd main.cpp
               tcp_server.cpp)
target_compile_features(busd PRIVATE cxx_std_20)