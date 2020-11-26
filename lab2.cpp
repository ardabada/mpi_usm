#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

using namespace std;

//определяет фиксированную грань куба
enum SIDE {
	X_FIRST, 	//грань, где x = 0
	X_LAST, 	//грань, где x = размерность куба
	Y_FIRST, 	//грань, где y = 0
	Y_LAST, 	//грань, где y = размерность куба
	Z_FIRST,	//грань, где z = 0
	Z_LAST		//грань, где z = размерность куба
};

const SIDE FIXED_SIDE = X_LAST; //выбранная фиксированная грань
const int ROOT_RANK = 0; //ранк root процесса
const int TAG = 111; //тэг для MPI_Send, MPI_Recv
const int N_DIMS = 3; //количество измерений куба
const int RELATIVE_TO_AXIS = 0; //указывает, если расчет направления по часовой осуществляется для всех осей или конкретно для каждой грани (0 - для каждой грани, 1 - для оси)

//определяет трехмерные координаты
struct Coords3D {
	Coords3D() { }
	Coords3D(int x, int y, int z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	int x;
	int y;
	int z;
};
//определяет двумерные координаты
struct Coords2D {
	Coords2D() { }
	Coords2D(int x, int y) {
		this->x = x;
		this->y = y;
	}

	int x;
	int y;

	bool operator==(Coords2D rhs) {
		return this->x == rhs.x && this->y == rhs.y;
	}
};
//определеяет координаты точки на грани для процесса
struct SidePoint {
	SidePoint() { }
	SidePoint(int rank, Coords3D source, Coords2D translated) {
		this->rank = rank;
		this->source = source;
		this->translated = translated;
	}

	int rank; //ранк процесса
	Coords3D source; //трехмерные координаты
	Coords2D translated; //исходные координаты, переведенные в двумерное пространство, в зависимости от фиксированной грани
};

//определяет, можно ли создать куб с заданным числом процессов. 
//возвращает true, если можно создать куб с заддным числом процессов. в противном случае - false
// [IN] numtasks		- количество процессов
// [OUT] side_length	- размер грани куба при заданном числе процессов
bool canCreateCube(int numtasks, int *side_length) {
	//число процессов должно быть 3 степень числа, которое определяет размерность куба
	//например, если side_length = 3, то numtasks = 27, если side_length = 4, то numtasks = 64 и тд.
	int curoot = round(pow(numtasks, 1.0/3.0));
	*side_length = curoot;
	return curoot * curoot * curoot == numtasks;
}

//проверяет, если точка находится на ребре куба
// [IN] points		- точка для проверки
// [IN] side		- зафиксированная грань
// [IN] side_length - размерность куба
bool isPointOnSide(Coords3D &point, SIDE side, int side_length) {
	int last_coord = side_length - 1;
	switch (side) {
		case X_FIRST:
			return point.x == 0 && (point.y == 0 || point.y == last_coord || point.z == 0 || point.z == last_coord); 
		case X_LAST:
			return point.x == last_coord && (point.y == 0 || point.y == last_coord || point.z == 0 || point.z == last_coord);
		case Y_FIRST:
			return point.y == 0 && (point.x == 0 || point.x == last_coord || point.z == 0 || point.z == last_coord);
		case Y_LAST:
			return point.y == last_coord && (point.x == 0 || point.x == last_coord || point.z == 0 || point.z == last_coord); 
		case Z_FIRST:
			return point.z == 0 && (point.x == 0 || point.x == last_coord || point.z == 0 || point.z == last_coord); 
		case Z_LAST:
			return point.z == last_coord && (point.x == 0 || point.x == last_coord || point.y == 0 || point.y == last_coord);
		default:
			return false;
	}
}

//переводит 3d координаты в 2d, в зависимости от фиксированной грани
// [IN] side		- зафиксированная грань
// [IN] point3d		- исходная точка
// [OUT] point2d	- переведенные координаты
void translateTo2D(SIDE side, Coords3D &point3d, Coords2D *point2d) {
	switch (side) {
		case X_FIRST:
		case X_LAST:
			//если зафиксировали грань x, то нас интересуют координаты y и z
			point2d->x = point3d.y;
			point2d->y = point3d.z;
			break;
		case Y_FIRST:
		case Y_LAST:
			//если зафиксировали грань y, то нас интересуют координаты x и z
			point2d->x = point3d.x;
			point2d->y = point3d.z;
			break;
		case Z_FIRST:
		case Z_LAST:
			//если зафиксировали грань z, то нас интересуют координаты x и y
			point2d->x = point3d.x;
			point2d->y = point3d.y;
			break;
	}
}

//выполняет поиск точек с заданными двумерными координатами и возвращает указатель на найденный элемент
// [IN] data		- массив исходных данных
// [IN] ranks_count	- количество элементов в массиве
// [IN] coord		- двумерные координаты для поиска
SidePoint *findBy2dCoord(SidePoint *data, int ranks_count, Coords2D coord) {
	for (int i = 0; i < ranks_count; i++) {
		if (data[i].translated == coord)
			return &data[i];
	}
	return NULL;
}

//определяет ранки процессов на грани куба
//созвращает массив ранков отсортированных в порядке прохода по часовой стрелке, начиная с точки (0,0) в двумерном представлении грани
// [IN] side			- зафиксированная грань
// [IN] side_length		- размерность куба
// [OUT] ranks_count	- количество процессов на грани
int *getRanksOnSide(SIDE side, int side_length, int *ranks_count) {
	int last_coord = side_length - 1;
	*ranks_count = 4 * last_coord;
	int *ranks = (int*)malloc(*ranks_count * sizeof(int));
	SidePoint *data = (SidePoint*)malloc(*ranks_count * sizeof(SidePoint));
	int current_rank = 0, i = 0;
	Coords3D coord;
	Coords2D coord_translated;

	//пробегаем по всем точкам куба
	for (int x = 0; x < side_length; x++) {
		coord.x = x;
		for (int y = 0; y < side_length; y++) {
			coord.y = y;
			for (int z = 0; z < side_length; z++, current_rank++) {
				coord.z = z;
				if (!isPointOnSide(coord, side, side_length))
					continue;

				//если точка (x,y,z) принадлежит заданной грани, переводим ее координаты в двумерные
				translateTo2D(side, coord, &coord_translated);
				data[i++] = SidePoint(current_rank, coord, coord_translated);
			}
		}
	}

	//сортируем ранки в порядке прохода по часовой стрелке
	i = 0;
	Coords2D p(0,0);
	SidePoint *ordered;

	do {
		ordered = findBy2dCoord(data, *ranks_count, p);
		ordered != NULL && (ranks[i++] = ordered->rank);
		p.x++;
	} while (p.x < last_coord);

	do {
		ordered = findBy2dCoord(data, *ranks_count, p);
		ordered != NULL && (ranks[i++] = ordered->rank);
		p.y++;
	} while (p.y < last_coord);

	do {
		ordered = findBy2dCoord(data, *ranks_count, p);
		ordered != NULL && (ranks[i++] = ordered->rank);
		p.x--;
	} while (p.x > 0);

	do {
		ordered = findBy2dCoord(data, *ranks_count, p);
		ordered != NULL && (ranks[i++] = ordered->rank);
		p.y--;
	} while (p.y > 0);

	return ranks;
}

int main(int argc, char **argv) {
	int rank, size, side_length, side_rank;
	Coords3D coords;

	MPI_Comm comm, side_comm, side_ring_comm;
	MPI_Group MPI_GROUP_WORLD, side_group;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	//проверяем, если можно создать куб из текущего числа процессов
	bool isCube = canCreateCube(size, &side_length);
	if (!isCube) {
		if (rank == ROOT_RANK)
			printf("Can not create cube from %d processes.\n", size);

		MPI_Finalize();
		exit(1);
	}

	int dims[N_DIMS] = { side_length, side_length, side_length };
	int periods[N_DIMS] = { 0, 0, 0 };
	int _coords[N_DIMS];
	//создаём топологию куб
	MPI_Cart_create(MPI_COMM_WORLD, N_DIMS, dims, periods, 0, &comm);
	//получаем координаты процесса в топологии куб
	MPI_Cart_coords(comm, rank, N_DIMS, _coords);
	coords.x = _coords[0]; coords.y = _coords[1]; coords.z = _coords[2];

	//получаем ранки процессов, расположенных на ребрах фиксированной грани
	int ranks_on_side_count;
	int *ranks_on_side = getRanksOnSide(FIXED_SIDE, side_length, &ranks_on_side_count);

	//создаём группу для MPI_COMM_WORLD, чтобы создать новую группу из процессов на стороне одной грани
	MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);
	//создаём группу из процессов на стороне одной грани, чтобы создать топологию круг
	MPI_Group_incl(MPI_GROUP_WORLD, ranks_on_side_count, ranks_on_side, &side_group);
	//создаем коммуниктор для данной группы
	MPI_Comm_create(MPI_COMM_WORLD, side_group, &side_comm);
	//получаем ранк процесса в созданной группе
	MPI_Group_rank(side_group, &side_rank);

	if (side_rank != MPI_UNDEFINED) {
		//если процесс принадлежит группе (то есть находится на фиксированной грани)

		//создаём декартову топологию типа круг
		int side_comm_size, source, destination;
		int side_card_persiods[1] = { 1 };
		MPI_Comm_size(side_comm, &side_comm_size);
		MPI_Cart_create(side_comm, 1, &side_comm_size, side_card_persiods, 0, &side_ring_comm);

		int displ = -1;
		if (!RELATIVE_TO_AXIS && (FIXED_SIDE == X_FIRST || FIXED_SIDE == Y_FIRST || FIXED_SIDE == Z_FIRST))
			displ = 1;
		//определяем соседей
		MPI_Cart_shift(side_ring_comm, 1, displ, &source, &destination);

		//отправляем данные по кругу
		const int BUF_SIZE = N_DIMS + 2;
		int sendbuf[BUF_SIZE] = { rank, side_rank, coords.x, coords.y, coords.z };
		int recvbuf[BUF_SIZE]; 
		MPI_Send(sendbuf, BUF_SIZE, MPI_INT, destination, TAG, side_ring_comm);
		MPI_Recv(recvbuf, BUF_SIZE, MPI_INT, source, TAG, side_ring_comm, MPI_STATUS_IGNORE);

		printf("Process %3d (%d,%d,%d) -> %3d (%d,%d,%d)\n", sendbuf[0], sendbuf[2], sendbuf[3], sendbuf[4], recvbuf[0], recvbuf[2], recvbuf[3], recvbuf[4]);
		
		MPI_Comm_free(&side_ring_comm);
	}

	// MPI_Group_free(&side_group);
	MPI_Finalize();
	return 0;
}