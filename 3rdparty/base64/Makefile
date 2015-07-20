TOP = .
SRCS = $(shell ls *.cpp)
OBJS = $(SRCS:%.cpp=%.o)
LIB = lib$(notdir $(CURDIR)).a
CPPFLAGS+=-g -Wall -W

lib: $(LIB)

$(LIB): $(OBJS)
	ar -rcs $@ $?

clean:
	rm -f *.a *.o
