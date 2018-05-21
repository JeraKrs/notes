# Add any `module load` or `export` commands that your code needs to
# compile and run to this file.

module use /mnt/storage/easybuild/modules/local
module use /mnt/storage/scratch/jp8463/modules/modulefiles
module load clang-ykt/2017-07-24
module load languages/intel/2017.01
module load languages/anaconda2/5.0.1
module load git/2.4.1-GCC-4.9.2
module load CUDA/8.0.44

module use /mnt/storage/scratch/jp8463/modules/modulefiles
module load oclgrind
module load icc/2016.3.210-GCC-5.4.0-2.26
