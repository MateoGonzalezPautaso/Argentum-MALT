file(GLOB EDITOR_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB EDITOR_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(taller_editor
    PRIVATE
    ${EDITOR_SRC}
    PUBLIC
    ${EDITOR_HEADERS}
)