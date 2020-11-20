#include <stdio.h>
#include <iostream>
#include <mpi.h>
using namespace std;
int main(int argc, char* argv[])
{
	int size, rank, t, namelen, rank_buffer;
	int incep = 8;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Status status;
	MPI_Win window;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(processor_name, &namelen);

	rank_buffer = rank;

	MPI_Win_create(&rank_buffer, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window);
	MPI_Win_fence(0, window);

	int fetched_rank;
	MPI_Get(&fetched_rank, 1, MPI_INT, (rank + size - 1) % size, 0, 1, MPI_INT, window);
	MPI_Win_fence(0, window);

	printf("Procesu cu rancul %d al nodului '%s' a primit valoarea %d de la procesul cu rancul %d\n", rank, processor_name, fetched_rank, fetched_rank);

	MPI_Win_free(&window);
	MPI_Finalize();
	return 0;
}
