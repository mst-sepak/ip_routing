CC		= gcc
CFLAGS	= -Wall -Wextra -O2 -Iinclude -MMD -MP

SRCS	= src/main.c src/router.c
OBJS	= $(SRCS:.c=.o)
DEPS	= $(SRCS:.c=.d)
TARGET 	= iprouter

all: $(TARGET)

# 実行ファイル(iprouter)の作成
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

# オブジェクトファイルの作成（コンパイル）
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean

-include $(DEPS)