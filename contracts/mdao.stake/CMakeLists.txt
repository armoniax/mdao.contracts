add_contract(mdao.stake mdao.stake
${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.stake.cpp
)

target_include_directories(mdao.stake
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.info/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.gov/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty

   )

set_target_properties(mdao.stake
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/mdao.stg.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/mdao.stg.contracts.md @ONLY )

target_compile_options( mdao.stake PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
