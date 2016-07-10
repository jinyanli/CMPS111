myshell: myshell.o lex.yy.o  
	cc -Wall -g -o myshell myshell.o lex.yy.o -lfl

myshell.o : myshell.c
	cc -c -Wall -g myshell.c

lex.yy.o : lex.yy.c  
	cc -c -g lex.yy.c

lex.yy.c : shell.l
	flex shell.l

clean :
	rm -f lex.yy.o myshell.o lex.yy.c myshell myshell.core