#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define INCLUDE_DIAGONAL 1
#define lower_triangle_size_with_diagonal(m_size) (m_size * m_size - m_size) / 2 + m_size
#define lower_triangle_size_without_diagonal(m_size) (m_size * m_size - m_size) / 2
#define lower_tiangle_size(m_size) INCLUDE_DIAGONAL ? lower_triangle_size_with_diagonal(m_size) : lower_triangle_size_without_diagonal(m_size)

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
			lengths[i] = INCLUDE_DIAGONAL ? i + 1 : i;
			displacements[i] = i*m_size;
		}
		
		MPI_Datatype lower_triangle_type;
		MPI_Type_indexed(m_size, lengths, displacements, MPI_INT, &lower_triangle_type);
		MPI_Type_commit(&lower_triangle_type);

		for (int i = 1; i < size; i++) {
			MPI_Send(matrix, 1, lower_triangle_type, i, 111, MPI_COMM_WORLD);
		}
	}

	MPI_Bcast(&m_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (myrank != 0) {
		int recv_count = lower_tiangle_size(m_size);
		int buf[recv_count];
		MPI_Recv(&buf, recv_count, MPI_INT, 0, 111, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Rank %d received lower triangle (%s diagonal): %d elements: ", myrank, INCLUDE_DIAGONAL ? "with" : "without", recv_count);
		for (int i = 0; i < recv_count; i++) {
			printf("%3d", buf[i]);
		}
		printf("\n");
	}
	
    MPI_Finalize();
 
    return 0;
}