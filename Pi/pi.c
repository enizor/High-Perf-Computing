#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// Pi constant, double precision
#define M_PI 3.14159265358979323846

float f(float x) {
    /* Evaluate f(x) */
    return 4 / (1 + x * x);
}

float trapeze_sequential(float a, float b, int n) {
    float integral = (f(b) - f(a)) / 2; // result
    float h = (b - a) / n; // step
    for (int i = 1; i < n; i++) {
        integral += (f(a + i * h));
    }
    integral *= h;

    return integral;
}

int main(int argc, char *argv[]) {
    float a = 0, b = 1;    // points gauche et droits
    int n = 10000;             // nombre de trapèzes, divisible par size !!!
    float h = (b - a) / n; // pas

    int rank, size;
    MPI_Init(&argc, &argv);               /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size); /* get number of processes */

    // Usage of arguments
    // n will be the first argument, defaults to 10 000
    if (argc > 0) {
        n = atoi(argv[1]);
    }
    if (rank == 0) {
        float integral, sub_integral; // result
        float h = (b - a) / n;        // step
        // Evaluate the first part of the integral
        integral = trapeze_sequential(a, a + (b - a) / size, n / size);
        // Sum with the other sub integrals
        for (int k = 1; k < size; k++)
        {
            printf("> Waiting for process %d to compute...\n", k);
            MPI_Recv(&sub_integral, 1, MPI_FLOAT, k, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE);
            integral += sub_integral;
        }
        // Printing the results
        printf("> Result: \n%f\n", integral);
        printf("> Error (%%): \n%f", 100*(M_PI - integral)/M_PI);
    } else {
        // Other process use directly the sequential computation
        printf("> Calculation from process %d\n", rank);
        float local_a = a + rank * (b - a) / size,
               local_b = a + (rank + 1) * (b - a) / size;

        // Evaluate the sub integral
        float sub_integral =
            trapeze_sequential(local_a, local_b, n / size);
        // Send the result to the main process
        MPI_Send(&sub_integral, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}
