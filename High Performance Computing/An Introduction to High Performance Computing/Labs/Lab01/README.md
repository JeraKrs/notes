Introduction to High Performance Computing - Lab 1


To make sure that you are ready to use the BlueCrystal Phase 3 supercomputer for your coursework you must be able to follow a typical workflow which supercomputer users follow.



Using what you have learnt in the first lab session, your first exercise is to copy a program source file from a lab machine/your own machine to the supercomputer, compile that program and then execute it on the supercomputer by submitting it to the queue.



To do this follow these steps:

1. Download the tarball from 

  https://www.ole.bris.ac.uk/bbcswebdav/courses/COMS30005_2017/Open%20Access%20for%20CS/labs/lab1.tar.gz

  to your own machine/lab machine.

2. Using your terminal, navigate to where the tarball was downloaded to and extract it using:

	tar -xzf lab1.tar.gz

This should create a directory called "lab1" and inside that directory you should find a "Hello, World" C source file (lab1.c) and a job submit script (lab1.job).

3. Copy the lab1 directory to your local area on the BlueCrystal Phase 3 supercomputer (Hint: use scp).

4. Compile your program using gcc

	gcc -Wall -o lab1 lab1.c

5. Submit a job using the provided "lab1.job" job submit script (Hint: use qsub).

6. Once your job has completed verify that the output file "lab1.out" contains "Hello, World!".

7. Repeat steps 4-6, however use the Intel Compiler v16.2 instead of gcc (Hint: the module you need to load is called "languages/intel-compiler-16-u2" and the compiler executable is called icc).



If you can successfully complete all these steps you should be able to use the BlueCrystal Phase 3 supercomputer for your coursework!
