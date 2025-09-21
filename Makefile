CC = clang
CFLAGS = -Oz -ffunction-sections -fdata-sections -flto=full -fuse-ld=lld -Wl,--gc-sections -Wl,--strip-all -lSDL2 -lm

all: clean size

clean:
	rm -rf 32kb 32kbupx

32kb: main.c
	$(CC) $(CFLAGS) main.c -o 32kb
	strip -s 32kb
	upx --best -o32kbupx -f --ultra-brute 32kb

run: 32kb
	./32kb

size: 32kb
	@stat -c%s 32kb 32kbupx main.c | awk '{printf("%.2f KiB\n",$$1/1024)}'
