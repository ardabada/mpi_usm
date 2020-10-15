/*====
In acest program se distrubuie coloane  unei matrici de dimensiuni arbitrare
proceselor din comunicatorul MPI_COMM_WORLD. Elementele matricei sunt initializate de procesul cu rankul 0.
====*/
#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
using namespace std;

//
void invertMatrix(double *m, int mRows, int mCols, double *rez)
{
    for (int i = 0; i < mRows; ++i)
        for (int j = 0; j < mCols; ++j)
            rez[j * mRows + i] = m[i * mCols + j];
}

int main(int argc, char *argv[])
{
    int size, reccount, source;
    double *Matr_Init, *Matr_Fin;
    int myrank, root = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int sendcounts[size], displs[size];
    int rows, cols;
    double *Cols;

    if (myrank == 0)
        printf("\n=====REZULTATUL PROGRAMULUI '%s' \n", argv[0]);

    MPI_Barrier(MPI_COMM_WORLD);

    //Procesul cu rankul root citeste dimensiunile matricii, aloca spatiul si initializeaza matricea
    if (myrank == root)
    {
        cout << "Introduceti numarul de rinduri:  ";
        cin >> rows;
        cout << "Introduceti numarul de coloane:  ";
        cin >> cols;
        Matr_Init = (double *)malloc(rows * cols * sizeof(double));
        Matr_Fin = (double *)malloc(rows * cols * sizeof(double));
        for (int i = 0; i < rows * cols; i++)
            Matr_Init[i] = rand() / 1000000000.0;

        printf("Matricea initiala\n");
        for (int i = 0; i < rows; i++)
        {
            printf("\n");
            for (int j = 0; j < cols; j++)
                printf("Matr_Init[%d,%d]=%5.2f ", i, j, Matr_Init[i * cols + j]);
        }
        printf("\n");
        MPI_Barrier(MPI_COMM_WORLD);
    }
    else
        MPI_Barrier(MPI_COMM_WORLD);

    MPI_Bcast(&rows, 1, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, root, MPI_COMM_WORLD);

    int rinduriPeProces = cols / size; // 3/3 = 1
    int rinduriRamase = cols % size;   // 3/3 = 0
    int deplasarea = 0;

    for (int i = 0; i < size; ++i)
    {
        displs[i] = deplasarea; // 0  3 6
        if (i < rinduriRamase)
            sendcounts[i] = (rinduriPeProces + 1) * rows;
        else
            sendcounts[i] = rinduriPeProces * rows; // 3
        deplasarea += sendcounts[i];                // 3
    }

    reccount = sendcounts[myrank];
    Cols = new double[reccount];

    double *invMatr;
    invMatr = new double[cols * rows];
    if (myrank == root)
    {
        invertMatrix(Matr_Init, rows, cols, invMatr);
    }

    MPI_Scatterv(invMatr, sendcounts, displs, MPI_DOUBLE, Cols, reccount, MPI_DOUBLE, root, MPI_COMM_WORLD);
    printf("\n");
    printf("Rezultatele f-tiei MPI_Scatterv pentru procesul cu rankul %d \n", myrank);

    for (int i = 0; i < reccount; ++i)
        printf(" Cols[%d]=%5.2f ", i, Cols[i]);

    printf("\n");
    cout << "\nProcesul " << myrank << " a primit " << reccount << " elemente (" << reccount / rows << " coloane)" << endl;
    MPI_Barrier(MPI_COMM_WORLD);

    int sendcount = reccount;
    int *recvcounts = sendcounts;
    MPI_Gatherv(Cols, sendcount, MPI_DOUBLE, Matr_Fin, recvcounts, displs, MPI_DOUBLE, root, MPI_COMM_WORLD);

    if (myrank == root)
    {
        printf("\n");
        printf("Resultatele f-tiei MPI_Gatherv ");
        for (int i = 0; i < rows; i++)
        {
            printf("\n");
            for (int j = 0; j < cols; j++)
                printf("Final[%d,%d]=%5.2f ", i, j, Matr_Fin[i * cols + j]);
        }
        printf("\n");
        MPI_Barrier(MPI_COMM_WORLD);
        free(Matr_Init);
        free(Matr_Fin);
    }
    else
        MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    delete[] Cols;
    return 0;
}
