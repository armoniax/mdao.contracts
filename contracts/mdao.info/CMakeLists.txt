add_contract(mdao.info mdao.info ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.info.cpp)

target_include_directories(mdao.info
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.gov/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.token/include

)
set_target_properties(mdao.info
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.info PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
