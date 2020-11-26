#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int ROOT_RANK = 0; //ранк root процесса
const int TAG = 111; //тэг для MPI_Send, MPI_Recv
const int N_DIMS = 2; //размерность матрицы процессов

int 
	//размерность исходной матрицы
	rows, 
	cols,
	//размерность одного блока разбиения
	block_rows,
	block_cols,
	//число блоков, на которые будет разбита исходная матрица 
	total_block_rows, 
	total_block_cols;

//описывает тип MPI для разбиения подматрицы
struct BBlockInfo {
	MPI_Datatype MPI_B; //тип MPI, описывающий блок матрицы
	int B_rows; //количество строк в разделенном блоке
	int B_cols; //количество столбцов в разделенном блоке
};

//создает тип, описывающий разбиение исходной матрицы для конкретного блока матрицы разбиения
// [IN] block_i	- индекс строки матрицы разбиения
// [IN] block_j	- индекс колонки матрицы разбиения
// [OUT] block	- тип, описывающий разбиение
void createSubBType(int block_i, int block_j, BBlockInfo *block) {
	//начальные индексы блока в исходной матрице
	int start_row = block_i * block_rows,
		start_col =  block_j * block_cols;
	//размерность блока, который получится
	int size_row = block_rows,
		size_col = block_cols;
	//индексы конца блока в исходной матрицы
	int row_end = start_row + size_row,
		col_end = start_col + size_col;
	//если блок с учетом смещения (начальные индексы) больше размерности исходной матрицы
	//то устанавливаем максимальную допустимую размерность для блока (с начального индекса до конца измерения исходной матрицы)
	if (row_end > rows)
		size_row = rows - start_row;
	if (col_end > cols)
		size_col = cols - start_col;

	int starts[N_DIMS] = { start_row, start_col };
	int bigsizes[N_DIMS] = { rows, cols };
	int subsizes[N_DIMS] = { size_row, size_col };

	//создаем тип MPI, описывающий блок как подматрицу
	MPI_Datatype b_type;
	MPI_Type_create_subarray(N_DIMS, bigsizes, subsizes, starts, MPI_ORDER_C, MPI_INT, &b_type);
	MPI_Type_commit(&b_type);

	block->MPI_B = b_type;
	block->B_rows = size_row;
	block->B_cols = size_col;
}

int main(int argc, char **argv) {
	srand(time(NULL));

	int rank, size, grila_rank;
	int *grila_ranks;
	int *matrix;

	MPI_Group MPI_GROUP_WORLD, grila_group;
	MPI_Comm grila_group_com, grila_com;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int grila_group_size = size - (size % N_DIMS); //размерность группы матрицы процессов (количество процесс, входящих в эту группу)
	int grila_cols = grila_group_size / N_DIMS; //количество колонок в матрице процессов
	grila_ranks = (int*)malloc(grila_group_size * sizeof(int)); //выделяем память для определения ранков процессов матрицы процессов

	if (rank == ROOT_RANK) {
		//ввод размерности исходной матрицы
		printf("Enter number of rows:\n");
		scanf("%d", &rows);
		printf("Enter number of cols:\n");
		scanf("%d", &cols);

		//ввод размерности блоков разбиения
		printf("Enter number of block rows:\n");
		scanf("%d", &block_rows);
		printf("Enter number of block cols:\n");
		scanf("%d", &block_cols);

		// rows = cols = 10;
		// block_rows = block_cols = 3;

		//инициализируем исходную матрицу
		int total = rows * cols;
		matrix = (int*)malloc(total * sizeof(int));
		for (int i = 0; i < total; i++) {
			matrix[i] = i;
		}
		
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++)
				printf("%3d", matrix[i * cols + j]);
			printf("\n");
		}

		//определяем ранки процессов, которые будут входить в группу, образующую матрицу процессов
		printf("Grila size = %d\n", grila_group_size);
		for (int i = 0; i < grila_group_size; i++) {
			grila_ranks[i] = i;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	//передаем всем процессам введенные данные и ранки процессов для группы матрицы процессов
	MPI_Bcast(&rows, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&cols, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&block_rows, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&block_cols, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Bcast(grila_ranks, grila_group_size, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);

	//общее число строк и столбцов в матрице разбиения
	total_block_rows = rows / block_rows + (rows % block_rows == 0 ? 0 : 1); //= ceil(rows / block_rows)
	total_block_cols = cols / block_cols + (cols % block_cols == 0 ? 0 : 1); //= ceil(cols / block_cols)

	//создаем группу, состоящую из процессов, которые будут образовывать матрицу процессов
    MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);
	MPI_Group_incl(MPI_GROUP_WORLD, grila_group_size, grila_ranks, &grila_group);
	//создаем коммуникатор для этой группы
	MPI_Comm_create(MPI_COMM_WORLD, grila_group, &grila_group_com);
	//определяем ранк процесса в этой группе
    MPI_Group_rank(grila_group, &grila_rank);

	//если процесс принадлежит этой группе
    if (grila_rank != MPI_UNDEFINED) {

		int dim_size[N_DIMS] = { N_DIMS, grila_group_size / N_DIMS };
		int periods[N_DIMS] = { 1, 0 };
		int reorder = 1;
		int coords[N_DIMS];
		//создаем двумерную матрицу процессов
		MPI_Cart_create(grila_group_com, N_DIMS, dim_size, periods, reorder, &grila_com);
		//определяем координаты процесса
		MPI_Cart_coords(grila_com, rank, N_DIMS, coords);

		if (rank == ROOT_RANK) {
			BBlockInfo b_type;
			int grila_coords[N_DIMS] = { 0, 0 };
			int calculated_rank;
			//пробегаем по всей матрице разбиения
			//матрица разбиения не создается, но зная ее размерность, мы можем определить конкретный блок на любой позиции
			for (int i = 0; i < total_block_rows; i++) {
				for (int j = 0; j < total_block_cols; j++) {
					createSubBType(i, j, &b_type); //определяем блок по координатам i,j в матрице разбиения

					//расчитываем координаты процесса (из матрицы процессов), которому надо передать блок
					grila_coords[0] = i % N_DIMS;
					grila_coords[1] = j % grila_cols;
					//находим ранк процесса по его координатам
					MPI_Cart_rank(grila_com, grila_coords, &calculated_rank);

					//отправляем блок процессу из матрицы процессов
					MPI_Send(&matrix[0], 1, b_type.MPI_B, calculated_rank, TAG, grila_com);
					MPI_Type_free(&b_type.MPI_B);
				}
			}
		}

		//шаг по колонкам (horizontal) и по строкам (vertial) для определения следующего блока, который должен принять данный процесс
		int horizontal_step = grila_cols, vertical_step = N_DIMS;

		BBlockInfo b_type;
		int *buf;
		int total_received = 0;
		//пробегаем по нужным блокам (те, которые надо принять этим процессов) из матрицы разбиения
		for (int current_row = coords[0]; current_row < total_block_rows; current_row += vertical_step) {
			for (int current_col = coords[1]; current_col < total_block_cols; current_col += horizontal_step) {
				createSubBType(current_row, current_col, &b_type); //определяем размерность блока разбиения на текущей позиции матрицы разбиения
				buf = (int*)malloc(sizeof(int) * b_type.B_rows * b_type.B_cols); //выделяем память под блок

				//получаем блок из процесса с ранком root
				MPI_Recv(buf, b_type.B_rows * b_type.B_cols, MPI_INT, ROOT_RANK, TAG, grila_com, MPI_STATUS_IGNORE);
				total_received++;

				//выводим полученный блок
				printf("Process %d (%d,%d) received:\n", grila_rank, coords[0], coords[1]);
				for (int _i = 0; _i < b_type.B_rows; _i++) {
					for (int _j = 0; _j < b_type.B_cols; _j++) {
						printf("%3d", buf[_i * b_type.B_cols + _j]);
					}
					printf("\n");
				}
				
				MPI_Type_free(&b_type.MPI_B);
				free(buf);
			}
		}
		printf("Process %d received total %d blocks\n", grila_rank, total_received);
	}
	else {
		printf("Process %d is idle\n", rank);
	}

	MPI_Finalize();
	return 0;
}