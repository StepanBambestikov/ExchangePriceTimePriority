CXX = g++
CXXFLAGSPROD = -std=c++20 -O3 -march=native -DNDEBUG
CXXFLAGSPROF = -std=c++20 -g -O2
CXXFLAGSGPROF = -std=c++20 -g -O2 -pg

TARGET = myapp
TARGET_PROF = myapp_prof
TARGET_GPROF = myapp_gprof

all: $(TARGET)

# Production build (максимальная скорость)
$(TARGET): main.cpp
	$(CXX) $(CXXFLAGSPROD) main.cpp -o $(TARGET)

# Профилирование через callgrind (без -pg!)
$(TARGET_PROF): main.cpp
	$(CXX) $(CXXFLAGSPROF) main.cpp -o $(TARGET_PROF)

# Профилирование через gprof (с -pg)
$(TARGET_GPROF): main.cpp
	$(CXX) $(CXXFLAGSGPROF) main.cpp -o $(TARGET_GPROF)

# Для финальных замеров скорости
benchmark: $(TARGET)
	./$(TARGET)

# Профилирование через gprof
gprof: $(TARGET_GPROF)
	./$(TARGET_GPROF)
	gprof $(TARGET_GPROF) gmon.out > profile_gprof.txt
	@echo "gprof profile saved to profile_gprof.txt"

# Профилирование через valgrind/callgrind (БЕЗ -pg!)
valgrind: $(TARGET_PROF)
	valgrind --tool=callgrind --callgrind-out-file=callgrind.out ./$(TARGET_PROF)
	callgrind_annotate callgrind.out > profile_callgrind.txt
	@echo "callgrind profile saved to profile_callgrind.txt"

# Быстрый просмотр callgrind
valgrind-quick: $(TARGET_PROF)
	valgrind --tool=callgrind ./$(TARGET_PROF)
	callgrind_annotate callgrind.out.* | head -100

clean:
	rm -f $(TARGET) $(TARGET_PROF) $(TARGET_GPROF) gmon.out callgrind.out* profile*.txt

.PHONY: all benchmark gprof valgrind valgrind-quick clean