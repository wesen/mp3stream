all: mkpasswd

mkpasswd: mkpasswd.c
	$(CC) -o $@ $(CFLAGS) $< -lcrypt

install: mkpasswd
	cp $< /usr/local/bin

clean:
	- rm -rf mkpasswd

