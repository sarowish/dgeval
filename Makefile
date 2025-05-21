CXX = clang++
SRC = main.cpp driver.cpp parser.cpp scanner.cpp printer.cpp dependency.cpp checker.cpp
OBJ = ${SRC:.cpp=.o}
CXXFLAGS = -std=c++23 -O2

project3: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

main.o: main.cpp driver.hpp parser.hpp dependency.hpp ast.hpp visitor.hpp

driver.o: driver.cpp driver.hpp parser.hpp scanner.hpp visitor.hpp

printer.o: printer.cpp printer.hpp ast.hpp context.hpp visitor.hpp

dependency.o: dependency.cpp dependency.hpp ast.hpp context.hpp visitor.hpp

checker.o: checker.cpp checker.hpp ast.hpp visitor.hpp

parser.o: parser.cpp parser.hpp scanner.hpp driver.hpp visitor.hpp ast.hpp

scanner.o: scanner.cpp scanner.hpp parser.hpp driver.hpp ast.hpp


parser.cpp parser.hpp: parser.yy
	bison -o parser.cpp $^

scanner.cpp: scanner.ll scanner.hpp
	flex -o $@ $<

clean:
	rm $(OBJ) scanner.cpp parser.{cpp,hpp} project3

clean_output:
	rm *.json
