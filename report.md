# PAGINADOR DE MEMÓRIA - RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

Preencha as linhas abaixo com o nome e o e-mail dos integrantes do
grupo.  Substitua marcadores `XX` pela contribuição de cada membro
do grupo no desenvolvimento do trabalho (os valores devem somar
100%).

  * Fillipe de Oliveira Almeida <fillipeoa1234@gmail.com> 50%
  * Alan Augusto Martins Campos <augustoalan56@gmail.com> 50%

3. Referências bibliográficas
    https://www.certificacaolinux.com.br/sinais-no-linux-guia-basico/
    https://www.geeksforgeeks.org/multithreading-in-c/
    https://embarcados.com.br/threads-posix/

4. Estruturas de dados

  1. Descreva e justifique as estruturas de dados utilizadas para
     gerência das threads de espaço do usuário (partes 1, 2 e 5).

      As estruturas de dados utilizadas para a gerência das threads de espaço do usuário no código são:

      a. dccthread_t: estrutura principal que representa uma thread. Ela contém um contexto de execução, um nome e flags para indicar se a thread terminou ou está dormindo. Além disso, ela contém um identificador de temporizador que é usado para acordar a thread após um período de tempo especificado.

      b. Lista de threads prontas (ready_threads): uma lista duplamente encadeada (implementada usando a biblioteca dlist) que contém todas as threads que estão prontas para serem executadas. Quando uma nova thread é criada, ela é adicionada a esta lista. Quando uma thread termina ou cede a CPU, ela é removida desta lista.

      c. Threads especiais (main_thread, current_thread, manager_thread): Estas são instâncias da estrutura dccthread_t que representam a thread principal do programa, a thread atualmente em execução e a thread gerente, respectivamente.

      A escolha dessas estruturas de dados é justificada pela necessidade de gerenciar múltiplas threads de execução em um programa. A estrutura dccthread_t permite encapsular todas as informações necessárias sobre uma thread em um único objeto. A lista de threads prontas permite manter um registro de todas as threads que estão prontas para serem executadas, e facilita a troca entre elas. As threads especiais são necessárias para o funcionamento básico do sistema de threads: a thread principal é necessária para iniciar o programa, a thread atual é necessária para saber qual thread está atualmente em execução, e a thread gerente é necessária para controlar quando e como as threads são escalonadas.

  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).

      O mecanismo de sincronização utilizado para coordenar as chamadas de dccthread_yield e os  disparos do temporizador é baseado em sinais do sistema operacional e na função sigprocmask.

      A função sigprocmask é usada para bloquear e desbloquear o sinal SIGALRM, que é gerado quando o temporizador dispara. Isso é feito para evitar que o sinal seja recebido durante a troca de contexto entre as threads, o que poderia causar inconsistências.

      Quando a função dccthread_yield é chamada, o sinal SIGALRM é bloqueado usando sigprocmask(SIG_BLOCK, &mask, NULL). Isso impede que o temporizador interrompa a execução enquanto a thread atual está sendo retirada da CPU e a próxima thread está sendo escolhida para execução. Depois que a próxima thread foi escolhida e o contexto foi trocado, o sinal SIGALRM é desbloqueado usando sigprocmask(SIG_UNBLOCK, &mask, NULL), permitindo que o temporizador interrompa a execução novamente.

      Da mesma forma, quando o temporizador dispara e a função timer_function é chamada, a função dccthread_yield é chamada para retirar a thread atual da CPU. Novamente, o sinal SIGALRM é bloqueado durante esse processo para evitar interrupções.

      Essa abordagem garante que as chamadas de dccthread_yield e os disparos do temporizador sejam sincronizados de maneira segura. Isso evita condições de corrida e garante que apenas uma thread esteja em execução em um determinado momento.

      Além disso, na função dccthread_sleep, um temporizador separado é configurado para disparar após um tempo especificado. Quando esse temporizador dispara, ele envia um sinal SIGUSR1 que é capturado pela função wake_up_thread. A função wake_up_thread então “acorda” a thread definindo o campo sleep da thread para 0. Isso permite que uma thread “durma” por um período de tempo especificado e seja “acordada” quando esse tempo expirar. O uso deste temporizador separado permite que as threads sejam colocadas em um estado de espera sem bloquear outras threads.
