cmake_minimum_required(VERSION 3.12)
project(OBD2_Diagnostic_Tool)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compile options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.c
    src/obd2_core.c
    src/obd2_protocol.c
    src/protocol_can.c
    src/protocol_j1850.c
    src/protocol_kwp2000.c
    src/diagnostics.c
    src/j2534_interface.c
)

# Create executable
add_executable(obd2_program ${SOURCES})

# Link libraries
if(WIN32)
    target_link_libraries(obd2_program PRIVATE ws2_32)
endif()

# Installation rules
install(TARGETS obd2_program
    RUNTIME DESTINATION bin
)

install(FILES 
    CONFIG.CFG
    README.md
    LICENSE
    DESTINATION .
)

install(DIRECTORY docs/
    DESTINATION docs
    FILES_MATCHING PATTERN "*.md"
)
