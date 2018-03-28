#ifndef __VECTOR_H__
#define __VECTOR_H__

struct Vector
{
    unsigned int dim;
    double *vector;
};

// Empty vector creation
struct Vector create_vector(int dim);

// Vector destruction
void destruct_vector(struct Vector *vec);

struct Vector load_vector(char* path, int first_row, int num_rows);

void print_vector(struct Vector vec);

#endif
