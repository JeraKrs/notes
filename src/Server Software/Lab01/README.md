# Intorduction Lab

This lab will help you refresh your knowledge of C programming in preparation for coursework 1. The exercises are based on the material from week 1 in the lectures.

First, create a named pipe "PIPE" with the shell command `mkfifo PIPE`.

Write a C program that forks into two processes, parent and child.

The parent executes the following steps.

1. Open a file named "PIPE" for writing.
2. Write the line "parent writing to pipe" to standard output.
3. Write the line "hello 1" to the open file.
4. Write the line "hello 2" to the open file.
5. Close the file.
6. Wait for the child to finish.

The child executes the following steps.

1. Open a file named "PIPE" for reading (assume this exist).
2. Read the file line by line until end-of-file. For every line, print "Child got: %s" to standard output where %s stands for the line you read.
3. Close the file.
4. Write the line "child done" to standard output.

Hints:

* You may use a fixed-size buffer (e.g. 100 characters) for reading lines in the child process. But make sure you can't get a buffer overflow even if you read a line that's longer than the limit.
* Look up C functions in the manual pages - "man 3 name" works for many functions e.g. "man 3 fopen". (Some functions are in a different section of the manual - try "man 2 name" or simply "man name".)
* Make sure you understand which functions can cause errors and how to handle them. The manual pages for each function explain what errors are possible.
* Compile your program with "-Wall" to catch common errors and "-std=gnu99" to allow for modern C constructs (or gcc will default to gnu89).
* Make sure the child prints each line exactly once.

If you found this task easy, adapt the program to take one command-line argument, a file anme. The parent should read this file line by line and write the lines to the pipe. The child should still print every thing it gets from the pipe line by line.
