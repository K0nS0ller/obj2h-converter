converter:
	gcc converter.c -o obj2h

clean:
	rm obj2h

install:
	cp obj2h /bin/

uninstall:
	rm /bin/obj2h
