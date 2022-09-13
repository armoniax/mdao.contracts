add_contract(fixswap mdao.fixswap
${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.fixswap.cpp
)

target_include_directories(mdao.fixswap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stg/include
   )


set_target_properties(mdao.fixswap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/mdao.fixswap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/mdao.fixswap.contracts.md @ONLY )

target_compile_options( mdao.fixswap PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )


