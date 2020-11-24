#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

struct RowsInfo {
	int process_rank;
	int displacement;
	int recv_count;
	bool can_receive;
};

void calculateRows(int rows, int cols, int size, int *rows_per_process, int *remaining_rows, int *items_per_process, int *items_per_last_process) {
	*rows_per_process = rows / size;
	if (*rows_per_process == 0)
		*rows_per_process = 1;
	*remaining_rows = rows % size;
	*items_per_process = *rows_per_process * cols;
	*items_per_last_process = (rows - (*rows_per_process * (size - 1))) * cols;
}
void calculateSendRows(int rows, int cols, int rank, int size, RowsInfo *result) {
	int rows_per_process, remaining_rows, items_per_process, items_per_last_process;
	calculateRows(rows, cols, size, &rows_per_process, &remaining_rows, &items_per_process, &items_per_last_process);

	int total = rows * cols;
	result->displacement = items_per_process * rank;
	result->recv_count = items_per_process;
	if (remaining_rows != 0 && rank == size - 1)
		result->recv_count = items_per_last_process;
	result->can_receive = result->displacement <= total && result->displacement + result->recv_count <= total && result->recv_count != 0;
}

int main(int argc, char **argv) {
	srand(time(NULL));

	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int rows, cols;
	int *matrix;
	if (rank == 0) {
		printf("Enter number of rows\n");
		scanf("%d", &rows);
		printf("Enter number of collumns\n");
		scanf("%d", &cols);
	}
	
	MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	int total = rows * cols;
	
	int rows_per_process, remaining_rows, items_per_process, items_per_last_process;
	calculateRows(rows, cols, size, &rows_per_process, &remaining_rows, &items_per_process, &items_per_last_process);

	MPI_Datatype my_row_type, my_last_row_type;
	MPI_Type_contiguous(items_per_process, MPI_INT, &my_row_type);
	MPI_Type_commit(&my_row_type);
	MPI_Type_contiguous(items_per_last_process, MPI_INT, &my_last_row_type);
	MPI_Type_commit(&my_last_row_type);
	
	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0) {
		matrix = (int*)malloc(total * sizeof(int));
		
		for (int i = 0; i < total; i++)
			matrix[i] = i;

		printf("== MATRIX ==\n");
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++)
				printf("%5d", matrix[i * cols + j]);
			printf("\n");
		}

		for (int i = 0; i < size; i++) {
			RowsInfo info;
			calculateSendRows(rows, cols, i, size, &info);
			if (info.can_receive) {
				printf("Sending data (%d-%d) to process with rank %d\n", info.displacement, info.displacement + info.recv_count, i);
				MPI_Send(matrix + info.displacement, 1, i == size - 1 ? my_last_row_type : my_row_type, i, 111, MPI_COMM_WORLD);
			}
			else {
				printf("No data to send to process with rank %d\n", i);
			}
		}
		
	}

	
	RowsInfo info;
	calculateSendRows(rows, cols, rank, size, &info);
	if (info.can_receive) {
		int buf[info.recv_count];
		MPI_Recv(buf, 1, rank == size - 1 ? my_last_row_type : my_row_type, 0, 111, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Rank %d received: ", rank);
		for (int i = 0; i < info.recv_count; i++)
			printf("%5d", buf[i]);
		printf("\n");
	}
	else 
		printf("Rank %d did not receive any rows\n", rank);
		

	MPI_Type_free(&my_row_type);
	MPI_Type_free(&my_last_row_type);
	MPI_Finalize();
	return 0;
}
