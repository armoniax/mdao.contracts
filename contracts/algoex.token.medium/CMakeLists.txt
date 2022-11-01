add_contract(tokenmedium1 algoex.token.medium ${CMAKE_CURRENT_SOURCE_DIR}/src/algoex.token.medium.cpp)

target_include_directories(algoex.token.medium
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include   
   ${CMAKE_CURRENT_SOURCE_DIR}/../algoex.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.gov/include   
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.info/include   
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stake/include   

   )
set_target_properties(algoex.token.medium
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( algoex.token.medium PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
