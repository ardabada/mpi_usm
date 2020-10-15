#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    int numtask, sendcount, reccount, source;
    double *A;
    int i, myrank, root = 1;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &numtask);
    double ain[numtask], aout[numtask];
    int ind_A[numtask];
    int ind_B[numtask];
    struct
    {
        double val;
        int rank;
    } in[numtask], out[numtask];
    sendcount = numtask;
    reccount = numtask;
    srand(time(NULL));
    if (myrank == root)
    {
        A = (double*)malloc(numtask * numtask * sizeof(double));
		for(int i=0;i<numtask*numtask;i++){
  			A[i]=rand()/1000000000.0;
   		}
        printf("Tipar datele initiale\n");
        for (int i = 0; i < numtask; i++)
        {
            for (int j = 0; j < numtask; j++)
                printf("%.2f ", A[i * numtask + j]);
            printf("\n");
        }
        printf("\n");
        MPI_Barrier(MPI_COMM_WORLD);
    }
    else
        MPI_Barrier(MPI_COMM_WORLD);
    MPI_Scatter(A, sendcount, MPI_DOUBLE, ain, reccount, MPI_DOUBLE, root, MPI_COMM_WORLD);
    for (i = 0; i < numtask; ++i)
    {
        in[i].val = ain[i];
        in[i].rank = myrank;
    }
    MPI_Reduce(in, out, numtask, MPI_DOUBLE_INT, MPI_MAXLOC, root, MPI_COMM_WORLD);
    if (myrank == root)
    {
        printf("\n");
        for (int j = 0; j < numtask; ++j)
        {
            aout[j] = out[j].val;
            ind_A[j] = out[j].rank;
            printf("Coloana %d, max=  %.2f, linia= %d\n", j, aout[j], ind_A[j]);
        }
    }
	
    MPI_Finalize();
    return 0;
}
