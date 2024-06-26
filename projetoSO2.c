#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define X 99
#define Y 99

typedef struct {
    char *nomeArquivo;
    int matriz[X][Y];
    int linhas;
    int colunas;
} LeituraArgs;

typedef struct {
    int (*matriz1)[Y];
    int (*matriz2)[Y];
    int (*matrizResultado)[Y];
    int linhas;
    int colunas;
    int startRow;
    int endRow;
} OperacaoArgs;

void *lerMatriz(void *args) {
    LeituraArgs *leituraArgs = (LeituraArgs *)args;
    FILE *arquivo = fopen(leituraArgs->nomeArquivo, "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s\n", leituraArgs->nomeArquivo);
        pthread_exit(NULL);
    }

    fscanf(arquivo, "%d %d", &leituraArgs->linhas, &leituraArgs->colunas);
    for (int i = 0; i < leituraArgs->linhas; i++) {
        for (int j = 0; j < leituraArgs->colunas; j++) {
            fscanf(arquivo, "%d", &leituraArgs->matriz[i][j]);
        }
    }
    fclose(arquivo);
    pthread_exit(NULL);
}

void *somaMatriz(void *args) {
    OperacaoArgs *opArgs = (OperacaoArgs *)args;
    for (int i = opArgs->startRow; i < opArgs->endRow; i++) {
        for (int j = 0; j < opArgs->colunas; j++) {
            opArgs->matrizResultado[i][j] = opArgs->matriz1[i][j] + opArgs->matriz2[i][j];
        }
    }
    pthread_exit(NULL);
}

void *multiplicaMatriz(void *args) {
    OperacaoArgs *opArgs = (OperacaoArgs *)args;
    for (int i = opArgs->startRow; i < opArgs->endRow; i++) {
        for (int j = 0; j < opArgs->colunas; j++) {
            opArgs->matrizResultado[i][j] = 0;
            for (int k = 0; k < opArgs->colunas; k++) {
                opArgs->matrizResultado[i][j] += opArgs->matriz1[i][k] * opArgs->matriz2[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

void gravaMatriz(char *nomeArquivo, int matriz[X][Y], int linhas, int colunas) {
    FILE *arquivo = fopen(nomeArquivo, "w");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s para escrita\n", nomeArquivo);
        return;
    }

    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            fprintf(arquivo, "%d ", matriz[i][j]);
        }
        fprintf(arquivo, "\n");
    }
    fclose(arquivo);
}

void *reduzMatriz(void *args) {
    OperacaoArgs *opArgs = (OperacaoArgs *)args;
    int *resultado = malloc(sizeof(int));
    *resultado = 0;
    for (int i = 0; i < opArgs->linhas; i++) {
        for (int j = 0; j < opArgs->colunas; j++) {
            *resultado += opArgs->matrizResultado[i][j];
        }
    }
    pthread_exit((void *)resultado);
}

int main(int argc, char *argv[]) {
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

    int matrizD[X][Y], matrizE[X][Y];

    pthread_t threadsLeitura[2];
    LeituraArgs leituraArgsA = {arqA, {0}, 0, 0};
    LeituraArgs leituraArgsB = {arqB, {0}, 0, 0};

    clock_t start, end;
    double somaTime, multiplicacaoTime, reducaoTime, totalTime;

    // Passo 1: Leitura das matrizes A e B
    start = clock();
    pthread_create(&threadsLeitura[0], NULL, lerMatriz, (void *)&leituraArgsA);
    pthread_create(&threadsLeitura[1], NULL, lerMatriz, (void *)&leituraArgsB);

    pthread_join(threadsLeitura[0], NULL);
    pthread_join(threadsLeitura[1], NULL);
    end = clock();
    somaTime = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Passo 2: Soma das matrizes A e B = D
    start = clock();
    pthread_t threadsSoma[T];
    OperacaoArgs opArgsSoma[T];
    int rowsPerThread = n / T;

    for (int i = 0; i < T; i++) {
        opArgsSoma[i].matriz1 = leituraArgsA.matriz;
        opArgsSoma[i].matriz2 = leituraArgsB.matriz;
        opArgsSoma[i].matrizResultado = matrizD;
        opArgsSoma[i].linhas = n;
        opArgsSoma[i].colunas = n;
        opArgsSoma[i].startRow = i * rowsPerThread;
        opArgsSoma[i].endRow = (i + 1) * rowsPerThread;
        pthread_create(&threadsSoma[i], NULL, somaMatriz, (void *)&opArgsSoma[i]);
    }

    for (int i = 0; i < T; i++) {
        pthread_join(threadsSoma[i], NULL);
    }
    end = clock();
    somaTime = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Passo 3: Gravação da matriz D
    gravaMatriz(arqD, matrizD, n, n);

    // Passo 4: Leitura da matriz C
    start = clock();
    pthread_t threadLeituraC;
    LeituraArgs leituraArgsC = {arqC, {0}, 0, 0};
    pthread_create(&threadLeituraC, NULL, lerMatriz, (void *)&leituraArgsC);
    pthread_join(threadLeituraC, NULL);
    end = clock();
    multiplicacaoTime = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Passo 5: Multiplicação das matrizes D e C = E
    start = clock();
    pthread_t threadsMultiplicacao[T];
    OperacaoArgs opArgsMultiplicacao[T];

    for (int i = 0; i < T; i++) {
        opArgsMultiplicacao[i].matriz1 = matrizD;
        opArgsMultiplicacao[i].matriz2 = leituraArgsC.matriz;
        opArgsMultiplicacao[i].matrizResultado = matrizE;
        opArgsMultiplicacao[i].linhas = n;
        opArgsMultiplicacao[i].colunas = n;
        opArgsMultiplicacao[i].startRow = i * rowsPerThread;
        opArgsMultiplicacao[i].endRow = (i + 1) * rowsPerThread;
        pthread_create(&threadsMultiplicacao[i], NULL, multiplicaMatriz, (void *)&opArgsMultiplicacao[i]);
    }

    for (int i = 0; i < T; i++) {
        pthread_join(threadsMultiplicacao[i], NULL);
    }
    end = clock();
    multiplicacaoTime = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Passo 6: Gravação da matriz E
    gravaMatriz(arqE, matrizE, n, n);

    // Passo 7: Redução da matriz E
    start = clock();
    pthread_t threadsReducao[T];
    OperacaoArgs opArgsReducao[T];
    int reducao = 0;
    int *parcial;

    for (int i = 0; i < T; i++) {
        opArgsReducao[i].matrizResultado = matrizE;
        opArgsReducao[i].linhas = n;
        opArgsReducao[i].colunas = n;
        pthread_create(&threadsReducao[i], NULL, reduzMatriz, (void *)&opArgsReducao[i]);
    }

    for (int i = 0; i < T; i++) {
        pthread_join(threadsReducao[i], (void **)&parcial);
        reducao += *parcial;
        free(parcial);
    }
    end = clock();
    reducaoTime = ((double)(end - start)) / CLOCKS_PER_SEC;

    totalTime = somaTime + multiplicacaoTime + reducaoTime;
    printf("Tempo total: %.6f segundos\n", totalTime);

    return 0;
}
