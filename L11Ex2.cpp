#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int STRUCT_N_FIELDS = 2;
struct ProcessData {
	char name[MPI_MAX_PROCESSOR_NAME];
	int rank;
};

int main(int argc, char **argv) {
	int rank, size, namelen;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	MPI_Datatype MPI_Proc_Data;
	ProcessData data;
	int blens[STRUCT_N_FIELDS] = { MPI_MAX_PROCESSOR_NAME, 1 };
	MPI_Aint displ[STRUCT_N_FIELDS] = { (MPI_Aint)&data.name - (MPI_Aint)&data, (MPI_Aint)&data.rank - (MPI_Aint)&data };
	MPI_Datatype types[STRUCT_N_FIELDS] = { MPI_CHAR, MPI_INT };
	MPI_Type_struct(STRUCT_N_FIELDS, blens, displ, types, &MPI_Proc_Data);
	MPI_Type_commit(&MPI_Proc_Data);

	MPI_Get_processor_name(data.name,&namelen);
	data.rank = rank;

	MPI_Send(&data, 1, MPI_Proc_Data, 0, 111, MPI_COMM_WORLD);

	if (rank == 0) {
		printf("== RESULTS ==\n");
		ProcessData recv_data;
		for (int i = 0; i < size; i++) {
			MPI_Recv(&recv_data, 1, MPI_Proc_Data, i, 111, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printf("Recv: rank %d on %s\n", recv_data.rank, recv_data.name);
		}
	}

	MPI_Type_free(&MPI_Proc_Data);
	MPI_Finalize();
	return 0;
}