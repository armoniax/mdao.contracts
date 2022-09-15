add_contract(mdao.redpack mdao.redpack ${CMAKE_CURRENT_SOURCE_DIR}/src/redpack.cpp)

if(DEFINED ENV{DAY_SECONDS_FOR_TEST})
   message(WARNING "ENV{DAY_SECONDS_FOR_TEST}=$ENV{DAY_SECONDS_FOR_TEST} should use only for test!!!")
   target_compile_definitions(tg.redpack PUBLIC "DAY_SECONDS_FOR_TEST=$ENV{DAY_SECONDS_FOR_TEST}")
endif()


target_include_directories(mdao.redpack
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(mdao.redpack
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")


target_compile_options( mdao.redpack PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
