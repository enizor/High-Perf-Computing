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

    // Arguments handling
    if (argc > 4)
    {
        dim_row = atoi(argv[1]);
        dim_col = dim_row;
        sync_type = atoi(argv[4]);
    }
    else
    {
        fprintf(stderr, "Use arguments to specify number of rows and columns, the name of the files contraining the matrix and the vector and the sync type \n\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Calculate the number of rows, depending on the processor rank
    num_rows = dim_row / mpi_size;
    first_row = rank * (dim_row / mpi_size);
    if (rank < dim_row % mpi_size)
    {
        num_rows++;
        first_row += rank;
    }
    else
    {
        first_row += dim_row % mpi_size;
    }

    // Load the (local) matrix and vector from the file
    struct Matrix mtx = load_matrix(argv[2], first_row, num_rows);
    struct Vector vec = load_vector(argv[3], first_row, num_rows);

    // Create local and global x vector
    struct Vector x_local = create_vector(num_rows);
    struct Vector x_global = create_vector(dim_col);

    // Residual analysis booleans
    // Contains true if x_i still needs to iterate
    _Bool *residues = calloc(num_rows, sizeof(_Bool));
    for (int i = 0; i < num_rows; i++)
    {
        residues[i] = true;
    }
    _Bool run = true;
    int iterations = 0;
    // Jacobi method
    while (run && (iterations < 5000))
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
            // Always calculate when async and removing MPI_Barrier
            if (residues[i])
            {
                x_local.vector[i] = 0;

                // Actual matrix-vector product
                for (int j = 0; j < dim_col; j++)
                {
                    if (i + first_row != j)
                    {
                        x_local.vector[i] += mtx.matrix[i][j] * x_global.vector[j];
                    }
                }
                // final calculations
                x_local.vector[i] = (1 / mtx.matrix[i][i + first_row]) * (vec.vector[i] - x_local.vector[i]);

                // Residual analysis
                // Do not update when async and removing MPI_Barrier
                if (fabs(x_local.vector[i] - x_global.vector[first_row + i]) < 1e-6 && (sync_type < 2))
                {
                    residues[i] = false;
                }
            }
        }

        // Test for whole convergence
        // Local "Should I continue to iterate?"
        bool local_continue = false;
        int i = 0;
        while (!local_continue && i < num_rows)
        {
            local_continue |= residues[i];
            i++;
        }

        // The root process receives a finishing ping from each other processes
        // and replies with a continue or stop order
        if (rank == 0)
        {
            bool global_continue = local_continue;
            // Receive all continues
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
            //Override the global_continue when without Barrier
            if (sync_type > 1)
            {
                global_continue = true;
            }

            // Send the global continue
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
            // Other processors just have to send their local continues
            // And get the answer from the root process
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
    // Finish properly all communications in async case
    if (sync_type > 1)
    {
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Cleanup memory
    free(residues);
    destruct_vector(&x_local);
    destruct_vector(&x_global);
    destruct_vector(&vec);
    destruct_matrix(&mtx);

    // Finish MPI
    MPI_Finalize();
    return 0;
}
