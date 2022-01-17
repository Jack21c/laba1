#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>



sigjmp_buf obl; /* область памяти для запоминания
 состояния процесса */
int prerCount = 0; /* Cчётчик прерываний */
void prer(int sig); /* подпрограмма обработки прерывания */
unsigned long long int totalBlockCount = 0; /* Общее количество блоков */

int main(){
    int s;
    struct sigaction sa; /* Создаём sigaction и заполняем поля */
    sigset_t set;
    sigemptyset(&set);
    sigprocmask(0, 0, &set);
    sa.sa_mask = set;
    sa.sa_flags = 0;
    sa.sa_handler = prer;
    sigaction(SIGINT, &sa, NULL);

    int descr[2]; /* дескрипторы межпроцессного канала */
    int end_flag = 0; /* флаг завершения работы программы*/
    sigsetjmp(obl, 1);

    pipe(descr); /* Создаем межпроцессный канал */
    if(fork() == 0){ /* Распараллеливание процесса */
    /* Процесс-потомок*/
        close(descr[0]); /* Закрываем межпроцессный канал
        на чтение... */
        dup2(descr[1], 1); /* Дублируем дескриптор межпроцессного канала
        на стандартный вывод */
        execl("/usr/bin/du", "du", "-a",  NULL); /* вывод информации о каждом файле
        текущего каталога в межпроцессный канал */
    }
    else{
        wait(&s); /* Ожидаем окончания процесса-потомка */
        printf("%d\n", s);
        close(descr[1]); /* Закрываем канал на запись */
        close(0);
        dup2(descr[0], 0); /* Дублируем дескриптор межпроцессного канала
        на стандартный ввод */
        int blocks;
        char filename[255];
        while(scanf("%d%s", &blocks, filename) != EOF){
        /* Считываем информацию из межпроцессорного канала */
            if((blocks > 4) && (strcmp(filename, ".") != 0)){
            /* Выводим файлы более 4 блоков */
                printf("%s: %d\n", filename, blocks);
                sigsetjmp(obl, 1);
            }
            sleep(1);
        }
    }

    sigsetjmp(obl, 1);
    return 0;
}
/* Подпрограмма обработки прерывания */
void prer(int sig)
{
    int c;
    prerCount++;
    if(prerCount%5 == 0){ /* Выводим общее количество блоков
    при каждом пятом прерывании */
        //printf("5 interruption start\n");
        int descr2[2];
        pipe(descr2);
        if(fork() == 0){
            close(descr2[0]);
            dup2(descr2[1], 1);

            //system("du -s");
            //exit(1);
            execl("/usr/bin/du", "du", "-s",  NULL);
        }
        else{
            wait(&c);
            close(descr2[1]);
            close(0);
            dup(descr2[0]);
            int totalBlocks;
            char rand1[80];
            char rand2;
            read(descr2[0], rand1, 80);
            totalBlocks = atoi(rand1);
            //scanf("%d%c", &totalBlocks, &rand2);
            printf("Total blocks: %d\n", totalBlocks);


        }
        //system("du -s | awk \'{print $1}\'");
        //printf("5 interruption end\n");

        //printf("%llu\n", totalBlockCount);

    }
    /*if(prerCount == 9){
        sleep(3);
        exit(0);
    }*/
    siglongjmp(obl, 1); /* возвращаемся на последний setjmp */
}
