CXX = g++
SRC = main.cpp driver.cpp parser.cpp scanner.cpp printer.cpp dependency.cpp checker.cpp fold.cpp intermediate_code.cpp optimize.cpp
OBJ = ${SRC:.cpp=.o}
CXXFLAGS = -std=c++23 -g

project3: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

main.o: main.cpp checker.hpp ast.hpp location.hpp visitor.hpp context.hpp \
  intermediate_code.hpp dependency.hpp driver.hpp parser.hpp fold.hpp \
  printer.hpp optimize.hpp

driver.o: driver.cpp driver.hpp parser.hpp context.hpp ast.hpp \
  location.hpp visitor.hpp intermediate_code.hpp scanner.hpp

printer.o: printer.cpp printer.hpp context.hpp ast.hpp location.hpp \
  visitor.hpp intermediate_code.hpp

dependency.o: dependency.cpp dependency.hpp context.hpp ast.hpp \
  location.hpp visitor.hpp intermediate_code.hpp

checker.o: checker.cpp checker.hpp ast.hpp location.hpp visitor.hpp \
  context.hpp intermediate_code.hpp

fold.o: fold.cpp fold.hpp context.hpp ast.hpp location.hpp visitor.hpp \
  intermediate_code.hpp

intermediate_code.o: intermediate_code.cpp intermediate_code.hpp \
  context.hpp ast.hpp location.hpp visitor.hpp

parser.o: parser.cpp parser.hpp context.hpp ast.hpp location.hpp \
  visitor.hpp intermediate_code.hpp scanner.hpp driver.hpp

scanner.o: scanner.cpp scanner.hpp driver.hpp parser.hpp context.hpp \
  ast.hpp location.hpp visitor.hpp intermediate_code.hpp

parser.cpp parser.hpp location.hpp: parser.yy
	bison -o parser.cpp $^

scanner.cpp: scanner.ll scanner.hpp
	flex -o $@ $<

clean:
	rm $(OBJ) scanner.cpp parser.{cpp,hpp} project3

clean_output:
	rm *.json
