add_executable (goal
  Goal.cpp
  CodeGen.cpp
  Lexer.cpp
  Parser.cpp
  Sema.cpp
  )
target_link_libraries(goal PRIVATE ${llvm_libs})
