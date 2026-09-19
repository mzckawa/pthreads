#include <string.h>
#include <stdio.h> 
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define LEITORAS 3 
#define ESCRITORAS 3

int dados[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

// mutex e variáveis de condição
pthread_mutex_t mutex;
pthread_cond_t pode_ler;
pthread_cond_t pode_escrever;

// variáveis de estado
int leitoras_ativas = 0;
int escritora_ativa = 0;

// função de leitura 
void *leitora(void *arg){

    int id = *((int *) arg); // identificador da thread, fornecido pela main

    free(arg); // liberando o bloco alocado, na main, pelo malloc

    while (1){ // para o laço ser infinito

        pthread_mutex_lock(&mutex);

        // verificando se há alguma escritora em ação; se houver, a leitora dorme
        while(escritora_ativa != 0){
            pthread_cond_wait(&pode_ler, &mutex);
        }

        // quando a escritora sai, a leitora é acordada e deve ser contada entre o número de threads leitoras
        leitoras_ativas++;
        printf("Leitora %d em ação! Há %d leitoras ativas no momento.\n", id, leitoras_ativas);

        // agora que o número de leitoras foi devidamente atualizado, podemos permitir a entrada de outras leitoras liberando o mutex
        pthread_mutex_unlock(&mutex);

        // lendo o array (definição da região crítica)
        int pos = rand() % 20; // sorteando uma posição aleatória do array para ler 
        int valor = dados[pos]; // armazenando o valor lido
        printf("Leitora %d leu que dados[%d] = %d.\n", id, pos, valor);

        sleep(2); // pequena pausa, para melhor visualização das atividades das threads

        // feita a leitura, devemos travar o mutex e subtrair nossa leitora das ativas
        pthread_mutex_lock(&mutex);

        leitoras_ativas--;
        int  copia_leitoras_ativas = leitoras_ativas;

        // em seguida, precisamos conferir se essa foi a última leitura; nesse caso, a escrita estará liberada
        if(leitoras_ativas==0){
            pthread_cond_broadcast(&pode_escrever);
        }

        pthread_mutex_unlock(&mutex);

        if (copia_leitoras_ativas == 0){ // colocando depois do unlock para esse print não atrasar a concorrência
            printf("copia_leitoras_ativas = %d. Escrita liberada!\n", copia_leitoras_ativas);
        }

        sleep(2); 

    }

    return NULL;
}

void *escritora(void *arg){ // estrutura análoga à leitora

    int id = *((int *) arg);

    free(arg);

    while(1){

        pthread_mutex_lock(&mutex);

        // verificando se há alguma leitora ou outra escritora em ação
        while(leitoras_ativas>0 || escritora_ativa !=0){
            pthread_cond_wait(&pode_escrever, &mutex);
        }

        // se saiu do laço, é porque pode escrever 
        escritora_ativa = 1;

        // atualizadas as variáveis de estado, destravamos o mutex
        pthread_mutex_unlock(&mutex);

        // sorteando uma posição do array e um valor para nele escrever
        int pos = rand() % 20;
        int antigo_valor = dados[pos];
        int novo_valor = rand() % 20;

        // escrevendo no array
        dados[pos] = novo_valor;

        printf("Escritora %d em ação! Alterei o valor de dados[%d] de %d para %d.\n", id, pos, antigo_valor, novo_valor);

        sleep(2);

        // feita a escrita, travamos novamente o mutex, resetamos escritora_ativa e liberamos a leitura e a escrita novamente 
        pthread_mutex_lock(&mutex);
        escritora_ativa = 0;
        pthread_cond_broadcast(&pode_ler);
        pthread_cond_broadcast(&pode_escrever);
        printf("escritora_ativa = %d. Escrita e leitura liberadas!\n", escritora_ativa);

        pthread_mutex_unlock(&mutex);

        sleep(2);

    }

    return NULL;
}


int main(int argc, char *argv[]){

    // inicializando mutex e variáveis de condição
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&pode_ler, NULL);
    pthread_cond_init(&pode_escrever, NULL);

    pthread_t th_leitoras[LEITORAS];
    pthread_t th_escritoras[ESCRITORAS];

    int *ids_leitoras[LEITORAS];
    int *ids_escritoras[ESCRITORAS];

    int rc;

    for(int i = 0; i<LEITORAS; i++){

        ids_leitoras[i] = (int*) malloc(sizeof(int));
        *ids_leitoras[i] = i;
        rc = pthread_create(&th_leitoras[i], NULL, leitora, ids_leitoras[i]);
        printf("Leitora %d criada!\n", i);

        if(rc){
            printf("ERRO; código de retorno é %d\n", rc);
            exit(-1);
        }

    }

    for(int i = 0; i<ESCRITORAS; i++){

        ids_escritoras[i] = (int*) malloc(sizeof(int));
        *ids_escritoras[i] = i;
        rc = pthread_create(&th_escritoras[i], NULL, escritora, ids_escritoras[i]);
        printf("Escritora %d criada!\n", i);

        if(rc){
            printf("ERRO; código de retorno é %d\n", rc);
            exit(-1);
        }

    }

    // mantendo a main rodando enquantos as threads trabalham
    for(int i = 0; i<LEITORAS; i++){
        pthread_join(th_leitoras[i], NULL);
    }

    for(int i = 0; i<ESCRITORAS; i++){
        pthread_join(th_escritoras[i], NULL);
    }

    return 0;

}
