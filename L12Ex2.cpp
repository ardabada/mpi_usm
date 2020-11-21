#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int size, myrank, count;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	MPI_Status state;
	MPI_Datatype MPI_Row;
	MPI_File f_out;
	MPI_File_open(MPI_COMM_WORLD, "array.dat", MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &f_out);
	MPI_File_close(&f_out);
	MPI_File_open(MPI_COMM_WORLD, "array.dat", MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &f_out);
	MPI_Type_contiguous(size, MPI_INT, &MPI_Row);
    MPI_Type_commit(&MPI_Row);

	if (myrank == 0) {
		int matrix[size][size];
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++)
				matrix[i][j] = 10 * i + j;
		}

		MPI_File_write(f_out, &matrix, size, MPI_Row, &state);
		MPI_Get_count(&state, MPI_Row, &count);
		printf("== Root process wrote %d lines to array.dat ==\n", count);
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++)
				printf("%4d", matrix[i][j]);
			printf("\n");
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Datatype rank_ftype;
	int blengths[2] = { 0, 1 };
	MPI_Datatype types[2] = { MPI_Row, MPI_Row };
	MPI_Aint rowsize;
    MPI_Type_extent(MPI_Row, &rowsize);
	MPI_Aint disps[2] = { 0, myrank * rowsize };
	MPI_Type_struct(2, blengths, disps, types, &rank_ftype);
    MPI_Type_commit(&rank_ftype);

	int buff[size];
	MPI_File_set_view(f_out, 0, MPI_Row, rank_ftype, "native", MPI_INFO_NULL);
	MPI_File_read(f_out, &buff, 1, MPI_Row, &state);
	MPI_Get_count(&state, MPI_Row, &count);
	printf("Rank %d read: ", myrank);
	for (int i = 0; i < size; i++)
		printf("%4d", buff[i]);
	printf("\n");

	
	MPI_Type_free(&MPI_Row);
	MPI_Type_free(&rank_ftype);
	MPI_File_close(&f_out);
    MPI_Finalize();
	return 0;
}