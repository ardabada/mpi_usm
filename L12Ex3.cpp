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

	int rows, cols;

	if (myrank == 0) {
		printf("Enter number of rows:\n");
        scanf("%d", &rows);
        printf("Enter number of collumns:\n");
        scanf("%d", &cols);
	}
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Type_contiguous(cols, MPI_INT, &MPI_Row);
	MPI_Type_commit(&MPI_Row);

	if (myrank == 0) {
		int matrix[rows][cols];
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++)
				matrix[i][j] = 10 * i + j;
		}

		MPI_File_write(f_out, &matrix, rows, MPI_Row, &state);
		MPI_Get_count(&state, MPI_Row, &count);
		printf("== Root process wrote %d lines to array.dat ==\n", count);
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++)
				printf("%4d", matrix[i][j]);
			printf("\n");
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);

	int rows_per_process = rows / size;
	int remaining_rows = rows % size;

	int rows_to_read = rows_per_process + (myrank == size - 1 ? remaining_rows : 0);

	MPI_Aint rowsize;
    MPI_Type_extent(MPI_Row, &rowsize);

	int buff[rows_to_read][cols];
	MPI_File_set_view(f_out, rowsize * rows_per_process * myrank, MPI_Row, MPI_Row, "native", MPI_INFO_NULL);
	MPI_File_read(f_out, &buff, rows_to_read, MPI_Row, &state);
	MPI_Get_count(&state, MPI_Row, &count);
	printf("== Rank %d read %d lines ==\n", myrank, count);
	for (int i = 0; i < rows_to_read; i++) {
		printf("Rank %d read line %d/%d: ", myrank, i, rows_to_read);
		for (int j = 0; j < cols; j++)
			printf("%4d", buff[i][j]);
		printf("\n");
	}
	printf("\n");



	MPI_Type_free(&MPI_Row);
	MPI_File_close(&f_out);
    MPI_Finalize();
	return 0;
}