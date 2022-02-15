I finished both set 1 and set 2 functions, except for the "&" one.
Ive made functions for input redirection, output redirection and piping.
also created a function that takes the command line string and parses it into
a string array.

limitaions: 
cannot use > < | together in one line:
    e.g WONT WORK: sort < ls -l > textfile.txt
if useing > or <, it must be followed with a txt only:
    e.g. WONT WORK: ls -l > textfile.txt <smth else>

To run code, put in correct directory:
        make
        ./myShell
and the shell should run. To remove myShell, put in make clean.



Affan Khan 1095729
