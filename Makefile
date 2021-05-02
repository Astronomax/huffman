CXX = g++
CXXFLAGS = -std=c++03 -pedantic -Wall -Werror -Wextra -std=c++17 -Iinclude
LDFLAGS = 
OBJDIR = obj

all: hw_02

obj/huffman.o: src/huffman.h src/huffman.cpp $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c src/huffman.cpp -o obj/huffman.o

obj/main.o: src/main.cpp $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o obj/main.o

obj/doctest.o: test/doctest.h test/doctest_fwd.h test/doctest.cpp $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c test/doctest.cpp -o obj/doctest.o

obj/autotest.o: test/autotests.cpp $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c test/autotests.cpp -o obj/autotest.o

hw_02: obj/huffman.o obj/main.o
	$(CXX) obj/huffman.o obj/main.o -o hw_02 $(LDFLAGS)
	
test: obj/autotest.o obj/doctest.o obj/huffman.o
	$(CXX) obj/autotest.o obj/doctest.o obj/huffman.o -o hw_02_test $(LDFLAGS)
		
$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) hw_02 hw_02_test

.PHONY: clean all
