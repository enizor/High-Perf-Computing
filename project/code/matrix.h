#ifndef __MATRIX_H__
#define __MATRIX_H__

struct Matrix
{
    unsigned int dim_row;
    unsigned int dim_col;
    double **matrix;
};

// Empty matrix creation
struct Matrix create_matrix(int dim_row, int dim_col);

// Matrix destruction
void destruct_matrix(struct Matrix *mtx);

struct Matrix load_matrix(char *path, int first_row, int num_rows);

void print_matrix(struct Matrix mtx);

#endif
