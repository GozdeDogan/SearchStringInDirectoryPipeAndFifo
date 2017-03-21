////////////////////////////////////////////////////////////////////////////////
// Gozde DOGAN 131044019
// Homework 3
// c dosyasi
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
//      DirWalk fonksiyonunun calisma sekli icin bu siteden yararlanildi.
//
////////////////////////////////////////////////////////////////////////////////

#include "131044019_Gozde_Dogan_HW3_HeaderFile.h"

int iNumOfLine = 0;
int iLengthLine = 0;

FILE *fPtrInFile = NULL;

char *sSearchStr = NULL;
int iSizeOfSearchStr = 0;

FILE *fPtr;

int main(int argc, char *argv[]){
    int fifo = 0; //ana fifo 
    char *s; // fifoya yazialacak seyleri bu string'e atip yazdim
    int i=0; // tempFile.txt'den okuma yapabilmek icin kullandim
    int iWordCount = 0; //toplam kelime sayisini bulmak icin kullandim
    char path[MAXPATHSIZE];
    
    fPtr = fopen("temp.txt", "w+"); //file'lardaki string sayisinin yazildigi file
   
    if (argc != 3) {
        printf ("Usage>> \n");
        printf("./withPipeAndFifo \"searchString\" <directoryName>\n");
        return 1;
    }
   
    iSizeOfSearchStr = strlen(argv[1]);
    if(argv[1] != NULL)
        sSearchStr = (char*)calloc((strlen(argv[1])+1), sizeof(char));
    strncpy(sSearchStr, argv[1], ((int)strlen(argv[1])+1));
    
    getcwd(path, MAXPATHSIZE);
    strcat(path, "/temp.txt");
   
    fifo = open(LOGFILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    
    iWordCount = DirWalk(argv[2], fifo);
    
    rewind(fPtr);
    while(!feof(fPtr)){
        fscanf(fPtr, "%d", &i);
        iWordCount += i;
    }
    iWordCount -= i; //1 tane fazladan okuyor
    
    //printf("path:%s\n", path);
    //printf("remove : %d \n", remove(path));
    remove(path);
    
    //print screen 
    printf("\n****************************************************\n");
    printf("  %d %s were found in total in %s directory!\n", iWordCount, argv[1], argv[2]);
    printf("****************************************************\n\n"); 
    
    s = (char*)calloc(MAXSIZE, sizeof(char));
    
    //print log
    sprintf(s, "\n****************************************************\n");
    write(fifo, s, strlen(s));
    sprintf(s, "  %d %s were found in total in %s directory!\n", iWordCount, argv[1], argv[2]);
    write(fifo, s, strlen(s));
    sprintf(s, "****************************************************\n\n");
    write(fifo, s, strlen(s));
    
    free(sSearchStr);
    close(fifo);
    fclose(fPtr);
    free(s);
    return 0;
}

//Function Definitions
int DirWalk(const char *dir_name, int fifo){
    
    DIR *dir;
    struct dirent *mdirent = NULL; //recursive degisen directory
    struct stat buf; //file ve dir bilgisi ogrenmek icin

    //variabless
    pid_t pid; //forktan donen deger
    
    char fname[MAXPATHSIZE]; //file isimleri bulunduklari path isimlerinden ayri tutar
    char nameDirFile[MAXPATHSIZE];
    
    char PathFifo[MAXPATHSIZE]; //fifo olusturmak icin tutulan path
    char StringUnlink[MAXPATHSIZE]; //unlink etmek icin bilgilerin string olarak tutuldugu gecici array
    char path[MAXPATHSIZE]; //icinde bulunulan path
    char tempPath[MAXPATHSIZE]; //icinde bulunulan pathi olusturmak icin kullanilan temp path
    char **PathUnlink = NULL; //unlink etmek icin tum bilgilerin tutuldugu arra
    int iSizeUnlink = 0; //unlink etmek icin tum bilgilerin tutuldugu arrayin size'i
    
    int **arrPipe; //pipe array
    int iSizePipe = 0;
    int *arrFifo; //fifo array
    int iSizeFifo = 0;
    
    int fd; //checkStringFile'a gonderilecek file descriptor
    
    int iCount = 0; //bir file daki string sayisini tutar
    int iWords = 0; //toplam word sayisi
    int i = 0;
    int size = 0;
    
    //fifo ve pipe arrayleri icin yer ayirdim
    arrFifo = malloc(0);
    arrPipe = malloc(0);

    getcwd(tempPath, MAXPATHSIZE);
    sprintf(path, "%s/%s", tempPath, dir_name);
    
    chdir(path); 
    //path bufa atilip kontrol ediliyor
    if (stat(path, &buf) == -1)
        return -1;
    // directory testi yapiliyor
    if (S_ISDIR(buf.st_mode)) {
        if ((dir = opendir(path)) == NULL) { //directory acilmiyorsa hata verir
            printf("%s can't open.\n", path);
            exit(EXIT_FAILURE);
        }
        else {
            //butun directoryleri dolasip file larda stringi aradim
            while ((mdirent = readdir(dir)) != NULL) {
                if (strcmp(mdirent->d_name, ".") != 0 && strcmp(mdirent->d_name, "..") != 0 && mdirent->d_name[strlen(mdirent->d_name) - 1] != '~') {
                    sprintf(nameDirFile, "%s/%s", path, mdirent->d_name);
                    if (stat(nameDirFile, &buf) == -1)
                        return -1;
                    //directoryse fifo olusturup fork yaptim, directory'ler fifo ile haberlesecegi icin
                    if (S_ISDIR(buf.st_mode)) {
                        iSizeFifo++;
                        arrFifo = realloc(arrFifo, iSizeFifo * sizeof (int));

                        strcpy(PathFifo, nameDirFile);
                        sprintf(PathFifo, "%d", iSizeFifo);
                        mkfifo(PathFifo, (S_IRUSR | S_IWUSR)); //fifoyu olusturdum
                      
                        sprintf(StringUnlink, "%s/%d" ,  getcwd(StringUnlink, MAXPATHSIZE), iSizeFifo);    

                        //unlink etmek icin path bilgisini tuttum
                       	PathUnlink = (char **) realloc(PathUnlink, (iSizeUnlink + 1) * sizeof(*PathUnlink));
                        PathUnlink[iSizeUnlink] = (char *)malloc(2 * sizeof(StringUnlink));
                        iSizeUnlink++;
                        sprintf(PathUnlink[size], "%s/%d" ,  getcwd(PathUnlink[size], MAXPATHSIZE), iSizeFifo);                  
                        size++;

                        pid = fork();
                        if(pid == -1){
                             perror("Failed to fork\n");
                             closedir(dir);
                             return 0;
                        }
                        if (pid == 0) { //child
                            while (((fd = open(PathFifo, O_WRONLY)) == -1) && (errno == EINTR));
                            if (fd == -1) {
                                fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
                                        (long) getpid(), PathFifo, strerror(errno));
                                return 1;
                            }
                            //child directory icindeki ya da yanindaki bir path(directory OR file) ile tekrar DirWalk'i cagirir
                            iWords += DirWalk(mdirent -> d_name, fd);
                            closedir(dir);
                            close(fd);
				            //fork yapildiginda olusan process parant'in sahip oldugu her seye sahip olur
		                    free(sSearchStr); //isi biten processte kopyasi olusan bu string bosaltimak zorunda
		                    fclose(fPtr);
		                    //system("rm tempFile.txt");
                            //remove("tempFile.txt");
                            exit(iWords);
                        } else { //parent
                            while (((arrFifo[iSizeFifo - 1] = open(PathFifo, O_RDONLY)) == -1) && (errno == EINTR));
                            if (arrFifo[iSizeFifo - 1] == -1) {
                                fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
                                        (long) getpid(), PathFifo, strerror(errno));
                                return 1;
                            }
                        }
                    }
                    
                    //eger file ise pipe olusturup fork yaptim
                    else if (S_ISREG(buf.st_mode)) {
                        iSizePipe++;
                        arrPipe = realloc(arrPipe, iSizePipe * sizeof (int*));
                        arrPipe[iSizePipe - 1] = malloc(sizeof (int) * 2);
                        pipe(arrPipe[iSizePipe - 1]);
              
                        sprintf(fname, "%s", mdirent->d_name);
              
                        pid = fork();
                        if(pid == -1){
                             perror("Failed to fork\n");
                             closedir(dir);
                             return 0;
                        }
                        if (pid == 0) { //child                        
                            iCount = searchStringInFile(fname, arrPipe[iSizePipe - 1][1]);
                            iWords += iCount;
                            fprintf(fPtr, "%d\n", iCount);
                            #ifndef DEBUG
                                printf("iCount: %d\n", iCount);
                            #endif
                            closedir(dir);
				            //fork yapildiginda olusan process parent'in sahip oldugu her seye sahip olur
		                    free(sSearchStr); //isi biten processte kopyasi olusan bu string bosaltimak zorunda
		                    fclose(fPtr);
		                    //system("rm tempFile.txt");
                            //remove("tempFile.txt");
                            exit(iWords);
                        }
                    }
                }
            }
        }

        while (r_wait(NULL) > 0); //parent child lari bekliyor

        //pipe'lar degerleri fifoya veriliyor
        for (i = 0; i < iSizePipe; ++i) {
            close(arrPipe[i][1]);
            copyfile(arrPipe[i][0], fifo);
            close(arrPipe[i][0]);
        }

        //olusan fifolar temizleniyor
 		for (i = 0; i < size; ++i) {
            copyfile(arrFifo[i], fifo);
            close(arrFifo[i]);
            unlink(PathUnlink[i]);
        }
        //unlink bilgilerinin tutuldugu array bosaltiliyor
        for (i = 0; i < size; i++)
            free(PathUnlink[i]);
        free(PathUnlink);

        //pipelar temizleniyor
        for (i = 0; i < iSizePipe; ++i)
            free(arrPipe[i]);
        free(arrPipe);
        free(arrFifo);  
    }
    closedir(dir);
    return iWords;
}


/**
    Yapilan islemler main de kafa karistirmasin diye hepsini bu fonksiyonda 
    topladim.

    sFileName : String, input, icinde arama yapilacak dosyanin adini tutuyor
*/
int searchStringInFile(char* sFileName, int pipeFd){
    char **sStr=NULL;
    int i=0, j=0;
    int iCount = 0;
    char *s;
    //Burada adi verilen dosyanin acilip acilmadigina baktim
    //Acilamadiysa programi sonlandirdim.
    fPtrInFile = fopen (sFileName, "r");
    if (fPtrInFile == NULL) {
        perror (sFileName);
        exit(1);
    }

    if(isEmpty(fPtrInFile) == 1){
        rewind(fPtrInFile);
        //Dosyanin satir sayisini ve en uzun satirin 
        //column sayisini bulan fonksiyonu cagirdim.
        findLengthLineAndNumOFline();
        //Dosyayi tekrar kapatip acmak yerine dosyanin nerede oldugunu 
        //gosteren pointeri dosyanin basina aldim
        rewind(fPtrInFile);

        //Dosyayi string arrayine okudum ve bu string'i return ettim
        sStr=readToFile();

        #ifndef DEBUG //Dosyayi dogru okuyup okumadigimin kontrolü
            printf("File>>>>>>>\n");
            for(i=0; i<iNumOfLine; i++)
                printf("%s\n", sStr[i]);
        #endif
        
        s = (char*)calloc(MAXSIZE, sizeof(char));
        
        //String arrayi icinde stringi aradim ve sayisini iCount'a yazdim
        iCount=searchString(sFileName, sStr, pipeFd);
        //iWordCount += iCount;
        sprintf(s, "\n****************************************************\n");
        write(pipeFd, s, strlen(s));
        sprintf(s,"%s found %d in total in %s file\n", sSearchStr, iCount, sFileName); 
        write(pipeFd, s, strlen(s));
        sprintf(s, "****************************************************\n\n");
        write(pipeFd, s, strlen(s));
        
        free(s);
        
        //Strin icin ayirdigim yeri bosalttim
        for(i=0; i<iNumOfLine; i++)
            free(sStr[i]);
        free(sStr);
    }
    fclose(fPtrInFile);
    return iCount;
}


/**
    String arama isleminin ve her yeni bir string bulundugunda bulunan 
    kelime sayisinin arttirildigi fonksiyon

    sFile     :String arrayi, input, ıcınde arama yapiacak string arrayi
    sFileName :String'in aranacagi dosya adi
    output degeri ise integer ve bulunan string sayisini return eder
*/
int searchString(char* sFileName, char **sFile, int pipeFd){
    int i=0, j=0;
    int iRow=0, iCol=0;
    char *word=NULL;
    int iCount=0;
    char *s;    
    //string arrayinin her satirini sira ile str stringine kopyalayip inceleyecegim
    word=(char *)calloc(100, sizeof(char));
    s = (char *)calloc(MAXSIZE, sizeof(char));
    for(i=0; i<iNumOfLine; i++){ //Satir sayisina gore donen dongu
        for(j=0; j<iLengthLine; j++){ //Sutun sayisina gore donen dongu
                //printf("i:%d\tj:%d\n", i, j);
            if((copyStr(sFile, word, i, j, &iRow, &iCol)) == 1){ //str stringine kopyalama yaptim
                //kopyalama ile sSearchStr esit mi diye baktim
                if(strncmp(word, sSearchStr, (int)strlen(sSearchStr)) == 0){
                    #ifndef DEBUG
                        printf("%s: [%d, %d] %s first character is found.\n", sFileName, iRow, iCol, sSearchStr);
                    #endif
                	//Bulunan kelimenin satir ve sutun sayisi LogFile'a yazdim
                	sprintf(s, "%s: [%d, %d] %s first character is found.\n", sFileName, iRow, iCol, sSearchStr);
                	write(pipeFd, s, strlen(s));
                    iCount++; //String sayisini bir arttirdim kelime buldugum icin
                }
            }
        }
    }
    free(s);
    free(word);
    return iCount; //Bulunan string sayisini return ettim
}

/**
   Aranmasi gereken stringin karakter sayisi kadar karakteri word stringine kopyalar.
   Kopyalama yaparken kopyalanan karakterin space(' '), enter('\n') ve tab('\t') 
   olmamasina dikkat ederek kopyalar.
   
   sFile    :Dosyadaki karakterlerin tutuldugu iki boyutlu karakter arrayi
   word     :Kopyalanan karakterlerin tutulacagi 1 karakter arrayi
   iStartRow:Aramanin baslayacagi satir indexi
   iStartCol:Aramanin baslayacagi sutun indexi
   iRow     :Bulunan kelimenin ilk karakterinin bulundugu satir numarasi
   iCol     :Bulunan kelimenin ilk karakterinin bulundugu sutun numarasi
*/
int copyStr(char **sFile, char* word, int iStartRow, int iStartCol, int *iRow, int *iCol){

    int k=0, i=0, j=0, jStart = 0;
    //printf("iStartRowIndex:%d\tiStartColIndex:%d\n", iStartRow, iStartCol);
    
    if(sFile[iStartRow][iStartCol] == '\n' || sFile[iStartRow][iStartCol] == '\t' || sFile[iStartRow][iStartCol] == ' '){
        return 0;
    }  
    else{
        *iRow = iStartRow+1;
        *iCol = iStartCol+1;
	    #ifndef DEBUG
    	    printf("iRow:%d\tiCol:%d\n", *iRow, *iCol);
		    printf("iStartRow:%d\tiStartCol:%d\n", iStartRow, iStartCol);
	    #endif
        k=0;
        jStart = *iCol-1;
        for(i=*iRow-1; i<iNumOfLine && k < iSizeOfSearchStr; i++){
            for(j=jStart; j<iLengthLine && k < iSizeOfSearchStr; j++){
		        if(sFile[i][j] != '\n' && sFile[i][j] != '\t' && sFile[i][j] != ' ' && k < iSizeOfSearchStr){
                    word[k] = sFile[i][j];
                    k++;
                }                
		        if(sFile[i][j] == '\n' && k < iSizeOfSearchStr){
                    j=iLengthLine;
                }
            }
	        jStart=0; //jnin bir alt satirda baslangic konumu 0 olarak ayarlandi
        }    
        if(k != iSizeOfSearchStr)
            return 0;
        else
            return 1;
    }
    return -1;
}



/**
    dosyanin bos olup olmadigina bakar
    1->bos degil
    0->bos
*/
int isEmpty(FILE *file){
    long savedOffset = ftell(file);
    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0){
        return 0;
    }

    fseek(file, savedOffset, SEEK_SET);
    return 1;
}


/**
    Dosyadaki satir sayisini ve en uzun sutundaki karakter sayisini bulur.
    Burdan gelen sonuclara gore dynamic allocation yapilir.
*/
void findLengthLineAndNumOFline(){
	int iLenghtLine=0;
	int iMaxSize=0;
	char ch=' ';

		while(!feof(fPtrInFile)){
			fscanf(fPtrInFile, "%c", &ch);
			iMaxSize++;
				if(ch == '\n'){
					iNumOfLine=iNumOfLine+1;
					if(iMaxSize >=(iLengthLine))
						iLengthLine=iMaxSize;
					iMaxSize=0;
				}
		}
		iNumOfLine-=1; //bir azalttim cunku dongu bir defa fazla donuyor ve iNumOfLine
                        //bir fazla bulunuyor.
        iLengthLine+=1;
        #ifndef DEBUG
            printf("iLengthLine:%d\tiNumOfLine:%d\n", iLengthLine, iNumOfLine);
        #endif
}

/**
    Dosya okunur ve iki boyutlu bir karakter arrayyine atilir.
    Karakter arrayi return edilir.    

    output: char**, okunan dosyayi iki boyutlu stringe aktardim ve 
            bu string arrayini return ettim.
*/
char** readToFile(){
    char **sFile=NULL;
    int i=0;

    //Ikı boyutlu string(string array'i) icin yer ayirdim
    sFile=(char **)calloc(iNumOfLine*iLengthLine, sizeof(char*));
    if( sFile == NULL ){ //Yer yoksa hata verdim
        #ifndef DEBUG
            printf("INSUFFICIENT MEMORY!!!\n");
        #endif
        exit(1);
    }
    //Ikı boyutlu oldugu ıcın her satir icinde yer ayirdim
    for(i=0; i<iNumOfLine; i++){
        sFile[i]=(char *)calloc(iLengthLine, sizeof(char));
        if( sFile[i] == NULL ){ //Yer yoksa hata verdim
            #ifndef DEBUG
                printf("INSUFFICIENT MEMORY!!!\n");
            #endif
            exit(1);
        }
    }

    i=0;
    do{ //Dosyayi okuyup string arrayine yazdim
    
        fgets(sFile[i], iLengthLine, fPtrInFile);
        #ifndef DEBUG
            printf("*-%s-*\n", sFile[i]);
        #endif
        i++;
    }while(!feof(fPtrInFile));

    return sFile;
}


