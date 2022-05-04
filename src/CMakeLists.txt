cmake_minimum_required (VERSION 3.8)

add_executable(agn-decoder
    main.c
    agn-decoder/generate_hook.h
    agn-decoder/mesh.h
    agn-decoder/parser.h
    agn-decoder/decoder.h
)

target_include_directories(agn-decoder PUBLIC .)
target_link_libraries(agn-decoder zip)
