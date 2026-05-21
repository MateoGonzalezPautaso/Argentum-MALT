file(GLOB SERVER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB SERVER_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(taller_tests
    PRIVATE
    ${SERVER_SRC}
    ${CMAKE_SOURCE_DIR}/client/network/client_protocol.cpp
    ${CMAKE_SOURCE_DIR}/server/server_protocol.cpp
    ${CMAKE_SOURCE_DIR}/server/client_handler.cpp
    ${CMAKE_SOURCE_DIR}/server/sender.cpp
    ${CMAKE_SOURCE_DIR}/server/receiver.cpp
    PUBLIC
    ${SERVER_HEADERS}
)