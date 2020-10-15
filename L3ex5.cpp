#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	int rank, number, namelen, size;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	char processor_name[MPI_MAX_PROCESSOR_NAME];
	MPI_Get_processor_name(processor_name, &namelen);

	if (rank == 0)
	{
		for (int i = 1; i < size; i++)
		{
			MPI_Sendrecv(&rank, 1, MPI_INT, i, 1, &number, 0, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
		}
	}
	else
	{
		MPI_Sendrecv(NULL, 0, MPI_DOUBLE, 0, 1,
					 &number, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	printf("Procesu cu rancul %d al nodului '%s' a primit valoarea %d de la procesul cu rancul %d\n", rank, processor_name, number, number);

	MPI_Finalize();
	return 0;
}