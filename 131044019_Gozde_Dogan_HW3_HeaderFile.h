////////////////////////////////////////////////////////////////////////////////
// Gozde DOGAN 131044019
// Homework 3
// header dosyasi
// 
// Description:
//      Girilen directory icerisindeki her file'da yine girilen stringi aradim.
//      String sayisini ekrana yazdirdim
//      Her buldugum string'in satir ve sutun numarasini 
//      buldugum file ile birlikte log.log dosyasina yazdim.
//      Dosyaya yazarken de oncelikle aranilan stringi 
//      yazip sonrasinda file adi, satir ve sutun numarasini yazdim.
//      Her file dan sonra o file da kac tane string oldugunu da 
//      log dosyasina yazdim.
//      Yapilan fork girilen directory icindeki her directory ve file 
//      icin process olusturur. Ve icindeki directorylerin icindeki 
//      her sey icinde process olusturur.
//      Islem yapilirken directory-directory haberlesmesi fifo, 
//      file-directory haberlesmesi pipe ile saglandi.
//
// Important:
//      Kitaptaki restart.h kutuphanesi kullaildi.
//      restart.h ve restart.c dosyalari homework directory'sine eklendi.
//      tempFile.txt file'lardaki string sayisini tutan file
//      log.log ise log file!
//
// References:
//      https://gist.github.com/semihozkoroglu/737691
//      DirWalk fonksiyonunun calisma sekli icin.
//
////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>  //wait fonksiyonlari icin
#include <stdlib.h>
#include <dirent.h>     //DirWalk fonksiyonu icin
#include <string.h>
#include <errno.h>   
#include <sys/stat.h>
#include <unistd.h>     //fork icin
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include "restart.h"
 
 
#define MAXPATHSIZE 1024
#define DEBUG
#define MAXSIZE 100000
#define LOGFILE "log.log" //log dosyasi


//Function prototypes
int searchStringInFile(char *sFileName, int pipeFd);
//Genel islemlerimi topladigim bir fonksiyon
int isEmpty(FILE *file);
//Gelen dosyanin bos olup olmadigina bakar
char** readToFile(); 
//Dosyayi okuyup iki boyutlu stringe yazacak
void findLengthLineAndNumOFline();
//Dosyadaki satir sayisini ve en uzun satirin sütün sayisini hesapliyor.
int searchString(char* sFileName, char **sFile, int pipeFd);
//string iki boyutlu string icinde arayacak
int copyStr(char **sFile, char* word, int iStartRow, int iStartCol, int *iRow, int *iCol);
//1 return ederse kopyalama yapti, 0 return ederse kopyalama yapamadi
int DirWalk(const char *path, int fifo);
//fork yaparak her dosyanin icine girer
