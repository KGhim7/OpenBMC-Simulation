add_executable(sensord
    main.cpp
    sensor.cpp
    tcp_client.cpp
)

target_compile_features(sensord PRIVATE cxx_std_20)