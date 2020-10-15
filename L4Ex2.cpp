#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	int rank, number, namelen, size;
	MPI_Status status; 
	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Get_processor_name(processor_name,&namelen);
	
	int *source_matrix, *result_matrix, *test_matrix;
	int rows[size];
	if (rank == 0) {
		source_matrix = (int*)malloc(size * size * sizeof(int));
		result_matrix = (int*)malloc(size * size * sizeof(int));
		test_matrix = (int*)malloc(size * size * sizeof(int));

		printf("Ishodnaya matritsa:\n");
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				int index = i * size + j;
				source_matrix[index] = rand() % 20 + 1;
				printf("%5d", source_matrix[index]);
			}
			printf("\n");
		}
		printf("\n");
		MPI_Barrier(MPI_COMM_WORLD);
		
		for (int i = 0; i < size; i++) {
			MPI_Send(&source_matrix[i*size], size, MPI_INT, i, 10, MPI_COMM_WORLD);
		}
	}
	else MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_Recv(rows, size, MPI_INT, 0, 10, MPI_COMM_WORLD, &status);
	
    printf("MPI_Send (rank %d): ", rank);
    for (int i = 0; i < size; ++i)
        printf("%3d ", rows[i]);
    printf("\n");
    MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_Gather(rows, size, MPI_INT, result_matrix, size, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0) {
        printf("MPI_Gather:\n");
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++)
                printf("%5d", source_matrix[i * size + j]);
            printf("\n");
        }
        printf("\n");
        MPI_Barrier(MPI_COMM_WORLD);
		
    }
    else
        MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_Finalize();
	return 0;
}