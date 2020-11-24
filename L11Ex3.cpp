#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
 
void printMatrix(int *matrix, int rows, int cols) {
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			printf("%3d", matrix[i * cols + j]);
		}
		printf("\n");
	}
}

int main(int argc, char* argv[])
{
    int size, myrank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	int rows, cols;
	rows = cols = size;
	int *matrix, *transposed;
	if (myrank == 0) {
		matrix = (int*)malloc(rows * cols * sizeof(int));
		transposed = (int*)malloc(rows * cols * sizeof(int));
		for (int i = 0; i < rows * cols; i++)
			matrix[i] = i;
		
		MPI_Datatype col, transpose;
		MPI_Type_vector(rows, 1, cols, MPI_INT, &col);
		MPI_Type_hvector(cols, 1, sizeof(int), col, &transpose);
		MPI_Type_commit(&transpose);

		MPI_Status status;
		MPI_Request req;

		MPI_Isend(&(matrix[0]), rows*cols, MPI_INT, 0, 1, MPI_COMM_WORLD, &req);
		MPI_Recv(&(transposed[0]), 1, transpose, 0, 1, MPI_COMM_WORLD, &status);

		MPI_Type_free(&col);
		MPI_Type_free(&transpose);

		printf("Original:\n");
		printMatrix(matrix,rows,cols);
		printf("Transposed\n");
		printMatrix(transposed,cols,rows);
	}
	
    MPI_Finalize();
 
    return 0;
}
