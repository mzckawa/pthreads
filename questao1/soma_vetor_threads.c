#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TAMANHO_VETOR 10000

// struct que sera passada para cada thread (via void*)
// Cada thread recebe seu proprio "pedaco" de trabalho
typedef struct {
    int *vetor;         // ponteiro para o vetor original 
    int inicio;         // indice inicial do pedaco (inclusivo)
    int fim;             // indice final do pedaco (exclusivo)
    long soma_parcial;   // resultado calculado por essa thread
    int id;              // apenas para identificar a thread nos prints
} DadosThread;

// ssinatura obrigatoria do pthreads: recebe void*, retorna void*
void *somar_pedaco(void *arg) {
    DadosThread *dados = (DadosThread *) arg; // cast de volta para o tipo real
    dados->soma_parcial = 0;
    for (int i = dados->inicio; i < dados->fim; i++) {
        dados->soma_parcial += dados->vetor[i];
    }

    printf("Thread %d somou o intervalo [%d, %d) = %ld\n",
           dados->id, dados->inicio, dados->fim, dados->soma_parcial);

    return NULL; // nao precisamos devolver nada pelo retorno da thread
}

int main(int argc, char *argv[]) {
    int num_threads = 4; // valor padrao

    // ermite passar o numero de threads como argumento: ./programa 8
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads <= 0) {
            printf("Numero de threads invalido, usando 4.\n");
            num_threads = 4;
        }
    }

    // cria e preenche o vetor grande com valores de exemplo (1 a TAMANHO_VETOR)
    int *vetor = malloc(TAMANHO_VETOR * sizeof(int));
    for (int i = 0; i < TAMANHO_VETOR; i++) {
        vetor[i] = i + 1; // valores 1, 2, 3, ..., 10000
    }

    // 2) aloca os arrays de threads e de dados (um struct por thread)
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    DadosThread *dados = malloc(num_threads * sizeof(DadosThread));

    // 3) calcula o tamanho de cada pedaco
    //    Se TAMANHO_VETOR nao for divisivel igualmente, a ultima thread
    //    fica com o restante (resto da divisao)
    int tamanho_pedaco = TAMANHO_VETOR / num_threads;

    // 4) Ccia as threads, cada uma cuidando de um pedaco do vetor
    for (int i = 0; i < num_threads; i++) {
        dados[i].vetor = vetor;
        dados[i].id = i;
        dados[i].inicio = i * tamanho_pedaco;

        // a ultima thread pega tudo que sobrou ate o fim do vetor
        if (i == num_threads - 1) {
            dados[i].fim = TAMANHO_VETOR;
        } else {
            dados[i].fim = (i + 1) * tamanho_pedaco;
        }

        // pthread_create(referencia_da_thread, atributos, funcao, argumento)
        int erro = pthread_create(&threads[i], NULL, somar_pedaco, &dados[i]);
        if (erro != 0) {
            printf("Erro ao criar thread %d\n", i);
            exit(1);
        }
    }

    // 5) espera todas as threads terminarem antes de somar os parciais
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // 6) soma sequencial dos resultados parciais (etapa final, feita pela main)
    long soma_total = 0;
    for (int i = 0; i < num_threads; i++) {
        soma_total += dados[i].soma_parcial;
    }

    printf("\nSoma total (com threads) = %ld\n", soma_total);

    // 7) verificacao: soma sequencial simples, para conferir se o resultado bate
    long soma_verificacao = 0;
    for (int i = 0; i < TAMANHO_VETOR; i++) {
        soma_verificacao += vetor[i];
    }
    printf("Soma total (verificacao sequencial) = %ld\n", soma_verificacao);

    // 8) liberar mallocs
    free(vetor);
    free(threads);
    free(dados);

    return 0;
}
