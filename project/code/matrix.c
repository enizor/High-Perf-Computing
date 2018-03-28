#include <stdio.h>
#include <stdlib.h>

#include "matrix.h"

struct Matrix create_matrix(int dim_row, int dim_col)
{
    struct Matrix mtx;
    mtx.dim_row = dim_row;
    mtx.dim_col = dim_col;

    // Matrix initialisation
    mtx.matrix = malloc(dim_row * sizeof(double *));

    for (int i = 0; i < dim_row; i++)
    {
        mtx.matrix[i] = calloc(dim_col, sizeof(double));
    }

    return mtx;
}

void destruct_matrix(struct Matrix *mtx)
{
    for (int i = 0; i < mtx->dim_row; ++i)
    {
        free(mtx->matrix[i]);
        mtx->matrix[i] = NULL;
    }

    free(mtx->matrix);
    mtx->matrix = NULL;
}

struct Matrix load_matrix(char *path, int first_row, int num_rows)
{
    FILE *fp;
    fp = fopen(path, "r");

    if (fp == NULL)
    {
        printf("Error while opening file %s", path);
        exit(1);
    }

    int dim_row, dim_col;

    fscanf(fp, "%d %d", &dim_row, &dim_col);

    struct Matrix mtx = create_matrix(num_rows, dim_col);

    // skip the first rows
    for (int i = 0; i < first_row + 1; i++)
    {
        char c;
        do
        {
            c = fgetc(fp);
        } while (c != '\n');
    }

    // read the required rows and fill the matrix
    for (int i = 0; i < num_rows; i++)
    {
        for (int j = 0; j < dim_col; j++)
        {
            fscanf(fp, "%lf", &mtx.matrix[i][j]);
        }
    }

    fclose(fp);

    return mtx;
}

void print_matrix(struct Matrix mtx)
{
    for (int i = 0; i < mtx.dim_row; i++)
    {
        for (int j = 0; j < mtx.dim_col; j++)
        {
            printf("%.3e ", mtx.matrix[i][j]);
        }
        printf("\n");
    }
}
