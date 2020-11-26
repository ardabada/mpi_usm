#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <vector>

using namespace std;

const int ROOT_RANK = 0; //ранк root процесса
const int TAG = 111; //тэг для MPI_Send, MPI_Recv
const int USE_STATIC_MATRIX = 0; //указывает, если следует использовать статические данные (0 - нет, 1 - да)

//описывает разбиение матрицы для каждого процесса
//учитывается тот факт, что какой-то процесс может остаться без строк для матрицы (в случае, если rows < size)
//а также тот факт, чтобы все строки матрицы были распределены по процессам
struct RowsInfo {
	int process_rank; //ранк процесса
	int displacement; //смещения
	int recv_count; //количество отправляемых-получаемых данных
	bool can_receive; //указывает, если процесс с данным ранком может отправить-получить данные (то есть для данного процесса выделена хотя бы одна строка матрицы)
};

//описывает результат нахождения максимальных элементов в строках матрицы, переданных процессу
struct MaxReport {
	int max_items; //количество найденных максимальных элементов во всех строках которые были переданы процессу
	int rank; //ранк процесса
};

//описывает координаты элемента матрицы (строка и столбец)
struct Coords {
	Coords() { }
	Coords(int r, int c) {
		this->row = r;
		this->col = c;
	}

	int row; //строка
	int col; //столбец

	//меняет координаты строки и столбца местами
	void swap() {
		int t = this->row;
		this->row = this->col;
		this->col = t;
	}
};

//заполняет матрицы A и B статическими данными. матрицы должны быть размерностью 6 строк на 6 столбцов
void fillStaticData(double *A, double *B) {
	A[0]=400; A[1]=0; A[2]=0; A[3]=0; A[4]=0; A[5]=0; A[6]=300; A[7]=300; 
	A[8]=0; A[9]=0; A[10]=0; A[11]=0; A[12]=200; A[13]=200; A[14]=200; 
	A[15]=0; A[16]=0; A[17]=0; A[18]=100; A[19]=100; A[20]=100; A[21]=100; 
	A[22]=0; A[23]=0; A[24]=0; A[25]=0; A[26]=0; A[27]=0; A[28]=0; 
	A[29]=0; A[30]=-100; A[31]=-100; A[32]=-100; A[33]=-100; A[34]=-100; A[35]=-100; 

	B[0]=0; B[1]=200; B[2]=100; B[3]=0; B[4]=-100; B[5]=-200; B[6]=0; B[7]=0; 
	B[8]=100; B[9]=0; B[10]=-100; B[11]=-200; B[12]=0; B[13]=0; B[14]=0; 
	B[15]=0; B[16]=-100; B[17]=-200; B[18]=0; B[19]=0; B[20]=0; B[21]=0; 
	B[22]=-100; B[23]=-200; B[24]=0; B[25]=0; B[26]=0; B[27]=0; B[28]=0; 
	B[29]=-200; B[30]=0; B[31]=0; B[32]=0; B[33]=0; B[34]=0; B[35]=0;
}

//транспонирует матрицу и возвращает указатель на транспонированную матрицу
double* transposeMatrix(double *matrix, int n_rows, int n_cols) {
	double* result = (double*)malloc(n_rows * n_cols * sizeof(double));
	if (result == NULL)
		throw "Out of memory";
	for (int i = 0; i < n_rows; i++) {
		for (int j = 0; j < n_cols; j++) {
			result[j * n_rows + i] = matrix[i * n_cols + j];
		}
	}
	return result;
}

//рассчитывает количество строк матрицы, которые следует передать процессам
// [IN] rows					- количество строк в матрице
// [IN] cols					- количество столбцов в матрице
// [IN] size					- количество процессов
// [OUT] rows_per_process		- количество строк матрицы, которые можно равномерно распределить на каждый процесс (за исключением последнего процесса, в случае, если равномерное распределение невозможно)
// [OUT] remaining_rows			- количество строк матрицы, которые будут переданы последнему процессу
// [OUT] items_per_process		- количество элементов, которые будут переданны каждому процессу (за исключением последнего)
// [OUT] items_per_last_process	- количество элементов, которые будут переданы последнему процессу
void calculateRows(int rows, int cols, int size, int *rows_per_process, int *remaining_rows, int *items_per_process, int *items_per_last_process) {
	*rows_per_process = rows / size;
	if (*rows_per_process == 0)
		*rows_per_process = 1;
	*remaining_rows = rows % size;
	*items_per_process = *rows_per_process * cols;
	*items_per_last_process = (rows - (*rows_per_process * (size - 1))) * cols;
}

//расчитывает разбиение матрицы для процесса с заданным ранком
// [IN] rows	- количество строк в матрице
// [IN] cols	- количество столбцов в матрице
// [IN] rank	- ранк процесса, для которого надо расчитать разбиение
// [IN] size	- количество процессов
// [OUT] result	- результат разбияния, который описывает смещение для процесса с заданным ранком
void calculateSendRows(int rows, int cols, int rank, int size, RowsInfo *result) {
	int rows_per_process, remaining_rows, items_per_process, items_per_last_process;
	calculateRows(rows, cols, size, &rows_per_process, &remaining_rows, &items_per_process, &items_per_last_process);

	int total = rows * cols;
	result->displacement = items_per_process * rank;
	result->recv_count = items_per_process;
	if (remaining_rows != 0 && rank == size - 1)
		result->recv_count = items_per_last_process;
	//процесс сможет получить или отправить строку, если количество отправляемых/получаемых данных отлично от нуля, а также если смещение (вместе или отдельно от общего передаваемого количества данных) не превосходит количествого данных в матрице
	result->can_receive = result->displacement <= total && result->displacement + result->recv_count <= total && result->recv_count != 0;
}

//вывод матрицы
// [IN] matrix	- матрица для вывода
// [IN] rows	- количество строк в матрице
// [IN] cols	- количество столбцов в матрице
// [IN] name	- заголовок вывода (название матрицы)
// [IN] verbose	- указывает, если надо выполнить подробный вывод с координатами (true) или вывести только значения матрицы (false)
void printMatrix(double *matrix, int rows, int cols, const char *name, bool verbose) {
	printf("\n=== %s ===\n", name);
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			verbose ? printf(" %s[%d,%d]=%.2f", name, i, j, matrix[i * cols + j]) : printf("%7.2f", matrix[i * cols + j]);
		}
		printf("\n");
	}
}

//распределяет строки матрицы по процессам
// [IN] size	- количество процессов
// [IN] matrix	- исходная матрица
// [IN] rows	- количество строк в матрице
// [IN] cols	- количество столбцов в матрице
// [IN] verbose	- указывает на необходимость промежуточных выводов
void sendMatrixRows(int size, double *matrix, int rows, int cols, bool verbose) {
	for (int i = 0; i < size; i++) {
		RowsInfo info;
		calculateSendRows(rows, cols, i, size, &info); //расчитываем разбиение для каждого процесса
		if (info.can_receive) {
			//если есть что отправить, отправляем данные процессу с ранком i
			verbose && printf("Sending data (%d-%d) to process with rank %d\n", info.displacement, info.displacement + info.recv_count, i);
			MPI_Send(matrix + info.displacement, info.recv_count, MPI_DOUBLE, i, TAG, MPI_COMM_WORLD);
		}
		else {
			verbose && printf("No data to send to process with rank %d\n", i);
		}
	}
}

//находит индексы всех максимальных элементов построчно
// [IN] rank							- ранк текущего процесса
// [IN] size							- колчество процессов
// [IN] matrix							- исходная матрица
// [IN] rows							- количество строк в матрице
// [IN] cols							- количество столбцов в матрице
// [OUT] _out_total_max_items			- общее число всех максимальных элементов
// [OUT] _out_all_max_coords_on_matrix	- индексы найденных максимальных элементов
// [IN] verbose							- указывает на необходимость промежуточных выводов
void all_max_loc(int rank, int size, double *matrix, int rows, int cols, int *_out_total_max_items, Coords **_out_all_max_coords_on_matrix, bool verbose) {
	MaxReport *maxs;
	if (rank == ROOT_RANK) {
		maxs = new MaxReport[size];
	}

	RowsInfo info;
	calculateSendRows(rows, cols, rank, size, &info); //расчитывает разбиение матрицы для текущего процесса

	//результат поиска максимальных элементов (количество максимальных элементов, найденных текущим процессов)
	MaxReport maxReport;
	maxReport.rank = rank;
	//устанавливаем кол-во найденных макс элементов = 0, поскольку все процессу должны будут вернуть процессу с ранком root результат поисков
	//если у данного процесса не было строк, значит и кол-во найденных элементов = 0
	maxReport.max_items = 0;
	vector<Coords> all_max_coords_on_process; //массив координат всех максимальных элементов, найденных данным процессом

	if (info.can_receive) { //если под процесс выделены строки
		//получаем данные (строки) от процесса с ранком root
		double buf[info.recv_count];
		MPI_Recv(buf, info.recv_count, MPI_DOUBLE, ROOT_RANK, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		verbose && printf("Rank %d received:\n", rank);

		int received_rows_n = info.recv_count / cols; //количество полученных строк. 
		//в данном случае остаток от деления (info.recv_count % cols) будет равен 0, т.к. расчет разбиения производится в функции calculateSendRows
		//это значит, что не надо учитывать условие, что остались строки, а можно сразу приступить к поиску

		//поиск максимальных элементов в каждой строке
		for (int i = 0; i < received_rows_n; i++) {
			int last_max_index = 0;
			vector<Coords> max_row_coords; //координаты максимальных элементов строки i
			max_row_coords.push_back(Coords(i, last_max_index)); //добавляем первый элемент
			for (int j = 0; j < cols; j++) {
				verbose && printf("%.2f ", buf[i * cols + j]);
				if (j == 0)
					continue;
				//пробегаем по всей строке, начиная со второго элемента для поиска максимального значения
				double current = buf[i * cols + j], max = buf[i * cols + last_max_index];
				if (current > max) {
					//если текущий элемент больше максимального, очищаем массив координат максимальных элементов данной строки и добавляем текущий элемент в этот массив
					last_max_index = j;
					max_row_coords.clear();
					max_row_coords.push_back(Coords(i, j));
				}
				else if (current == max) {
					//если текуший элемент равен максимальному, определенному ранне, добавляем координаты в массив
					max_row_coords.push_back(Coords(i, j));
				}
			}

			verbose && printf(" | ");
			//добавляем массив найденных координат в массив координат всех максимальных элементов, найденных данным процессом, при этом пересчитываем координаты строк на координаты строки относительно всей матрицы
			for (vector<Coords>::iterator iterator = max_row_coords.begin(); iterator != max_row_coords.end(); ++iterator) {
				int global_row = info.displacement / cols + iterator->row;  //координаты текущей строки относительно всей матрицы
				all_max_coords_on_process.push_back(Coords(global_row, iterator->col));
				verbose && printf("(%d[%d],%d) ", iterator->row, global_row, iterator->col);
			}
			verbose && printf(" * %d %d", info.displacement, rows);
			verbose && printf("\n");
		}
		maxReport.max_items = all_max_coords_on_process.size(); //обновляем кол-во найденных макс элементов данным процессом
	}
	else 
		verbose && printf("Rank %d did not receive any rows\n", rank);

	//собираем данные о количестве найденных максимальных элементов со всех процессов
	//испольуем функцию MPI_Gather, поскольку каждый процесс вернет по 1 структуре MaxReport типа MPI_2INT
	MPI_Gather(&maxReport, 1, MPI_2INT, maxs, 1, MPI_2INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	Coords *all_max_coords_on_matrix;
	int total_max_items = 0;
	int *all_max_coords_on_matrix_displacements, *all_max_coords_on_matrix_recv_count;
	if (rank == ROOT_RANK) {
		//поскольку каждый процесс вернет разное количество данных (в зависимости от количества найденных максимальных элементов в каждой строке)
		//расчитываем количество оправляемых данных от каждого процесса и смещения отправляемых данных для MPI_Gatherv
		all_max_coords_on_matrix_displacements = new int[size]; //массив смещений
		all_max_coords_on_matrix_recv_count = new int[size]; //массив количества передаваемых данных
		all_max_coords_on_matrix_displacements[0] = 0; //смещение процесса с ранком root = 0
		for (int i = 0; i < size; i++) {
			verbose && printf("Process %d found %d items\n", i, maxs[i].max_items);
			all_max_coords_on_matrix_recv_count[i] = maxs[i].max_items;
			total_max_items += maxs[i].max_items;
			if (i > 0) {
				//смещение последущих процессов = смещение предыдущего + кол-во элементов переданных процессом с предыдущим ранком
				all_max_coords_on_matrix_displacements[i] = all_max_coords_on_matrix_displacements[i - 1] + maxs[i - 1].max_items;
			}
		}
		all_max_coords_on_matrix = new Coords[total_max_items];
	}
	MPI_Barrier(MPI_COMM_WORLD);
	
	//собираем данные о координатах найденных максимальных элементов со всех процессов
	//поскольку каждый процесс вернет разное количество данных (в зависимости от количества найденных максимальных элементов в каждой строке), используем MPI_Gatherv
	MPI_Gatherv(all_max_coords_on_process.data(), all_max_coords_on_process.size(), MPI_2INT, all_max_coords_on_matrix, all_max_coords_on_matrix_recv_count, all_max_coords_on_matrix_displacements, MPI_2INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == ROOT_RANK) {
		*_out_total_max_items = total_max_items;
		*_out_all_max_coords_on_matrix = all_max_coords_on_matrix;
	}
}

//поиск и вывод решений (максимальных элементов с одинаковыми координатами)
// [IN] a_coords	- массив координат найденных максимальных элементов в матрице A
// [IN] a_count		- количество координат найденных максимальных элементов в матрице A
// [IN] a_matrix	- исходная матрица A
// [IN] a_cols		- количество столбцов в матрице A
// [IN] b_coords	- массив координат найденных максимальных элементов в матрице B
// [IN] b_count		- количество координат найденных максимальных элементов в матрице B
// [IN] b_matrix	- исходная матрица B
// [IN] b_cols		- количество столбцов в матрице B
void printSolutions(Coords *a_coords, int a_count, double *a_matrix, int a_cols, 
					Coords *b_coords, int b_count, double *b_matrix, int b_cols) {
	int total = 0;
	for (int i = 0; i < a_count; i++) {
		for (int j = 0; j < b_count; j++) {
			if (a_coords[i].row == b_coords[j].row && a_coords[i].col == b_coords[j].col) {
				if (total == 0)
					printf("\n----------- SOLUTIONS ------------\n");
				int a_index = a_coords[i].row * a_cols + a_coords[i].col;
				int b_index = b_coords[j].row * b_cols + b_coords[j].col;
				printf("A(%d,%d)=%.2f\tB(%d,%d)=%.2f\n", a_coords[i].row, a_coords[i].col, a_matrix[a_index], b_coords[j].row, b_coords[j].col, b_matrix[b_index]);
				total++;
			}
		}
	}
	if (total > 0)
		printf("------------ TOTAL %d ------------\n", total);
	else
		printf("---------- NO SOLUTIONS ----------\n");
}

int main(int argc, char **argv) {
	srand(time(NULL));

	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int rows, cols;
	double *A, *B, *transposedB;
	if (rank == ROOT_RANK) {
		if (USE_STATIC_MATRIX) {
			rows = 6;
			cols = 6;
		}
		else {
			printf("Enter number of rows\n");
			scanf("%d", &rows);
			printf("Enter number of columns\n");
			scanf("%d", &cols);
		}
	}
	
	//передаем кол-во строк и столбцов всем процессам
	MPI_Bcast(&rows, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&cols, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	int total = rows * cols;
	
	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == ROOT_RANK) {
		//генерируем матрицы A и B
		A = (double*)malloc(total * sizeof(double));
		B = (double*)malloc(total * sizeof(double));
		
		if (USE_STATIC_MATRIX) {
			fillStaticData(A, B);
		}
		else {
			for (int i = 0; i < total; i++) {
				A[i] = rand() / 1000000000.0;
				B[i] = rand() / 1000000000.0;
			}
		}

		printMatrix(A, rows, cols, "A", false);
		printMatrix(B, rows, cols, "B", false);

		transposedB = transposeMatrix(B, rows, cols);
		//printMatrix(transposedB, cols, rows, "tB", false);

		//отпавляем процессам матрицу A и транспонированную матрицу B
		sendMatrixRows(size, A, rows, cols, false);
		sendMatrixRows(size, transposedB, cols, rows, false);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	int A_total_max_items, B_total_max_items;
	Coords *A_all_max_coords_on_matrix, *B_all_max_coords_on_matrix;
	all_max_loc(rank, size, A, rows, cols, &A_total_max_items, &A_all_max_coords_on_matrix, false); //поиск всех максимальных элементов построчно в матрице A
	all_max_loc(rank, size, transposedB, cols, rows, &B_total_max_items, &B_all_max_coords_on_matrix, false); //поиск всех максимальных элементов по столбцам в матрице B

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == ROOT_RANK) {
		printf("== TOTAL MAX COUNT A (rows) = %d ==\n", A_total_max_items);
		for (int i = 0; i < A_total_max_items; i++) {
			int row = A_all_max_coords_on_matrix[i].row, col = A_all_max_coords_on_matrix[i].col;
			printf("(%d,%d)=%.2f\n", row, col, A[row * cols + col]);
		}
		for (int i = 0; i < B_total_max_items; i++)
			B_all_max_coords_on_matrix[i].swap();
		printf("== TOTAL MAX COUNT B (cols) = %d ==\n", B_total_max_items);
		for (int i = 0; i < B_total_max_items; i++) {
			int row = B_all_max_coords_on_matrix[i].row, col = B_all_max_coords_on_matrix[i].col;
			printf("(%d,%d)=%.2f\n", row, col, B[row * cols + col]);
		}

		printSolutions(	A_all_max_coords_on_matrix, A_total_max_items, A, cols, 
						B_all_max_coords_on_matrix, B_total_max_items, B, cols);
	}

	MPI_Finalize();
	return 0;
}