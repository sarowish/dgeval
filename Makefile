CXX = g++
SRC = main.cpp driver.cpp parser.cpp scanner.cpp printer.cpp dependency.cpp \
	  checker.cpp fold.cpp linear_ir.cpp optimize.cpp codegen.cpp lang_runtime.cpp runtime_library.cpp
OBJ = ${SRC:.cpp=.o}
CXXFLAGS = -std=c++23 -g

project4: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

main.o: main.cpp checker.hpp ast.hpp location.hpp runtime_library.hpp \
  visitor.hpp context.hpp linear_ir.hpp codegen.hpp lang_runtime.hpp \
  dependency.hpp driver.hpp parser.hpp fold.hpp optimize.hpp printer.hpp

checker.o: checker.cpp checker.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp context.hpp linear_ir.hpp

codegen.o: codegen.cpp codegen.hpp context.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp linear_ir.hpp lang_runtime.hpp

dependency.o: dependency.cpp dependency.hpp context.hpp ast.hpp \
  location.hpp runtime_library.hpp visitor.hpp linear_ir.hpp

driver.o: driver.cpp driver.hpp parser.hpp context.hpp ast.hpp \
  location.hpp runtime_library.hpp visitor.hpp linear_ir.hpp scanner.hpp

fold.o: fold.cpp fold.hpp context.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp linear_ir.hpp

lang_runtime.o: lang_runtime.cpp lang_runtime.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp

linear_ir.o: linear_ir.cpp linear_ir.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp context.hpp optimize.hpp

optimize.o: optimize.cpp optimize.hpp linear_ir.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp

parser.o: parser.cpp parser.hpp context.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp linear_ir.hpp scanner.hpp driver.hpp

printer.o: printer.cpp printer.hpp context.hpp ast.hpp location.hpp \
  runtime_library.hpp visitor.hpp linear_ir.hpp

runtime_library.o: runtime_library.cpp runtime_library.hpp \
  lang_runtime.hpp ast.hpp location.hpp visitor.hpp

scanner.o: scanner.cpp scanner.hpp driver.hpp parser.hpp context.hpp \
  ast.hpp location.hpp runtime_library.hpp visitor.hpp linear_ir.hpp

parser.cpp parser.hpp location.hpp: parser.yy
	bison -o parser.cpp $^

scanner.cpp: scanner.ll scanner.hpp
	flex -o $@ $<

clean:
	rm $(OBJ) scanner.cpp parser.{cpp,hpp} location.hpp project4

clean_output:
	rm *.json
