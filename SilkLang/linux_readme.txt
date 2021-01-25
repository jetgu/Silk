How to build the Silk in linux?
1. install gcc and g++
2. run the build.sh directly or build with the following command:
g++ SilkLang.cpp Library.cpp AutoLock.cpp Lexer.cpp Interpreter.cpp Variable.cpp Token.cpp CallStack.cpp Parser.cpp AST.cpp GlobalData.cpp -o silk -ldl  -lpthread

You may need to add -std=c++11 in some old version of Linux:
g++ -std=c++11 SilkLang.cpp Library.cpp AutoLock.cpp Lexer.cpp Interpreter.cpp Variable.cpp Token.cpp CallStack.cpp Parser.cpp AST.cpp GlobalData.cpp -o silk -ldl  -lpthread

To compile with the static c++ lib, please add -static-libstdc++:
g++ SilkLang.cpp Library.cpp AutoLock.cpp Lexer.cpp Interpreter.cpp Variable.cpp Token.cpp CallStack.cpp Parser.cpp AST.cpp GlobalData.cpp -o silk -ldl  -lpthread -static-libstdc++
