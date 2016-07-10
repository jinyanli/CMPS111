all : diskinit fusefat32 create makemount mountx login

diskinit : diskinit.c 
	cc -Wall -lm -g -o diskinit diskinit.c 

fusefat32 : fusefat32.c
	cc -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse  -pthread -L/usr/local/lib -lfuse -lrt fusefat32.c -o fusefat32 -lm

create: diskinit
	./diskinit diskImage 1024

makemount:
	mkdir mount

mountx: fusefat32 mount
	sudo ./fusefat32 mount

login:
	sudo -s


clean : unMount rmMount cleanall

cleanall:
	rm -f diskinit fusefat32 diskImage

unMount: ./mount
	sudo umount ./mount

rmMount: mount
	rm -r mount
