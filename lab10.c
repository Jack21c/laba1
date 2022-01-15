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
  unsigned long blocks;
  char filename[255];
}fileinfo;

fileinfo files[n]; /* Массив структур */

sigjmp_buf obl; /* область памяти для запоминания
 состояния процесса */
int prerCount = 0; /* Cчётчик прерываний */
int prerFlag = 0; /* флаг 5 прерываний */
void prer(); /* подпрограмма обработки прерывания */
unsigned long int totalBlockCount = 0;

int main(){
    int s;
    int fileCount = 0; /* Счетчик количества файлов */
    int file4blocksCount = 0; /* Счетчик количества файлов,
    состоящих из более чем 4 блоков */
    char pathName[PATH_MAX]; /*буфер, в который будет
    помещен путь к текущей директории*/
    fileinfo buffer;

    int end_flag; /* флаг завершения работы программы*/
    int descr[2]; /* дескрипторы межпроцессного канала */
    signal(SIGINT, prer); /* уведомление о том, что
    в случае прихода сигнала прерывания SIGINT,
    управление передается процедуре prer */
    for(int i = 0; i < n; i++){ /* Инициализируем структуру */
        files[i].blocks = 0;
        files[i].filename[0] = '\0';
    }
    if(getcwd(pathName, sizeof(pathName)) != NULL){
    /* Если удалось получить путь к директории */
        sigsetjmp(obl, 1);
        DIR *d; /* Инициализируем структуру DIR */
        struct dirent *dir;
        d = opendir(pathName); /* Открываем директорию по имени */
        if(d){
            while (dir = readdir(d)) {
            /* Если директория открылась итерируем по ней до конца */
                struct stat fileStat; /* Создаем структуру
                статистики файла */
                if(stat(dir->d_name, &fileStat) == -1){
                /* Получаем статистику по файлу */
                    perror("stat");
                    return 1;
                }
                else if(dir->d_name[0]!='.'){
                /* В случае успешного получения... */
                    files[fileCount].blocks = fileStat.st_blocks; /* ...
                    записываем размер в блоках... */
                    strncpy(files[fileCount].filename, dir->d_name, 254);
                    /* ...и имя файла */
                    fileCount++; /*Считаем количество файлов*/
                    totalBlockCount += fileStat.st_blocks; /* Считаем
                    полное количество блоков */
                    if(fileStat.st_blocks > 4){ /* Считаем количество
                      файлов больше 4 блоков */
                        file4blocksCount++;
                    }
                }
            }
            closedir(d); /* Закрываем директорию */
            sigsetjmp(obl, 1);
            pipe(descr); /* Создаем межпроцессный канал */
            if(fork() == 0){ /* Распараллеливание процесса */
            /* Процесс-потомок*/
                close(descr[0]); /* Закрываем межпроцессный канал
                на чтение... */
                for(int i = 0; i < fileCount; i++){ /* ...и записываем
                в него информацию */
                    write(descr[1], &files[i].filename,
                      sizeof(files[i].filename));
                    write(descr[1], &files[i].blocks,
                      sizeof(files[i].blocks));
                }
                exit(1);
            }
            else{
                wait(&s); /* Ожидаем окончания процесса-потомка */
                sigsetjmp(obl, 1);
                close(descr[1]); /* Закрываем канал на запись */
                for(int i = 0; i < file4blocksCount; i++){ /* Считываем
                информацию из потока */
                    read(descr[0], &buffer.filename,
                      sizeof(buffer.filename));
                    read(descr[0], &buffer.blocks,
                      sizeof(buffer.blocks));
                    if(buffer.blocks > 4){ /* Вывод файлов более 4 блоков */
                        printf("%s: %lu\n",
                          buffer.filename, buffer.blocks);
                    }
                    sigsetjmp(obl, 1);
                }
            }
        }
    }
    else{
        perror("getcwd() error");
        return 1;
    }
    sleep(3);
    sigsetjmp(obl, 1);
    return 0;

}
/* Подпрограмма обработки прерывания */
void prer()
{
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
    siglongjmp(obl, 1); /* возвращение на последний setjmp */
}
