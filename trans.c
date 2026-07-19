/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    /*
     * 32x32: 8x8 blocking + register buffering.
     *
     * One row = 32 ints = 128B = 4 cache blocks.  An 8-int segment
     * fits exactly in one 32-byte cache block.
     *
     * Strategy: read an entire 8-element row of A into local vars
     * (a0-a7) BEFORE writing to B.  This way, even if writing to B
     * evicts A's cache line (diagonal blocks where A and B map to the
     * same set), we already have A's values in registers.
     */
    if (M == 32 && N == 32) {
        for (i = 0; i < N; i += 8) {          /* tile rows */
            for (j = 0; j < M; j += 8) {      /* tile cols */
                for (k = i; k < i + 8; k++) {
                    /* load full row from A into registers */
                    a0 = A[k][j];
                    a1 = A[k][j + 1];
                    a2 = A[k][j + 2];
                    a3 = A[k][j + 3];
                    a4 = A[k][j + 4];
                    a5 = A[k][j + 5];
                    a6 = A[k][j + 6];
                    a7 = A[k][j + 7];

                    /* store to B -- A is no longer needed from cache */
                    B[j][k] = a0;
                    B[j + 1][k] = a1;
                    B[j + 2][k] = a2;
                    B[j + 3][k] = a3;
                    B[j + 4][k] = a4;
                    B[j + 5][k] = a5;
                    B[j + 6][k] = a6;
                    B[j + 7][k] = a7;
                }
            }
        }
        return;
    }

    /*
     * 64x64: 8x8 blocking split into 4x4 quadrants + B as temp storage.
     *
     * Problem: row stride = 64*4 = 256B = 8 cache blocks.  The cache
     * has 32 sets, so rows 4 apart map to the SAME cache set.  With
     * direct-mapped (E=1), row 0 and row 4 cannot coexist -- one
     * always evicts the other.  Naive 8x8 blocking causes miss storms.
     *
     * Solution: process each 8x8 tile in three phases, never touching
     * more than 4 rows of the same array at once.
     *
     * Quadrant layout of one 8x8 tile:
     *
     *   A (source)          B (destination, transposed)
     *   ┌──────┬──────┐     ┌──────┬──────┐
     *   │ A_TL │ A_TR │     │ B_TL │ B_TR │
     *   ├──────┼──────┤     ├──────┼──────┤
     *   │ A_BL │ A_BR │     │ B_BL │ B_BR │
     *   └──────┴──────┘     └──────┴──────┘
     *
     *   Correct transpose mapping:
     *     A_TL^T → B_TL    A_TR^T → B_BL
     *     A_BL^T → B_TR    A_BR^T → B_BR
     */
    if (M == 64 && N == 64) {
        for (i = 0; i < N; i += 8) {          /* tile rows */
            for (j = 0; j < M; j += 8) {      /* tile cols */

                /*
                 * Phase 1: read A's top 4 rows (A_TL and A_TR).
                 *   - A_TL^T goes to B_TL (correct final position).
                 *   - A_TR^T goes to B_TR as TEMPORARY storage.
                 *     (It belongs in B_BL, but we can't write there
                 *      yet without causing set conflicts with B_TL.)
                 */
                for (k = i; k < i + 4; k++) {
                    a0 = A[k][j];
                    a1 = A[k][j + 1];
                    a2 = A[k][j + 2];
                    a3 = A[k][j + 3];
                    a4 = A[k][j + 4];
                    a5 = A[k][j + 5];
                    a6 = A[k][j + 6];
                    a7 = A[k][j + 7];

                    /* A_TL^T → B_TL (final position) */
                    B[j][k] = a0;
                    B[j + 1][k] = a1;
                    B[j + 2][k] = a2;
                    B[j + 3][k] = a3;

                    /* A_TR^T → B_TR (TEMPORARY -- will be moved later) */
                    B[j][k + 4] = a4;
                    B[j + 1][k + 4] = a5;
                    B[j + 2][k + 4] = a6;
                    B[j + 3][k + 4] = a7;
                }

                /*
                 * Phase 2: swap B_TR temp values with A_BL, in the
                 * same loop to reuse B's cache lines while they're hot.
                 *
                 * For each column k of B's top 4 rows:
                 *   1. Read B_TR temp values (A_TR^T) into registers
                 *   2. Read A_BL column and write A_BL^T → B_TR (final)
                 *   3. Write the saved A_TR^T values → B_BL (final)
                 */
                for (k = j; k < j + 4; k++) {
                    /* save temp values from B_TR */
                    a0 = B[k][i + 4];
                    a1 = B[k][i + 5];
                    a2 = B[k][i + 6];
                    a3 = B[k][i + 7];

                    /* A_BL^T → B_TR (final position) */
                    a4 = A[i + 4][k];
                    a5 = A[i + 5][k];
                    a6 = A[i + 6][k];
                    a7 = A[i + 7][k];

                    B[k][i + 4] = a4;
                    B[k][i + 5] = a5;
                    B[k][i + 6] = a6;
                    B[k][i + 7] = a7;

                    /* saved A_TR^T → B_BL (final position) */
                    B[k + 4][i] = a0;
                    B[k + 4][i + 1] = a1;
                    B[k + 4][i + 2] = a2;
                    B[k + 4][i + 3] = a3;
                }

                /*
                 * Phase 3: A_BR^T → B_BR (final position).
                 * Straightforward -- no conflict issues here.
                 */
                for (k = i + 4; k < i + 8; k++) {
                    a0 = A[k][j + 4];
                    a1 = A[k][j + 5];
                    a2 = A[k][j + 6];
                    a3 = A[k][j + 7];

                    B[j + 4][k] = a0;
                    B[j + 5][k] = a1;
                    B[j + 6][k] = a2;
                    B[j + 7][k] = a3;
                }
            }
        }
        return;
    }

    /*
     * 61x67 (and any other size): simple 23x23 blocking.
     *
     * Row stride = 61*4 = 244B, which is NOT a power of two, so rows
     * don't align to the same cache sets as neatly as 32x32 or 64x64.
     * The conflict pattern is naturally spread out, so a simple tile
     * without register tricks is enough.
     *
     * 23 works well because it's coprime with 32 (the number of cache
     * sets), further reducing set conflicts across tile boundaries.
     */
    for (i = 0; i < N; i += 23) {
        for (j = 0; j < M; j += 23) {
            for (k = i; k < i + 23 && k < N; k++) {
                for (l = j; l < j + 23 && l < M; l++) {
                    B[l][k] = A[k][l];
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
