# Pthreads — Projeto de Sistemas Operacionais

## Questão 1 — Soma de vetor dividida entre N threads

O problema aqui é simples de enunciar mas serve de base pra tudo o resto: temos um vetor de 10.000 inteiros e queremos somar ele mais rápido usando várias threads em vez de uma soma sequencial só.

A ideia foi dividir o vetor em pedaços do mesmo tamanho (um pedaço por thread) e deixar cada thread responsável por somar só o pedaço dela, guardando o resultado numa variável própria. Como cada thread escreve em uma posição de memória diferente (sua própria struct de dados), não existe disputa nenhuma entre elas — por isso essa questão não usa mutex. A thread principal só entra em ação depois que todas as outras terminaram (usando `pthread_join` pra garantir isso), e aí soma os resultados parciais numa soma final sequencial simples.

Se o número de threads não divide o vetor de forma exata, a última thread fica responsável pelo resto da divisão, pra não perder nenhum elemento.

O programa também faz uma soma sequencial de verificação no final, só pra provar que o resultado bate com o método paralelo.

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