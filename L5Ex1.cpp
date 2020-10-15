#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int *create_rand_nums(int num_elements) {
	int *rand_nums = (int *)malloc(sizeof(int) * num_elements);
	for (int i = 0; i < num_elements; i++) {
		rand_nums[i] = (rand() % 20 + 1);
	}
	return rand_nums;
}

int main(int argc, char** argv) {
	int rank, size;
	MPI_Status status; 
	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	int *rand_nums = create_rand_nums(size);
	int local_sum = 0;
	for (int i = 0; i < size; i++)
		local_sum += rand_nums[i];
	
	printf("Rank %d sum %d\n", rank, local_sum);

	int global_sum;
	//MPI_Allreduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	MPI_Reduce(&local_sum, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Bcast(&global_sum, 1, MPI_INT, 0, MPI_COMM_WORLD);

	printf("Local %d total %d\n", local_sum, global_sum);

	free(rand_nums);

	MPI_Finalize();
	return 0;
}