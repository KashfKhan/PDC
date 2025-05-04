CXX = mpic++
CXXFLAGS = -O2 -std=c++17

TARGET = sssp_mpi

SRCS = main.cpp graph_loader.cpp sssp_mpi.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o
