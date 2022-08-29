add_contract(fixswap fixswap
${CMAKE_CURRENT_SOURCE_DIR}/src/fixswap.cpp
)

target_include_directories(fixswap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(fixswap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/fixswap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/fixswap.contracts.md @ONLY )

target_compile_options( fixswap PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
