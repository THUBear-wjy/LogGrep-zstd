# cc = g++
# EXEC = THULR
# SRCS = main.cpp sampler.cpp Coffer.cpp Encoder.cpp util.cpp LengthParser.cpp template.cpp union.cpp SubPattern.cpp timer.cpp
# OBJS = $(SRCS:.cpp=.o)

# LIBS = -llzma
# LIBPATH = -L ./LZMA

# start:$(OBJS)
# 	$(cc) -o $(EXEC) $(OBJS) $(LIBPATH) $(LIBS)
# 	rm -rf $(OBJS)
# .cpp.o:
# 	$(cc) -std=c++11 -o $@ -c $^
# clean:
# 	rm -rf $(OBJS) $(EXEC)


LIBDIR =../zstd-dev/lib
CPPFLAGS += -I$(LIBDIR)
LIB = $(LIBDIR)/libzstd.a

cc = g++
EXEC = THULR
SRCS = main.cpp sampler.cpp Coffer.cpp Encoder.cpp util.cpp LengthParser.cpp template.cpp union.cpp SubPattern.cpp timer.cpp
OBJS = $(SRCS:.cpp=.o)


start:$(OBJS)
	$(cc) $(OBJS) $(LIB) -o $(EXEC) $(LIBPATH) $(LIBS) 
	rm -rf $(OBJS)
.cpp.o:
	$(cc) -I$(LIBDIR) -O2 -std=c++11 -g -Wall -o $@ -c $^ 
clean:
	rm -rf $(OBJS) $(EXEC)