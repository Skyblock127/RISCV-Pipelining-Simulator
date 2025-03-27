CC = g++
CFLAGS = -Wall -O2

FORWARD_EXE = forward
NOFORWARD_EXE = noforward

FORWARD_SRC = forward.cpp
NOFORWARD_SRC = noforward.cpp

all: $(FORWARD_EXE) $(NOFORWARD_EXE)

$(FORWARD_EXE): $(FORWARD_SRC)
$(CC) $(CFLAGS) -o $@ $<

$(NOFORWARD_EXE): $(NOFORWARD_SRC)
$(CC) $(CFLAGS) -o $@ $<

clean:
rm -f $(FORWARD_EXE) $(NOFORWARD_EXE)