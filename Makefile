: myShell

myShell: myShell.c
	gcc -std=gnu99 -Wpedantic -o myShell myShell.c

clean:
	rm myShell