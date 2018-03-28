#include "matrix.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int dim_row, dim_col, num_rows, first_row = 0;

    int rank, mpi_size;
    MPI_Init(&argc, &argv);                   // starts MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);     // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size); // get number of processes

    if (argc > 4)
    {
        dim_row = atoi(argv[1]);
        dim_col = atoi(argv[2]);
    }
    else
    {
        fprintf(stderr, "Use arguments to specify number of rows and columns, and the name of the files contraining the matrix and the vector \n\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int *recv_count = calloc(mpi_size, sizeof(int)),
        *first_rows = calloc(mpi_size, sizeof(int));

    // Calculate the number of rows for each processor, and their first row
    for (int p = 0; p < mpi_size; p++)
    {
        // Each processor gets dim_rows/mpi_size, +1 to account for the remainder
        num_rows = p < dim_row % mpi_size ? dim_row / mpi_size + 1 : dim_row / mpi_size;
        recv_count[p] = num_rows;

        first_rows[p] = first_row;
        first_row += num_rows;
    }
    first_row = first_rows[rank];
    num_rows = recv_count[rank];
    // Load the (local) matrix and vector from the file
    struct Matrix mtx = load_matrix(argv[3], first_row, num_rows);
    struct Vector vec = load_vector(argv[4], first_row, num_rows);

    //Create local and global x vector
    struct Vector x_local = create_vector(num_rows);
    struct Vector x_global = create_vector(dim_col);

    // Jacobi method
    for (int lol = 0; lol < 15; lol++)
    {

        // Send and receive each value of the global X
        for (int i = 0; i < dim_col; i++)
        {
            // Send him if i is in x_local
            if (i >= first_row && i < first_row + num_rows)
            {
                // Copy to the global X vector
                x_global.vector[i] = x_local.vector[i - first_row];
                // Send the value to the other processors
                for (int p = 0; p < mpi_size; p++)
                {
                    if (p != rank)
                    {
                        MPI_Send(&x_global.vector[i], 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                    }
                }
            }
            else
            {
                // find which processor will send the data
                int k = i * mpi_size / dim_col;
                // Receive the data from the processor
                MPI_Recv(&x_global.vector[i], 1, MPI_DOUBLE, k, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            }
        }
        print_vector(x_global);

        // Calculate the next iteration of x_local
        // x2_i += 1/a_ii(b_i-sum(a_ijx1_j))
        for (int i = 0; i < num_rows; i++)
        {
            x_local.vector[i] = 0;
            for (int j = 0; j < dim_col; j++)
            {
                if (i + first_row != j)
                {
                    x_local.vector[i] += mtx.matrix[i][j] * x_global.vector[j];
                }
            }
            x_local.vector[i] = (1 / mtx.matrix[i][i + first_row]) * (vec.vector[i] - x_local.vector[i]);
        }
    }
    print_vector(x_global);
    destruct_vector(&x_local);
    destruct_vector(&x_global);
    destruct_vector(&vec);
    destruct_matrix(&mtx);
    MPI_Finalize();
    return 0;
}
