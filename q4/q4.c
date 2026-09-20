#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUCLEOS_TESTE 1 // para fazer o teste com 1, 2, 4 núcleos
#define TAM_SISTEMA 5// matriz será NxN, com N = TAM_SISTEMA
#define P 10 // número de iterações

// dados do sistema (compartilhados entre as threads)
// usamos double para aumentar a precisão do método
double A[TAM_SISTEMA][TAM_SISTEMA];
double b[TAM_SISTEMA];
double x_k[TAM_SISTEMA]; // vetor xˆ(k)
double x_kmais1[TAM_SISTEMA]; // vetor xˆ(k + 1)

pthread_barrier_t barreira;

// struct com os dados que cada thread irá processar

typedef struct {

    int id; // identificador da thread
    int inicio; // índice da primeira incógnita a ser processada pela thread
    int fim; // índice da última incógnita a ser processada pela thread

} DadosThread;

void *calculo_jacobi(void *arg){

    DadosThread *dados = (DadosThread*) arg;

    // executando o algoritmo de Jacobi P vezes
    for(int k = 0; k < P; k++){

        // cálculo de x_kmais1
        for(int i = dados->inicio; i < dados->fim; i++){

            double soma = 0.0;
            for(int j = 0; j < TAM_SISTEMA; j++){
                if(i != j){
                    soma += A[i][j] * x_k[j];
                }
            }

            x_kmais1[i] = (b[i] - soma) / A[i][i];

        }

        // forçando as threads a aguardarem o cálculo de x_kmais1
        pthread_barrier_wait(&barreira);

        // atualizando o vetor x_k com os valores obtidos após o cálculo
        for(int i = dados->inicio; i < dados->fim; i++){
            x_k[i] = x_kmais1[i];
        }

        // forçando as threads a esperarem as demais atualizarem x_k antes de darem prosseguimento ao cálculo
        pthread_barrier_wait(&barreira);
    }

    // terminada a aplicação do método, encerramos as threads 
    pthread_exit(NULL);
}

void criar_threads(pthread_t *threads, DadosThread *dados, void *(*calculo_jacobi)(void *)){

    int tam_bloco = TAM_SISTEMA / NUCLEOS_TESTE;
    int resto = TAM_SISTEMA % NUCLEOS_TESTE;
    int inicio_atual = 0;

    // distribuindo variáveis entre as threads
    for (int i = 0; i < NUCLEOS_TESTE; i++){

        dados[i].id = i;
        dados[i].inicio = inicio_atual;

        // aumentando em 1 a thread, caso o índice dela seja menor do que o resto
        // exemplo: se resto = 3, as threads cujo id é 0, 1 ou 2 devem receber 1 incógnita a mais para processar
        dados[i].fim = inicio_atual + tam_bloco + (i < resto ? 1:0);
        inicio_atual = dados[i].fim; // atualizando o início da próxima thread (limite exclusivo)

        // criando a thread passando a struct correspondente
        int rc = pthread_create(&threads[i], NULL, calculo_jacobi, (void*)&dados[i]);

        if(rc){
            printf("ERRO; código de retorno é %d.\n", rc);
            exit(-1);
        }
    }
}

int main{

    return 0;
}
