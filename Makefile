# 1. Variables - Change these if needed
CC = gcc
CFLAGS = -Wall -std=c99 -Wno-missing-braces
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# 2. Target Name
TARGET = zion

# 3. Build Rule
$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET) $(LDFLAGS)

# 4. Cleanup Rule
clean:
	rm -f $(TARGET)
