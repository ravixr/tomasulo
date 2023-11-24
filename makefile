CXX = g++
CFLAGS_DEBUG = -g -Wall -Wextra -pedantic -std=c++11 -fsanitize=address -fsanitize=undefined -O0
CFLAGS_RELEASE = -O2 -std=c++11

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = tomasulo

.PHONY: release debug clean

release: CFLAGS = $(CFLAGS_RELEASE)
release: $(TARGET)

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)