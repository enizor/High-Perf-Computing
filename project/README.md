Jacobi method project
=====================

## Description

The project is separated in 3 parts:
 - the code, contained in code/, with the different librairies
 - the report, contained in report/
 - the slides, contained in slides/
 - the matrices and vectors examples, in examples/

## Usage

```
mpicc code/*.c -o out
mpirun -n <number_of_processors> ./out <dim_row> <dim_col> <matrix_file_path> <vector_file_path> <sync_type>
```
sync_type correspond to the choice of synchronous Send/Recv with MPI.
0: Synchronous
1: Asynchronous with MPI_Barrier
2: Asynchronous without MPI_Barrier
