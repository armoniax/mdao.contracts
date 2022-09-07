add_contract(xdao.fixswap xdao.fixswap
${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.fixswap.cpp
)

target_include_directories(xdao.fixswap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.stg/include
   )


set_target_properties(xdao.fixswap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/xdao.fixswap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/xdao.fixswap.contracts.md @ONLY )

target_compile_options( xdao.fixswap PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )


