#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv) {
	srand(time(NULL));

	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Datatype my_row_type;
	MPI_Type_contiguous(size, MPI_DOUBLE, &my_row_type);
	MPI_Type_commit(&my_row_type);

	double *matrix = (double*)malloc(size * size * sizeof(double));
	double buf[size];
	if (rank == 0) {
		for (int i = 0; i < size * size; i++)
			matrix[i] = rand() / 1000000000.0;

		printf("== MATRIX ==\n");
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++)
				printf("%.2f ", matrix[i * size + j]);
			printf("\n");
		}

		for (int i = 0; i < size; i++)
			MPI_Send(matrix + i * size, 1, my_row_type, i, 10, MPI_COMM_WORLD);
	}

	MPI_Recv(buf, 1, my_row_type, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	printf("Rank %d recv: ", rank);
	for (int i = 0; i < size; i++)
		printf("%.2f ", buf[i]);
	printf("\n");

	MPI_Type_free(&my_row_type);
	MPI_Finalize();
	return 0;
}