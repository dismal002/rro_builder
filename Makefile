CXX = g++
CXXFLAGS = -std=c++17 -O2
TARGET = rro_builder

all: $(TARGET)

$(TARGET): rro_builder.cpp
	$(CXX) $(CXXFLAGS) rro_builder.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf build_temp
	rm -rf dist

.PHONY: all clean
