#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define X 99
#define Y 99

typedef struct
{
  char *nomeArquivo;
  int matriz[X][Y];
  int linhas;
  int colunas;
} LeituraArgs;

typedef struct
{
  int (*matriz1)[X][Y];
  int (*matriz2)[X][Y];
  int (*matrizResultado)[X][Y];
  int inicio;
  int fim;
  int colunas;
} SomaArgs;

typedef struct
{
  int (*matriz1)[X][Y];
  int (*matriz2)[X][Y];
  int (*matrizResultado)[X][Y];
  int inicio;
  int fim;
  int colunas;
} MultArgs;

void *
lerMatriz (void *args)
{
  LeituraArgs *leituraArgs = (LeituraArgs *) args;
  FILE *arquivo = fopen (leituraArgs->nomeArquivo, "r");

  if (arquivo == NULL)
	{
	  printf ("Erro ao abrir o arquivo %s\n", leituraArgs->nomeArquivo);
	  pthread_exit (NULL);
	}

  fscanf (arquivo, "%d %d", &leituraArgs->linhas, &leituraArgs->colunas);

  for (int i = 0; i < leituraArgs->linhas; i++)
	{
	  for (int j = 0; j < leituraArgs->colunas; j++)
		{
		  fscanf (arquivo, "%d", &leituraArgs->matriz[i][j]);
		}
	}

  fclose (arquivo);
  pthread_exit (NULL);
}

void *
somaMatriz (void *args)
{
  SomaArgs *somaArgs = (SomaArgs *) args;

  for (int i = somaArgs->inicio; i < somaArgs->fim; i++)
	{
	  for (int j = 0; j < somaArgs->colunas; j++)
		{
		  (*somaArgs->matrizResultado)[i][j] =
			(*somaArgs->matriz1)[i][j] + (*somaArgs->matriz2)[i][j];
		}
	}

  pthread_exit (NULL);
}

void *
multiplicaMatriz (void *args)
{
  MultArgs *multArgs = (MultArgs *) args;

  for (int i = multArgs->inicio; i < multArgs->fim; i++)
	{
	  for (int j = 0; j < multArgs->colunas; j++)
		{
		  (*multArgs->matrizResultado)[i][j] = 0;
		  for (int k = 0; k < multArgs->colunas; k++)
			{
			  (*multArgs->matrizResultado)[i][j] +=
				(*multArgs->matriz1)[i][k] * (*multArgs->matriz2)[k][j];
			}
		}
	}

  pthread_exit (NULL);
}

void
gravaMatriz (char *nomeArquivo, int matriz[X][Y], int linhas, int colunas)
{
  FILE *arquivo = fopen (nomeArquivo, "w");

  if (arquivo == NULL)
	{
	  printf ("Erro ao abrir o arquivo %s\n", nomeArquivo);
	  return;
	}

  for (int i = 0; i < linhas; i++)
	{
	  for (int j = 0; j < colunas; j++)
		{
		  fprintf (arquivo, "%d ", matriz[i][j]);
		}
	  fprintf (arquivo, "\n");
	}

  fclose (arquivo);
}

void
reduzMatriz (int matriz[X][Y], int *resultado, int linhas, int colunas)
{
  *resultado = 0;
  for (int i = 0; i < linhas; i++)
	{
	  for (int j = 0; j < colunas; j++)
		{
		  *resultado += matriz[i][j];
		}
	}
}

int
main (int argc, char *argv[])
{
  if (argc != 8)
	{
	  printf ("Uso: %s T n arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat\n",
			  argv[0]);
	  return 1;
	}

  int T = atoi (argv[1]);
  int n = atoi (argv[2]);
  char *arqA = argv[3];
  char *arqB = argv[4];
  char *arqC = argv[5];
  char *arqD = argv[6];
  char *arqE = argv[7];

  int matrizA[X][Y], matrizB[X][Y], matrizC[X][Y], matrizD[X][Y],
	matrizE[X][Y];
  int linhas, colunas;
  int reducao;

  pthread_t threads[2];
  LeituraArgs leituraArgsA = { arqA, {0}, 0, 0 };
  LeituraArgs leituraArgsB = { arqB, {0}, 0, 0 };

  pthread_create (&threads[0], NULL, lerMatriz, (void *) &leituraArgsA);
  pthread_create (&threads[1], NULL, lerMatriz, (void *) &leituraArgsB);

  pthread_join (threads[0], NULL);
  pthread_join (threads[1], NULL);

  linhas = leituraArgsA.linhas;
  colunas = leituraArgsA.colunas;

  pthread_t somaThreads[T];
  SomaArgs somaArgs[T];
  int bloco = linhas / T;

  clock_t inicioSoma = clock ();

  for (int i = 0; i < T; i++)
	{
	  somaArgs[i].matriz1 = &leituraArgsA.matriz;
	  somaArgs[i].matriz2 = &leituraArgsB.matriz;
	  somaArgs[i].matrizResultado = &matrizD;
	  somaArgs[i].inicio = i * bloco;
	  somaArgs[i].fim = (i == T - 1) ? linhas : (i + 1) * bloco;
	  somaArgs[i].colunas = colunas;

	  pthread_create (&somaThreads[i], NULL, somaMatriz,
					  (void *) &somaArgs[i]);
	}

  for (int i = 0; i < T; i++)
	{
	  pthread_join (somaThreads[i], NULL);
	}

  clock_t fimSoma = clock ();

  gravaMatriz (arqD, matrizD, linhas, colunas);

  LeituraArgs leituraArgsC = { arqC, {0}, 0, 0 };
  pthread_create (&threads[0], NULL, lerMatriz, (void *) &leituraArgsC);
  pthread_join (threads[0], NULL);

  pthread_t multThreads[T];
  MultArgs multArgs[T];
  bloco = linhas / T;

  clock_t inicioMult = clock ();

  for (int i = 0; i < T; i++)
	{
	  multArgs[i].matriz1 = &matrizD;
	  multArgs[i].matriz2 = &leituraArgsC.matriz;
	  multArgs[i].matrizResultado = &matrizE;
	  multArgs[i].inicio = i * bloco;
	  multArgs[i].fim = (i == T - 1) ? linhas : (i + 1) * bloco;
	  multArgs[i].colunas = colunas;

	  pthread_create (&multThreads[i], NULL, multiplicaMatriz,
					  (void *) &multArgs[i]);
	}

  for (int i = 0; i < T; i++)
	{
	  pthread_join (multThreads[i], NULL);
	}

  clock_t fimMult = clock ();

  gravaMatriz (arqE, matrizE, linhas, colunas);

  clock_t inicioRed = clock ();
  reduzMatriz (matrizE, &reducao, linhas, colunas);
  clock_t fimRed = clock ();

  double tempoSoma = (double) (fimSoma - inicioSoma) / CLOCKS_PER_SEC;
  double tempoMult = (double) (fimMult - inicioMult) / CLOCKS_PER_SEC;
  double tempoRed = (double) (fimRed - inicioRed) / CLOCKS_PER_SEC;
  double tempoTotal = tempoSoma + tempoMult + tempoRed;

  printf ("ReduC'C#o: %d\n", reducao);
  printf ("Tempo soma: %.3f segundos.\n", tempoSoma);
  printf ("Tempo multiplicaC'C#o: %.3f segundos.\n", tempoMult);
  printf ("Tempo reduC'C#o: %.3f segundos.\n", tempoRed);
  printf ("Tempo total: %.3f segundos.\n", tempoTotal);

  return 0;
}
