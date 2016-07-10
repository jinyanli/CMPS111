!!!!WARNING!!!!!!
*** DO NOT RUN "make" TWO TIMES IN A ROW ***
*** DO NOT RUN "make" TWO TIMES IN A ROW ***

RUN: "make" to compile generated files and mount to "mount directory"
RUN: "make clean" to unmount and removed generated files


--------
RUN make in a terminal. 

This will compile diskinit.c and fusefat32.c, create the diskImage file to mount, the directory called "mount", and mount as sudo, then login as root. Your password will be prompted for sudo.

run "make clean" after exiting file to unmount the "mount" directory 
and remove generated files.

If you would like to run our test commands, run ./test.sh

Currently supported commands are:
cd
ls
ls -l
cat
mkdir
rmdir
rm
