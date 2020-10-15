#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	int rank, size;
	MPI_Status status; 
	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	const int nums = 10;
	int data[nums] = { 5, 7, 2, 5, 11, 21, 23, -3, 3, 1};

	if (rank == 0) {
		for (int i = 0; i < nums; i++)
			printf("%d ", data[i]);
	}

	int elements_per_proc = nums / size;
	int subarr[elements_per_proc];
	int partres_min[elements_per_proc];
	int partres_max[elements_per_proc];

	MPI_Scatter(data, elements_per_proc, MPI_INT, subarr, elements_per_proc, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank % 2 == 0)
		MPI_Reduce(subarr, partres_max, elements_per_proc, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
	else
		MPI_Reduce(subarr, partres_min, elements_per_proc, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
	//min не работает должным образом, так как MPI_Reduce - коллективная функция, ее должны выполнить все процессы коммуникаторов. в данном случае она блокируется, так как она должна выполняться всеми процессами

	if (rank == 0) {
		printf("\nmax = %d, min = %d\n", partres_max[0], partres_min[0]);
	}

	MPI_Finalize();
	return 0;
}