add_executable(eventd
    main.cpp
    eventd.cpp
)

target_compile_features(eventd PRIVATE cxx_std_20)