#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TAM_SISTEMA 400// matriz será NxN, com N = TAM_SISTEMA
#define P 100 // número de iterações

// dados do sistema (compartilhados entre as threads)
// usamos double para aumentar a precisão do método
double A[TAM_SISTEMA][TAM_SISTEMA];
double b[TAM_SISTEMA];
double x_k[TAM_SISTEMA]; // vetor xˆ(k)
double x_kmais1[TAM_SISTEMA]; // vetor xˆ(k + 1)

pthread_barrier_t barreira;

struct timespec inicio, fim;

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

void criar_threads(pthread_t *threads, DadosThread *dados, void *(*calculo_jacobi)(void *), int nucleos_teste){

    int tam_bloco = TAM_SISTEMA / nucleos_teste;
    int resto = TAM_SISTEMA % nucleos_teste;
    int inicio_atual = 0;

    // distribuindo variáveis entre as threads
    for (int i = 0; i < nucleos_teste; i++){

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

int main(int argc, char *argv[]){

    int testes[] = {1, 2, 4};
    int total_testes = 3;

    // inicializando x_k, colocando todos os valores iguais a 1
    for(int k = 0; k < total_testes; k++){ // a cada iteração, é feito um teste

        int nucleos_teste = testes[k];

        pthread_t threads[nucleos_teste];
        DadosThread dados[nucleos_teste]; 

        for (int i = 0; i < TAM_SISTEMA; i ++){

        x_k[i] = 1.0;
        b[i] = 298.0; // vetor arbitrário de termos independentes

            /*
            O método de Jacobi funciona melhor quando A[i][i] 
            tem valor absoluto maior do que a soma dos valores absolutos da linha i,
            ou seja, quando a matriz é classificada como "estritamente diagonal dominante"
            sabendo disso, para ter resultados mais claros, vamos testar em matrizes desse tipo.
            */

            for(int j = 0; j< TAM_SISTEMA; j++){

                if(i == j){
                    A[i][j] = 100.0; // para a diagonal principal ser bem maior do que o resto
                }

                else{
                    A[i][j] = 1.0;
                }
            }
        }
    

    // inicializando a barreira
    pthread_barrier_init(&barreira, NULL, nucleos_teste);
    
    // registrando o tempo logo antes da criação das threads
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    // disparando as threads
    criar_threads(threads, dados, calculo_jacobi, nucleos_teste);

    // fazendo as threads aguardarem a execução das P iterações
    
    for (int i = 0; i< nucleos_teste; i++){
        pthread_join(threads[i], NULL);
    }
    
    // registrando o tempo de término 
    clock_gettime(CLOCK_MONOTONIC, &fim);

    // após o encerramento das threads, podemos destruir as barreiras
    pthread_barrier_destroy(&barreira);

    // calculando o tempo de execução (cálculo de segundos inteiros, cálculo da diferença em nanossegundos, soma de ambas as partes de divisão por 10ˆ9 (1e9), para obter uma fração decimal de segundos precisa)
    double tempo = (fim.tv_sec - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    // exibindo a solução final e o tempo de execução para a atual quantidade de núcleos
    printf("Solução final após %d iterações:\n", P);
    for (int i = 0; i < TAM_SISTEMA; i++){
        printf("x[%d] = %.2f\n", i, x_k[i]);
    }
    printf("Execução final com %d núcleos: %.6f segundos.\n", nucleos_teste, tempo);

    }

    return 0;
}
