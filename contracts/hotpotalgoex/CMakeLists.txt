add_contract(hotpotalgoex hotpotalgoex
   ${CMAKE_CURRENT_SOURCE_DIR}/src/hotpotalgoex.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/hotpotalgoexdb.cpp
)

target_include_directories(hotpotalgoex
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../hotpottoken/include
 )

set_target_properties(hotpotalgoex
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/hotpotalgoex.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/hotpotalgoex.contracts.md @ONLY )

target_compile_options( hotpotalgoex PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
