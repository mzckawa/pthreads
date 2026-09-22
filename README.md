# Pthreads — Projeto 1 de Sistemas Operacionais

## Questão 1 — Soma de vetor dividida entre N threads

O problema aqui é simples de enunciar mas serve de base pra tudo o resto: temos um vetor de 10.000 inteiros e queremos somar ele mais rápido usando várias threads em vez de uma soma sequencial só.

A ideia foi dividir o vetor em pedaços do mesmo tamanho (um pedaço por thread) e deixar cada thread responsável por somar só o pedaço dela, guardando o resultado numa variável própria. Como cada thread escreve em uma posição de memória diferente (sua própria struct de dados), não existe disputa nenhuma entre elas — por isso essa questão não usa mutex. A thread principal só entra em ação depois que todas as outras terminaram (usando `pthread_join` pra garantir isso), e aí soma os resultados parciais numa soma final sequencial simples.

Se o número de threads não divide o vetor de forma exata, a última thread fica responsável pelo resto da divisão, para não perder nenhum elemento.

O programa também faz uma soma sequencial de verificação no final, só para provar que o resultado bate com o método paralelo.

### Como rodar

```bash
gcc soma_vetor_threads.c -o soma_vetor_threads -lpthread
./soma_vetor_threads          # usa 4 threads por padrão
./soma_vetor_threads 8        # ou define o número de threads
```

---

## Questão 2 — Tabela de chamada de pacientes

Essa questão já é mais interessante porque mistura dois problemas: sincronização entre threads e manipulação de terminal.

**O cenário:** existem N "guichês" de atendimento, cada um representado por um arquivo de texto com uma lista de pacientes e o consultório pra onde cada um deve ser chamado. Cada guichê vira uma thread, que lê seu arquivo linha por linha e vai chamando os pacientes, atualizando uma tabela na tela em tempo real.

**Decisão principal: mutex por linha, não mutex global.** Se usássemos um mutex único pra tabela inteira, threads mexendo em consultórios diferentes ficariam bloqueando umas às outras sem necessidade nenhuma — e o enunciado pede exatamente o contrário: linhas diferentes podem ser atualizadas simultaneamente, só a mesma linha precisa de exclusão mútua. A solução foi criar um array de structs, uma por consultório, cada uma carregando seu próprio `pthread_mutex_t`. Assim, travar o consultório 1 não tem nenhum efeito sobre o consultório 4.

Depois que uma thread consegue a trava de um consultório, ela atualiza o nome do paciente e mantém a linha bloqueada por alguns segundos (`sleep`) antes de liberar — isso garante que a mudança fique visível na tela por tempo suficiente, e que a mesma linha não seja sobrescrita instantaneamente por outra chamada.

**Atualizando só uma linha, sem redesenhar a tabela:** pra isso usamos sequências de escape ANSI, que são comandos especiais que dá pra mandar pro terminal via `printf`. `\033[<linha>;1H` move o cursor pra uma linha específica, e `\033[K` limpa o conteúdo antigo daquela linha antes de escrever o novo. Combinando os dois, conseguimos reescrever só o consultório que mudou, sem afetar o resto da tabela.

**Um mutex extra, separado dos mutexes de consultório:** como duas threads podem estar liberadas pra rodar ao mesmo tempo (porque mexem em consultórios diferentes), existe o risco dos `printf`s delas se intercalarem no terminal e o texto sair embaralhado. Pra resolver isso, existe um mutex adicional (`mutex_impressao`) que protege só o ato físico de escrever na tela — é um lock bem mais curto que o dos consultórios, usado só durante os prints.

**Leitura dos arquivos:** cada linha do arquivo de pacientes segue o formato `Nome;Consultorio` (ex: `Maria Silva;1`). Usamos `sscanf` com o formato `"%99[^;];%d"` pra separar o nome (tudo antes do `;`) do número do consultório, e ignoramos silenciosamente linhas mal formatadas ou com número de consultório fora do intervalo válido.

### Como rodar

```bash
gcc pthreadsq2.c -o pthreadsq2 -lpthread
./pthreadsq2 pacientes1.txt pacientes2.txt
```

Cada arquivo passado como argumento vira uma thread/guichê. Os arquivos de exemplo seguem o formato `Nome;Consultorio`, um paciente por linha.

---

## Questão 3 — Leitura e scrita de array com pthreads 

Aqui, a proposta é implementarmos um sistema de leitura e modificação de um vetor de maneira concorrente, evitando condições de disputa. Assim, é permitido que várias threads leitoras acessem o array, enquanto as escritoras precisam atuar completamente sozinhas. 

Para atingir esses requisitos, nós utilizamos os seguintes recursos compartilhados: um mutex que controla a entrada de escritoras no array, uma variável de condição para autorizar leitura, outra para autorizar a escrita e, claro, o vator de dados. 

Note que a separação de variáveis de condição entre "canais de comunicação" separados (isto é, um sinal para acordar somente as leitoras e outro para acordar somente as escritoras) é essencial para usufruirmos da performance das pthreads. 

Ademais, como as variáveis de condição não tem memória, precisamos também de variáveis de estado, leitoras_ativas e escritora_ativa. Aquela serve para contabilizar quantas escritoras estão acessando o array simultaneamente e, quando zerar, mandar o sinal às leitoras, permitindo a escrita. Esta, por sua vez, faz as leitoras esperarem enquanto seu valor é 1, e ativa o sinal pode_ler quando é zero. 

Essa dinâmica é descrita em duas funções, "leitora" e "escritora". Além disso, na main, simulamos a atividade gerando valores aleatórios para serem os índices acessados e os valores alterados. 

### Como rodar

```bash
gcc q3.c -o q3 -lpthread
./q3
```

---

## Questão 4 - Aplicação concorrente do método de Jacobi 

Nessa questão, exercitamos os conceitos de pthreads e barreiras por meio da implementação do método de Jacobi, utilizado para aproximar soluções de sistemas lineares. De forma sucinta, o método estima uma solução inicial e, para cada vetor x_k, é aplicado um algoritmo matemático que gera o vetor x_(k+1). 

Encontramos a oportunidade de paralelização ao entendermos que os elementos de um mesmo vetor podem ser calculados de maneira independente entre si. 

Além disso, o conceito-chave dessa questão é o uso de barreiras: como os cálculos de cada elemento de x_(k+1) não necessariamente ocorrem ao mesmo tempo e não é possível avançar para a computação de x_(k+2) sem haver finalizado x_(k+1), é imperativo forçar as threads que terminaram seu trabalho antes a esperar as demais fazê-lo também (sincronização) - e, para tal, utilizamos barreiras. 

Como recursos compartilhados, temos os que descrevem o sistema linear (incógnitas, descritas pela matriz A, termos independentes, descritos pelo vetor b, e vetores de solução x_k e x_kmais1), a barreira e o vetor de dados. 

A respeito das estruturas de dados, temos DadosThread, que organiza as incógnitas a serem processadas pelas threads (que são inicializadas com os valores da estimativa de solução inicial) e timespec, uma struct que nos auxiliou na avaliação de desempenho dos diferentes níveis de paralelização. 

A respeito das funções, temos a que aplica o algoritmo de Jacobi (calculo_jacobi) e a função que cria threads com base na quantidade de núcleos desejada. Para facilitar a correção do exercício, a simulação da escolha de núcleos pelo usuário já está embutida no código: ao rodar o executável, o mesmo cálculo é feito com 1, 2 e 4 threads, e o tempo de execução de cada situação é exibido ao final. 

Finalmente, como dito no início do arquivo .c, a superioridade dos quatro núcleos costuma vigorar para sistemas grandes, de centenas de linhas. Isso se deve ao fato de que a criação de threads e barreiras ocupa bem mais a CPU do que as contas do método em si, de forma que, em sistemas menores, o "ponto ótimo" de performance acaba sendo com 2, e não 4, núcleos.

### Como rodar

```bash
gcc q4.c -o q4 -lpthread
./q4
```


