#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define n 100

typedef struct { /* Структура для хранения ирформации о файлах */
  int blocks;
  char filename[255];
}fileinfo;

fileinfo buffer; /* Массив структур */

sigjmp_buf obl; /* область памяти для запоминания
 состояния процесса */
int prerCount = 0; /* Cчётчик прерываний */
int prerFlag = 0; /* флаг 5 прерываний */
void prer(int); /* подпрограмма обработки прерывания */
unsigned long int totalBlockCount = 0;

int main(){
    int s;
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = prer;
    sigaction(SIGINT, &sa, NULL);
    char pathName[PATH_MAX]; /*буфер, в который будет
    помещен путь к текущей директории*/
    int descr[2]; /* дескрипторы межпроцессного канала */
    int end_flag = 0; /* флаг завершения работы программы*/
    do{
        pipe(descr); /* Создаем межпроцессный канал */
        if(fork() == 0){ /* Распараллеливание процесса */
        /* Процесс-потомок*/
            close(descr[0]); /* Закрываем межпроцессный канал
            на чтение... */
            dup2(descr[1], 1);
            //close(descr[1]);
            getcwd(pathName, sizeof(pathName));
            execl("/bin/du", "/bin/du", "-a", pathName, NULL);
        }
        else{
            wait(&s); /* Ожидаем окончания процесса-потомка */
            sigsetjmp(obl, 1);
            close(descr[1]); /* Закрываем канал на запись */
            scanf("%d%s", &buffer.blocks, buffer.filename);
            sigsetjmp(obl, 1);
            while(strcmp(buffer.filename, "")){
                if(strcmp(buffer.filename, ".") == 0){
                    totalBlockCount = buffer.blocks;
                    end_flag = 1;
                    break;
                }
                if(buffer.blocks > 4){ /* Вывод файлов более 4 блоков */
                    printf("%s: %d\n", buffer.filename, buffer.blocks);
                }
                scanf("%d%s", &buffer.blocks, buffer.filename);
                sigsetjmp(obl, 1);
                sleep(1);
            }
        }
    }
    while(end_flag != 1);
    sigsetjmp(obl, 1);
    return 0;
}
/* Подпрограмма обработки прерывания */
void prer(int sig)
{
    if(sig == SIGINT){
        prerCount++;
        if(prerCount%5 == 0){
            prerFlag = 1;
            printf("%lu\n", totalBlockCount);
        }
        else{
            prerFlag = 0;
        }
        /*if(prerCount == 9){
            sleep(3);
            exit(0);
        }*/
    }
    siglongjmp(obl, 1); /* возвращение на последний setjmp */
}
