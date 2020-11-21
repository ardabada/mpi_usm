#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
 
void printMatrix(int *matrix, int size) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			printf("%3d", matrix[i * size + j]);
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

	int m_size;
	int *matrix;
	if (myrank == 0) {
		printf("Input matrix size:\n");
		scanf("%d", &m_size);

		matrix = (int*)malloc(m_size * m_size * sizeof(int));
		for (int i = 0; i < m_size * m_size; i++)
			matrix[i] = i;

		printMatrix(matrix, m_size);

		int lengths[m_size], displacements[m_size];
		for (int i = 0; i < m_size; i++) {
			lengths[i] = 1;
			displacements[i] = (m_size+1)*i;
		}
		MPI_Datatype main_diagonal_type;
		MPI_Type_indexed(m_size, lengths, displacements, MPI_INT, &main_diagonal_type);
		MPI_Type_commit(&main_diagonal_type);

		for (int i = 1; i < size; i++) {
			MPI_Send(matrix, 1, main_diagonal_type, i, 111, MPI_COMM_WORLD);
		}
	}

	MPI_Bcast(&m_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	if (myrank != 0) {
		int buf[m_size];
		MPI_Recv(&buf, m_size, MPI_INT, 0, 111, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Rank %d received main diagonal: ", myrank);
		for (int i = 0; i < m_size; i++) {
			printf("%3d", buf[i]);
		}
		printf("\n");
	}
	
    MPI_Finalize();
 
    return 0;
}