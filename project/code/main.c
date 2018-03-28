#include "matrix.h"
#include "vector.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int dim_row, dim_col, num_rows, first_row, sync_type = 0;
    int rank, mpi_size;
    MPI_Init(&argc, &argv);                   // starts MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);     // get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size); // get number of processes

    // Dummy request required for async send
    MPI_Request req;

    if (argc > 5)
    {
        dim_row = atoi(argv[1]);
        dim_col = atoi(argv[2]);
        sync_type = atoi(argv[5]);
    }
    else
    {
        fprintf(stderr, "Use arguments to specify number of rows and columns, the name of the files contraining the matrix and the vector and the sync type \n\n");
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

    // Create local and global x vector
    struct Vector x_local = create_vector(num_rows);
    struct Vector x_global = create_vector(dim_col);

    // Residual analysis booleans
    // Contains true if x_i is still really near its limit
    _Bool *residues = calloc(num_rows, sizeof(_Bool));
    for (int i = 0; i < num_rows; i++)
    {
        residues[i] = false;
    }
    _Bool run = true;
    int iterations = 0;
    // Jacobi method
    while (run)
    {
        // print_vector(x_local);
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
                        if (sync_type == 0)
                        {
                            MPI_Send(&x_global.vector[i], 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                        }
                        else
                        {
                            MPI_Isend(&x_global.vector[i], 1, MPI_DOUBLE, p, 0, MPI_COMM_WORLD, &req);
                        }
                    }
                }
            }
            else
            {
                // find which processor will send the data
                int k = i * mpi_size / dim_col;
                // Receive the data from the processor
                if (sync_type == 0)
                {
                    MPI_Recv(&x_global.vector[i], 1, MPI_DOUBLE, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                else
                {
                    MPI_Irecv(&x_global.vector[i], 1, MPI_DOUBLE, k, 0, MPI_COMM_WORLD, &req);
                }
            }
        }
        if (sync_type == 1)
            MPI_Barrier(MPI_COMM_WORLD);

        // Calculate the next iteration of x_local
        for (int i = 0; i < num_rows; i++)
        {
            // Calculate the next iteration only if
            // the residue indicates it is far from its limit
            if (!residues[i])
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

                // Residual analysis
                if (fabs(x_local.vector[i] - x_global.vector[first_row + i]) < 1e-6)
                {
                    residues[i] = true;
                }
            }
        }

        // Test for whole convergence
        // The root process receives a finishing ping from each other processes
        // and replies with a continue or stop order
        bool local_continue = false;
        for (int i = 0; i < num_rows; i++)
        {
            local_continue |= !residues[i];
        }
        if (rank == 0)
        {
            bool global_continue = local_continue;
            for (int p = 1; p < mpi_size; p++)
            {
                _Bool external_continue = 0;
                if (sync_type == 0)
                {
                    MPI_Recv(&external_continue, 1, MPI_C_BOOL, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                else
                {
                    MPI_Irecv(&external_continue, 1, MPI_C_BOOL, p, 0, MPI_COMM_WORLD, &req);
                }
                global_continue |= external_continue;
            }
            for (int p = 1; p < mpi_size; p++)
            {
                if (sync_type == 0)
                {
                    MPI_Send(&global_continue, 1, MPI_C_BOOL, p, 0, MPI_COMM_WORLD);
                }
                else
                {
                    MPI_Isend(&global_continue, 1, MPI_C_BOOL, p, 0, MPI_COMM_WORLD, &req);
                }
            }
            run = global_continue;
        }
        else
        {
            if (sync_type == 0)
            {
                MPI_Send(&local_continue, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD);
                MPI_Recv(&run, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else
            {
                MPI_Isend(&local_continue, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, &req);
                MPI_Irecv(&run, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, &req);
            }
        }
        if (sync_type == 1)
            MPI_Barrier(MPI_COMM_WORLD);
        iterations++;
    }
    if (rank == 0)
    {
        printf("Converged in %d iterations\nSolution found:\n", iterations);
        print_vector(x_global);
    }
    free(recv_count);
    free(first_rows);
    free(residues);
    destruct_vector(&x_local);
    destruct_vector(&x_global);
    destruct_vector(&vec);
    destruct_matrix(&mtx);
    MPI_Finalize();
    return 0;
}
