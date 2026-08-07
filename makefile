converter:
	gcc converter.c -o obj2h -lm

clean:
	rm obj2h

install:
	cp obj2h /bin/

uninstall:
	rm /bin/obj2h
