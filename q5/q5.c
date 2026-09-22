#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUCLEOS 4 // quantidade de threads executando ao mesmo tempo
#define NUM_TAREFAS 100000

/*
Testes realizados nesse código:
valgrind --leak-check=full ./q5 => nao apontou nenhum erro na liberacao de memória alocada
valgrind --tool=helgrind ./q5 => **dubious: associated lock is not held by any thread**, que motivou o cond_signal da trabalhadora_nucleo a ser movida para antes do unlock do mutex_fila
*/
typedef struct Tarefa{

    void (*funcao)(void*); // ponteiro para a função a ser executada pela thread
    void *arg; // argumentos da tarefa
    struct Tarefa *prox;

} Tarefa;

typedef struct {

    Tarefa *inicio; 
    Tarefa *fim;

} TarefasProntas; // estabelece os limites da fila de tarefas prontas

// inicializando a fila de tarefas prontas, bem como o mutex da fila
TarefasProntas lista_pronto;
pthread_mutex_t mutex_fila;

// variáveis usadas para controlar o momento de o algoritmo finalizar
int tarefas_pendentes = 0;
pthread_cond_t cond_finalizado;

// alarme para "acordar" o escalonador
pthread_cond_t cond_escalonador;

// função agendar: adiciona uma tarefa à fila de processos prontos e acorda o escalonador (permitindo-o repousar quando a fila estiver vazia)
void Agendar(void (*funcao)(void *), void *arg){

    // alocando a nova tarefa
    Tarefa *nova = (Tarefa *)malloc(sizeof(Tarefa));
    nova->funcao = funcao;
    nova->arg = arg;
    nova->prox = NULL;

    // entrando na região crítica da fila
    pthread_mutex_lock(&mutex_fila);

    // caso a fila esteja vazia, a tarefa recém-criada será a única da fila
    if(lista_pronto.fim == NULL){

        lista_pronto.inicio = nova;
        lista_pronto.fim = nova; 

    }

    // caso não, a tarefa recém-criada deve ser a próxima e o fim da fila deve apontar para ela
    else{

        lista_pronto.fim->prox = nova;
        lista_pronto.fim = nova; 

    }

    // incrementando a quantidade de tarefas pendentes
    tarefas_pendentes++;

    // caso o escalonador esteja dormindo, é necessário acordá-lo, pois há pelo menos a tarefa recém-criada na fila
    pthread_cond_signal(&cond_escalonador);

    // a fila foi devidamente atualizada e o escalonador, sinalizado; podemos, então, destravar o mutex
    pthread_mutex_unlock(&mutex_fila);

}

// implementando um pool de threads, ou seja, um grupo de threads que serão criadas previamente (uma em cada núcleo) e, conforme as tarefas entram na fila, uma thread desocupada avoca a execução dessa tarefa para si
// essa estratégia poupa a necessidade de um mutex para controlar o núcleo, bem como de ficar criando threads para cada tarefa
void *trabalhadora_nucleo(void *arg){

    while(1){ // fica rodando em laço infinito, pois a qualquer momento pode chegar uma tarefa na fila

        // travando o mutex para acessar a fila
        pthread_mutex_lock(&mutex_fila);

        // se a fila está vazia, a thread dorme (ou seja, não há espera ocupada)
        while(lista_pronto.inicio == NULL){
            pthread_cond_wait(&cond_escalonador, &mutex_fila);
        }

        // se a fila deixou de estar na fila, tiramos o primeiro item dela
        Tarefa *tarefa_avocada = lista_pronto.inicio;
        lista_pronto.inicio = tarefa_avocada->prox;

        // se a fila, após isso, esvaziou, atualizamos o ponteiro do fim
        if(lista_pronto.inicio == NULL){
            lista_pronto.fim = NULL;
        }

        // para passar o mínimo de tempo possível com o mutex travado, destravamo-no antes de executar a tarefa
        pthread_mutex_unlock(&mutex_fila);

        // executando a função da tarefa 
        tarefa_avocada->funcao(tarefa_avocada->arg);

        // na função Agendar, alocamos memória para esse nó da fila; precisamos liberá-lo agora
        free(tarefa_avocada);

        // decrementando a quantida de tarefas pendentes e conferindo se o procesos já acabou
        pthread_mutex_lock(&mutex_fila);
        tarefas_pendentes--;
        if(tarefas_pendentes == 0){
            pthread_cond_signal(&cond_finalizado);
        }
        pthread_mutex_unlock(&mutex_fila);
    
    }
    return NULL; // embora acima haja um laço infinito, é interessante colocar o retorno, para evitar alertas do compilador e outros erros 
}

void thread_saudar(void *arg){
    int id = *(int *) arg;
    printf("Saudações! Tarefa %d executada!\n", id);
}

int main(){

    // inicializando recursos compartilhados
    pthread_mutex_init(&mutex_fila, NULL);
    pthread_cond_init(&cond_escalonador, NULL);
    pthread_cond_init(&cond_finalizado, NULL);
    lista_pronto.inicio = NULL;
    lista_pronto.fim = NULL;

    // disparando as threads de cada núcleo
    pthread_t nucleos[NUCLEOS];

    for(int i = 0; i < NUCLEOS; i++){
        pthread_create(&nucleos[i], NULL, trabalhadora_nucleo, NULL);
    }

    // simulação do agendamento de 10 tarefas 
    int ids[NUM_TAREFAS];

    printf("---Agendando 10 tarefas na fila---\n");

    for (int i = 0; i < NUM_TAREFAS; i++){
        ids[i] = i + 1;
        Agendar(thread_saudar, &ids[i]); // passando o ponteiro da função
    }

    // esperando as tarefas terminarem  
    pthread_mutex_lock(&mutex_fila); // o cond_wait precisa do mutex travado antes de ser chamado, mas, depois que o código entra nele, o mutex é liberado
    
    while(tarefas_pendentes > 0){
        pthread_cond_wait(&cond_finalizado, &mutex_fila);
    }

    // como, antes de sair do cond_wait (devido ao alarme de finalizado), o mutex foi travado de novo, precisamos destravá-lo
    pthread_mutex_unlock(&mutex_fila);

    return 0;
}
