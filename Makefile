# 1. Variables - Change these if needed
CC = gcc
CFLAGS = -Wall -std=c99 -Wno-missing-braces
TEST_CFLAGS = -g -fsanitize=undefined -fsanitize-undefined-trap-on-error
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# 2. Target Name
TARGET = zion

# 3. Build Rule
$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c -o $(TARGET) $(LDFLAGS)

.PHONY: test clean
test: zion-tests
	./zion-tests

zion-tests: tests/test.c main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_CFLAGS) tests/test.c -o $@ $(LDFLAGS)

# 4. Cleanup Rule
clean:
	rm -f $(TARGET) zion-tests
