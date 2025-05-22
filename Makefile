CXX = g++
SRC = main.cpp driver.cpp parser.cpp scanner.cpp printer.cpp dependency.cpp checker.cpp
OBJ = ${SRC:.cpp=.o}
CXXFLAGS = -std=c++23 -O2

project3: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

main.o: main.cpp checker.hpp context.hpp ast.hpp location.hpp visitor.hpp \
  dependency.hpp driver.hpp parser.hpp printer.hpp

driver.o: driver.cpp driver.hpp parser.hpp context.hpp ast.hpp \
  location.hpp visitor.hpp scanner.hpp

printer.o: printer.cpp printer.hpp context.hpp ast.hpp location.hpp \
  visitor.hpp

dependency.o: dependency.cpp dependency.hpp context.hpp ast.hpp \
  location.hpp visitor.hpp

checker.o: checker.cpp checker.hpp context.hpp ast.hpp location.hpp \
  visitor.hpp

parser.o: parser.cpp parser.hpp context.hpp ast.hpp location.hpp \
  visitor.hpp scanner.hpp driver.hpp

scanner.o: scanner.cpp scanner.hpp driver.hpp parser.hpp context.hpp \
  ast.hpp location.hpp visitor.hpp

parser.cpp parser.hpp location.hpp: parser.yy
	bison -o parser.cpp $^

scanner.cpp: scanner.ll scanner.hpp
	flex -o $@ $<

clean:
	rm $(OBJ) scanner.cpp parser.{cpp,hpp} project3

clean_output:
	rm *.json
