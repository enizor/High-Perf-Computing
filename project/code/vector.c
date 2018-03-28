#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

struct Vector create_vector(int dim)
{
    struct Vector vec;
    vec.dim= dim;

    // Vector initialisation
    vec.vector = calloc(dim, sizeof(double));

    return vec;
}

void destruct_vector(struct Vector *vec)
{
    free(vec->vector);
    vec->vector = NULL;
}

struct Vector load_vector(char *path, int first_row, int num_rows)
{
    FILE *fp;
    fp = fopen(path, "r");

    if (fp == NULL)
    {
        printf("Error while opening file %s", path);
        exit(1);
    }

    int dim;

    fscanf(fp, "%d", &dim);

    struct Vector vec = create_vector(num_rows);

    // skip the first rows
    for (int i = 0; i < first_row+1; i++)
    {
        char c;
        do
        {
            c = fgetc(fp);
        }
        while (c != '\n');
    }

    // read the required rows and fill the vector
    for (int i = 0; i < num_rows; i++)
    {
      fscanf(fp, "%lf", &vec.vector[i]);
    }

    fclose(fp);

    return vec;
}

void print_vector(struct Vector vec)
{
    printf("[ ");
    for (int i = 0; i < vec.dim; i++)
    {
      printf("%.3e ", vec.vector[i]);
    }
    printf("]\n");
}
