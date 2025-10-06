CXX = g++
CXXFLAGS = -std=c++20 -g -O2 -pg
TARGET = myapp

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

profile: $(TARGET)
	./$(TARGET)
	gprof $(TARGET) gmon.out > profile.txt
	@echo "Profile saved to profile.txt"

valgrind: $(TARGET)
	valgrind --tool=callgrind --callgrind-out-file=callgrind.out ./$(TARGET)
	callgrind_annotate callgrind.out

clean:
	rm -f $(TARGET) gmon.out callgrind.out* profile.txt