#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define NUM_CONSULTORIOS 5
#define LINHA_CABECALHO 3
#define TEMPO_VISIVEL 2

// struct principal que representa o consultorio
typedef struct {
    pthread_mutex_t mutex;  //trava
    char paciente_atual[100];
} Consultorio;

Consultorio consultorios[NUM_CONSULTORIOS]; //array de consultorios, cria travas INDEPENDENTES
pthread_mutex_t mutex_impressao = PTHREAD_MUTEX_INITIALIZER; //trava para escrita fisica no terminal

void atualizar_linha(int idx, const char *nome_paciente, int thread_id) {
    pthread_mutex_lock(&mutex_impressao);
// sequencias ansi, move o cursos pra onde ele deve mexer e tamvem limpa da posicao do cursor ate o final da linha, para evitar lixo de impressoes anteriores
    int linha_tela = LINHA_CABECALHO + idx;
    printf("\033[%d;1H", linha_tela);
    printf("\033[K");
    printf("Consultorio %d: %-30s (atendido por thread %d)",
           idx + 1, nome_paciente, thread_id);
    fflush(stdout);

    pthread_mutex_unlock(&mutex_impressao);
}
void desenhar_tabela_inicial() {
    printf("\033[2J");
    printf("\033[H");
    printf("TABELA DE CHAMADA DE PACIENTES\n\n");

    for (int i = 0; i < NUM_CONSULTORIOS; i++) {
        printf("Consultorio %d: %-30s\n", i + 1, "Livre");
    }
    fflush(stdout);
}
// Dados que cada thread recebe: o nome do arquivo que ela deve ler
// e um id apenas para fins de identificacao nas mensagens.
typedef struct {
    char nome_arquivo[256];
    int thread_id;
} DadosThread;

// Funcao executada por cada thread: le seu arquivo linha por linha
// e processa cada paciente sequencialmente.
void *processar_arquivo(void *arg) {
    DadosThread *dados = (DadosThread *) arg;

    FILE *arquivo = fopen(dados->nome_arquivo, "r");
    if (arquivo == NULL) {
        pthread_mutex_lock(&mutex_impressao);
        printf("\033[%d;1H[Thread %d] ERRO ao abrir arquivo %s\n",
               LINHA_CABECALHO + NUM_CONSULTORIOS + dados->thread_id,
               dados->thread_id, dados->nome_arquivo);
        fflush(stdout);
        pthread_mutex_unlock(&mutex_impressao);
        return NULL;
    }

    char linha[200];
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char nome[100];
        int consultorio;

        int lidos = sscanf(linha, "%99[^;];%d", nome, &consultorio);
        if (lidos != 2) {
            continue;
        }

        if (consultorio < 1 || consultorio > NUM_CONSULTORIOS) {
            continue;
        }

        int idx = consultorio - 1;

        pthread_mutex_lock(&consultorios[idx].mutex);

        strncpy(consultorios[idx].paciente_atual, nome, sizeof(consultorios[idx].paciente_atual) - 1);
        atualizar_linha(idx, nome, dados->thread_id);

        sleep(TEMPO_VISIVEL);

        pthread_mutex_unlock(&consultorios[idx].mutex);
    }

    fclose(arquivo);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s arquivo1.txt arquivo2.txt ...\n", argv[0]);
        printf("Cada arquivo passado vira uma thread (um \"guiche\" de chamadas).\n");
        return 1;
    }

    int num_threads = argc - 1;

    for (int i = 0; i < NUM_CONSULTORIOS; i++) {
        pthread_mutex_init(&consultorios[i].mutex, NULL);
        strcpy(consultorios[i].paciente_atual, "Livre");
    }

    desenhar_tabela_inicial();

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    DadosThread *dados = malloc(num_threads * sizeof(DadosThread));

    for (int i = 0; i < num_threads; i++) {
        strncpy(dados[i].nome_arquivo, argv[i + 1], sizeof(dados[i].nome_arquivo) - 1);
        dados[i].thread_id = i;
        pthread_create(&threads[i], NULL, processar_arquivo, &dados[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\033[%d;1H\n", LINHA_CABECALHO + NUM_CONSULTORIOS + num_threads + 2);
    printf("Todas as chamadas foram processadas.\n");

    for (int i = 0; i < NUM_CONSULTORIOS; i++) {
        pthread_mutex_destroy(&consultorios[i].mutex);
    }
    free(threads);
    free(dados);

    return 0;
}