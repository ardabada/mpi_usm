#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

const int ROOT_RANK = 0; //ранк root процесса
const int TAG_DEFAULT = 111; //тэг для MPI_Send, MPI_Recv
const int TAG_MAX_VALUE = 222; //тэг для MPI_Send, MPI_Recv для передачи максимума
const int N_DIMS = 2; //размерность матрицы процессов
char* FILE_NAME = "array.dat"; //имя файла

int rows, cols; //количество строк и столбцов в исходной матрице

//создает новый тип, который определяет распределение матрицы на конкретный процесс
// [IN] grila_size	- размер матрицы процессов
// [IN] grila_rank	- ранк процесса из матрицы процессов, для которого следует расчитать разбиение матрицы
// [OUT] MPI_Darray	- результирующий тип, описывающий разбиение матрицы на конкретный процесс
void createDArray(int grila_size, int grila_rank, MPI_Datatype *MPI_Darray) {
	
	//используется функция MPI_Type_create_darray для распределения матрицы на процессы, находящихся в двумерной матрице процессов
	//https://hpc-forge.cineca.it/files/ScuolaCalcoloParallelo_WebDAV/public/anno-2012/MPI2-DataTypes.pdf
	//https://www.mcs.anl.gov/~thakur/sc17-mpi-tutorial/slides.pdf
	//https://www.hpci-office.jp/invite2/documents2/MPI-intermediate181206.pdf

	int array_of_gsizes[N_DIMS] = { rows, cols };
	int array_of_distribs[N_DIMS] = { MPI_DISTRIBUTE_CYCLIC, MPI_DISTRIBUTE_BLOCK }; 
	int array_of_dargs[N_DIMS] = { MPI_DISTRIBUTE_DFLT_DARG, MPI_DISTRIBUTE_DFLT_DARG };
	int array_of_psizes[N_DIMS] = { N_DIMS, grila_size / N_DIMS };
	MPI_Type_create_darray(grila_size, grila_rank, N_DIMS, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, MPI_ORDER_C, MPI_INT, MPI_Darray);
	MPI_Type_commit(MPI_Darray);
}

int main(int argc, char **argv) {
	srand(time(NULL));

	int rank, size, grila_rank, read_rank;
	int *grila_ranks;
	int *matrix;

    MPI_File file_out;
	MPI_Group MPI_GROUP_WORLD, grila_group, read_group;
	MPI_Comm grila_group_com, grila_com, read_group_com, read_com;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	//удаляем файл, если он есть
	MPI_File_open(MPI_COMM_WORLD, FILE_NAME, MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &file_out);
    MPI_File_close(&file_out);

	if (rank == ROOT_RANK) {
		printf("Enter number of rows:\n");
		scanf("%d", &rows);
		printf("Enter number of cols:\n");
		scanf("%d", &cols);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&rows, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&cols, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);

	//расчитываем необходимое количество процессов в матрице процессов
	int grila_group_size = ceil(cols / (float)N_DIMS) * N_DIMS;
	grila_ranks = (int*)malloc(grila_group_size * sizeof(int)); 
	int read_group_size = grila_group_size;

	if (rank == ROOT_RANK) {
		//определяем ранки процессов, которые будут включены в матрицу процессов
		printf("Grila size = %d\n", grila_group_size);
		for (int i = 0; i < grila_group_size; i++) {
			grila_ranks[i] = i;
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(grila_ranks, grila_group_size, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);

	//создаем группу, в которую входят процессы, которые будут включены в матрицу процессов для записи
    MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);
	MPI_Group_incl(MPI_GROUP_WORLD, grila_group_size, grila_ranks, &grila_group);
	MPI_Comm_create(MPI_COMM_WORLD, grila_group, &grila_group_com);
	
	//создаем группу, в которую входят процессы, которые будут включены в матрицу процессов для чтенеия
	MPI_Group_incl(MPI_GROUP_WORLD, grila_group_size, grila_ranks, &read_group);
	MPI_Comm_create(MPI_COMM_WORLD, read_group, &read_group_com);
	
	//определяем ранк текущего процесса в каждой группе
    MPI_Group_rank(grila_group, &grila_rank);
    MPI_Group_rank(read_group, &read_rank);

    if (grila_rank != MPI_UNDEFINED) {
		//если процесс принадлежит первой группе (группа для записи)
		int dim_size[N_DIMS] = { N_DIMS, grila_group_size / N_DIMS };
		int periods[N_DIMS] = { 1, 0 };
		int reorder = 1;
		int coords[N_DIMS];

		//создаем двумерную матрицу процессов
		MPI_Cart_create(grila_group_com, N_DIMS, dim_size, periods, reorder, &grila_com);
		//определяем координаты процесса
		MPI_Cart_coords(grila_com, rank, N_DIMS, coords);

		//открываем файл для записи
		MPI_File_open(grila_com, FILE_NAME, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &file_out);

		//определяем тип, который описывает все подматрицы, которые принадлежат данному процессу
		MPI_Datatype MPI_Darray;
		int darray_size;
		createDArray(grila_group_size, grila_rank, &MPI_Darray);
		MPI_Type_size(MPI_Darray, &darray_size);
		//количество элементов во всех подматрицах для данного процесса
		darray_size /= sizeof(int);
		int *buf = (int*)malloc(sizeof(int) * darray_size);
		//генерируем данные
		printf("Process %d (%d,%d) generates next data: ", grila_rank, coords[0], coords[1]);
		for (int i = 0; i < darray_size; i++) {
			buf[i] = grila_rank + rand() % 50;
			printf("%3d", buf[i]);
		}
		printf("\n");
		//записываем сгенерированные данные в файл
		MPI_File_set_view(file_out, 0, MPI_INT, MPI_Darray, "native", MPI_INFO_NULL);
		MPI_File_write(file_out, buf, darray_size, MPI_INT, MPI_STATUS_IGNORE);

		//закрываем файл
		MPI_File_close(&file_out);
	}

	if (read_rank != MPI_UNDEFINED) {
		//если процесс принадлежит второй группе (группа для чтения)
		int dim_size[N_DIMS] = { N_DIMS, read_group_size / N_DIMS };
		int periods[N_DIMS] = { 1, 0 };
		int reorder = 1;
		int coords[N_DIMS];

		//создаем двумерную матрицу процессов
		MPI_Cart_create(read_group_com, N_DIMS, dim_size, periods, reorder, &read_com);
		//определяем координаты процесса
		MPI_Cart_coords(read_com, rank, N_DIMS, coords);

		//открываем файл для чтения
		MPI_File_open(read_com, FILE_NAME, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &file_out);

		//определяем тип, который описывает все подматрицы, которые принадлежат данному процессу
		MPI_Datatype MPI_Darray;
		int darray_size;
		createDArray(read_group_size, read_rank, &MPI_Darray);
		MPI_Type_size(MPI_Darray, &darray_size);
		darray_size /= sizeof(int);

		//считываем все элементы всех подматриц, которые принадлежат данному процессу
		int *buf = (int*)malloc(sizeof(int) * darray_size);
		MPI_File_set_view(file_out, 0, MPI_INT, MPI_Darray, "native", MPI_INFO_NULL);
		MPI_File_read(file_out, buf, darray_size, MPI_INT, MPI_STATUS_IGNORE);

		//определяем максимальный элемент в подматрицах
		int max_value = buf[0];
		for (int i = 1; i < darray_size; i++) {
			if (buf[i] > max_value)
				max_value = buf[i];
		}
		printf("Process %d (%d,%d) max value = %d\n", read_rank, coords[0], coords[1], max_value);

		//отправляем считанные подматрицы процессу с ранком root, чтобы собрать всю матрицу
		MPI_Send(buf, darray_size, MPI_INT, ROOT_RANK, TAG_DEFAULT, read_com);
		//отправляем максимальное значение подматриц процессу с ранком root
		MPI_Send(&max_value, 1, MPI_INT, ROOT_RANK, TAG_MAX_VALUE, read_com);

		if (read_rank == ROOT_RANK) {
			int total = rows * cols;
			matrix = (int*)malloc(total * sizeof(int));

			//все максимальные элементы, полученные от других процессов
			int *maxs = (int*)malloc(grila_group_size * sizeof(int));
			
			for (int i = 0; i < grila_group_size; i++) {
				int recv_rank = grila_ranks[i];
				//определяем тип, который описывает все подматрицы, которые принадлежат процессу с ранком recv_rank
				MPI_Datatype MPI_Darray_sub;
				createDArray(read_group_size, recv_rank, &MPI_Darray_sub);
				//получаем все подматрицы от процесса с ранком recv_rank
				MPI_Recv(matrix, 1, MPI_Darray_sub, recv_rank, TAG_DEFAULT, read_com, MPI_STATUS_IGNORE);
				//получаем максимальное значение от процесса с ранком recv_rank
				MPI_Recv(maxs + recv_rank, 1, MPI_INT, recv_rank, TAG_MAX_VALUE, read_com, MPI_STATUS_IGNORE);
			}

			//находим максимальное значение в матрице
			int max_value = maxs[0];
			for (int i = 1; i < grila_group_size; i++) {
				if (maxs[i] > max_value)
					max_value = maxs[i];
			}

			//выводим матрицу и максимальное значение
			printf("== MATRIX (max value = %d) ==\n", max_value);
			for (int i = 0; i < rows; i++) {
				for (int j = 0; j < cols; j++)
					printf("%3d", matrix[i * cols + j]);
				printf("\n");
			}
		}

		//закрываем файл
		MPI_File_close(&file_out);
	}

	if (read_rank == MPI_UNDEFINED && grila_rank == MPI_UNDEFINED)
		printf("Process %d is idle\n", rank);

	MPI_Finalize();
	return 0;
}