#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <locale.h>

// Passar argumentos para as threads
typedef struct {
    int **A;
    int **B;
    int **C;
    int **D;
    int **E;
    int n;
    int start;
    int end;
} ThreadArgs;

// Alocar matriz dinamicamente
int **alocarMatriz(int n) {
    int **matriz = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        matriz[i] = (int *)malloc(n * sizeof(int));
    }
    return matriz;
}

// Ler matriz do arquivo
void lerMatriz(char *filename, int **matriz, int n) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fscanf(file, "%d", &matriz[i][j]);
        }
    }
    fclose(file);
}

// Escreve matriz do arquivo
void escreverMatriz(char *filename, int **matriz, int n) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(file, "%d ", matriz[i][j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// Soma matrizes A e B
void *somarMatrizes(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    for (int i = args->start; i < args->end; i++) {
        for (int j = 0; j < args->n; j++) {
            args->D[i][j] = args->A[i][j] + args->B[i][j];
        }
    }
    pthread_exit(NULL);
}

// Multiplicacao das matrizes D e C
void *multiplicarMatrizes(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    for (int i = args->start; i < args->end; i++) {
        for (int j = 0; j < args->n; j++) {
            args->E[i][j] = 0;
            for (int k = 0; k < args->n; k++) {
                args->E[i][j] += args->D[i][k] * args->C[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

// Reducao matriz E para 1 valor
int reduzirMatriz(int **matriz, int n) {
    int soma = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            soma += matriz[i][j];
        }
    }
    return soma;
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");  
    
    if (argc != 8) {
        printf("Uso: %s T n arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat\n", argv[0]);
        return 1;
    }

    int T = atoi(argv[1]);
    int n = atoi(argv[2]);
    char *arqA = argv[3];
    char *arqB = argv[4];
    char *arqC = argv[5];
    char *arqD = argv[6];
    char *arqE = argv[7];

    int **A = alocarMatriz(n);
    int **B = alocarMatriz(n);
    int **C = alocarMatriz(n);
    int **D = alocarMatriz(n);
    int **E = alocarMatriz(n);

    lerMatriz(arqA, A, n);
    lerMatriz(arqB, B, n);
    lerMatriz(arqC, C, n);

    pthread_t threads[T];
    ThreadArgs args[T];
    int step = n / T;
    clock_t start, end;
    double tempo_soma, tempo_multiplicacao, tempo_reducao, tempo_total;

    start = clock();
    // Thread para somar matrizes A e B
    for (int i = 0; i < T; i++) {
        args[i].A = A;
        args[i].B = B;
        args[i].D = D;
        args[i].n = n;
        args[i].start = i * step;
        args[i].end = (i == T - 1) ? n : (i + 1) * step;
        pthread_create(&threads[i], NULL, somarMatrizes, (void *)&args[i]);
    }
    // Aguardar as threads terminarem
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }
    escreverMatriz(arqD, D, n);
    end = clock();
    tempo_soma = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tempo soma: %f segundos.\n", tempo_soma);

    start = clock();
    // Threads para multiplicar matrizes D e C
    for (int i = 0; i < T; i++) {
        args[i].D = D;
        args[i].C = C;
        args[i].E = E;
        args[i].start = i * step;
        args[i].end = (i == T - 1) ? n : (i + 1) * step;
        pthread_create(&threads[i], NULL, multiplicarMatrizes, (void *)&args[i]);
    }
    // Aguardar as threads terminarem
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }
    escreverMatriz(arqE, E, n);
    end = clock();
    tempo_multiplicacao = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tempo multiplicacao: %f segundos.\n", tempo_multiplicacao);

    start = clock();
    // Reduzir matriz E
    int reducao = reduzirMatriz(E, n);
    end = clock();
    tempo_reducao = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Reducao: %d\n", reducao);
    printf("Tempo reducao: %f segundos.\n", tempo_reducao);

    // Tempo total
    tempo_total = tempo_soma + tempo_multiplicacao + tempo_reducao;
    printf("Tempo total: %f segundos.\n", tempo_total);

    // Liberar memória alocada
    for (int i = 0; i < n; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
        free(D[i]);
        free(E[i]);
    }
    free(A);
    free(B);
    free(C);
    free(D);
    free(E);

    return 0;
}
