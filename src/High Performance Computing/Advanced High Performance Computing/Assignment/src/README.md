# Submit instructions

README.md: a brief description about all submit files.

report.pdf: the report of this assignment.

cpu.zip: the cpu implementation with best performance.
	- d2q9-bgk.c:	the basic code.
	- env.sh:		contain the module or environment variable commands.
	- job_submit_d2q9-bgk: the jobscript.
	- Makefile:		builds the d2q9-bgk binary by just typing make.

	the implementation is Flat MPI, the best performance for 1024x1024 is
	9.06s by using 4 nodes with 112 cores.


gpu.zip: the gpu implementation with best performance.
	the folder has two version of codes, OpenCL and MPI+OpenCL.

	OpenCL: single GPU program which the best performance for 1024x1024 is 3.5s.
		- d2q9-bgk.c:	the basic code.
		- env.sh:		contain the module or environment variable commands.
		- job_submit_d2q9-bgk: the jobscript.
		- Makefile:		builds the d2q9-bgk binary by just typing make.
		- opencl.cl:	the kernel function for OpenCL.
	
	MPI+OpenCL: multiply GPU program which the best performance for 1024x1024
		is 7.51s by using 2 nodes with 4 GPUs.
		- d2q9-bgk.c:	the basic code.
		- env.sh:		contain the module or environment variable commands.
		- job_submit_d2q9-bgk: the jobscript.
		- Makefile:		builds the d2q9-bgk binary by just typing make.
		- opencl.cl:	the kernel function for OpenCL.
