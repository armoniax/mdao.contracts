add_contract(mdao.stg mdao.stg
${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.stg.cpp
)

target_include_directories(mdao.stg
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stake/include
   )

set_target_properties(mdao.stg
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/mdao.stg.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/mdao.stg.contracts.md @ONLY )

target_compile_options( mdao.stg PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
