CC = clang
CFLAGS = -Oz -ffunction-sections -fdata-sections -flto=thin -fuse-ld=lld -Wl,--gc-sections -Wl,--strip-all -lSDL2

all: clean run
	@echo ======
	@echo
	@stat -c%s main | awk '{printf("%.2f KiB\n",$$1/1024)}'

clean:
	rm -rf main

main:
	$(CC) $(CFLAGS) main.c -o main

run: main
	@printf \\n======\\n
	@./main