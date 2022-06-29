add_contract(xdaostg xdaostg
${CMAKE_CURRENT_SOURCE_DIR}/src/xdaostg.cpp
)

target_include_directories(xdaostg
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(xdaostg
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/xdaostg.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/xdaostg.contracts.md @ONLY )

target_compile_options( xdaostg PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
