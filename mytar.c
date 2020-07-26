 
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#define BUFFERSIZE 512

//returns position + 1 in argv if s is in argv, 0 if s is not in argv
int contains(const char* s, int argc, char*argv[]){
    for(int i = 0; i < argc; i++)
        if(strcmp(s, argv[i]) == 0){
            return i + 1;
        }
    return 0;
}

//returns argument that is right after -f option
char* getFileName(int argc, char* argv[]){
    int position = contains("-f", argc, argv);
    return argv[position];
}

//exit with return value 2 if there is some unrecognized option
void checkForWrongOption(int argc, char* argv[], char* supportedOptions[], 
int suppOptSize){
    for(int i = 1; i < argc; i++){
        if(*argv[i] == '-' && !contains(argv[i], suppOptSize, 
        supportedOptions)){
            err(2,"mytar: Unknown option: %s",argv[i]);
        }
    }
}

void checkForZeroBlock(size_t* zeroBlocks, char* buffer){
    bool zero = true;
    for(int i = 0; i < BUFFERSIZE; i++){    //checking if it is zero block
        if(buffer[i] != '\0')
            zero = false;
    }
    if(zero){
        (*zeroBlocks)++;
    }
    else{
        err(2,"Unexpected file format");   //no zero block, some corrupted data
    }
}

int OctToDec(char num[]){
    int decNum = num[10] - '0';
    int multiplier = 8;
    for(int i = 9; i > -1; i--){
        decNum += (num[i] - '0') * multiplier;
        multiplier = multiplier * 8;
    }

    return decNum;
}   

void handle_t_option(char* fileName, char *fileList[], int filesCount){
    if(filesCount){    //if true, fileList is not empty
        int pos;
        if((pos = contains(fileName, filesCount, fileList)) > 0){
            fileList[pos-1] = "";
            printf("%s\n", fileName);
        }
    }
    else{
        printf("%s\n", fileName);
    }
}

void handleNotPresentFiles(char* fileList[], int count){
    bool quit = false;
    if(count > 0){
        for(int i=0; i< count; i++){
            if(strcmp(fileList[i], "") != 0){
                fprintf(stderr, "mytar: %s: Not found in archive\n", 
                fileList[i]);
                quit = true;
            }
        }
    }
    if(quit){
        fprintf(stderr, "mytar: Exiting with failure status"
        " due to previous errors\n");
        exit(2);
    }
}

void createFile(FILE* tarFile, char *fileName, int fileSize, char fileType){
    if(fileType == '0' || fileType == '\0'){
        //create new file with given name
        FILE* newFile = fopen(fileName, "w");
        if(newFile == NULL){
            err(2,"tar: %s: Cannot open: No such file or directory\ntar:"
            " Error is not recoverable: exiting now", fileName);
        }
        char buff[513] = {'\0'};
        //copy and write 512B chunks into newFile
        for(int i=0; i <fileSize/512; i++){
            if(fread(buff, 1, 512, tarFile) != 512){
                exit(2);
            }
            buff[512] = '\0';
            if(fwrite(buff, 1, 512, newFile) != 512){
                exit(2);
            }
        }
        //if there are some leftovers whose size is smaller than 512B
        if(fileSize%512 != 0){
            if(fread(buff, 1, fileSize%512, tarFile) != (size_t)fileSize%512){
                exit(2);
            }
            buff[fileSize%512] = '\0';
            if(fwrite(buff, 1, fileSize%512, newFile) != (size_t)fileSize%512){
                exit(2);
            }
        }

        fclose(newFile);
    }
}

void freeList(char** fileList, int count){
    for(int i = 0; i < count; i++){
        free(fileList[i]);
    }
    fileList = NULL;
}

void processFile(char* fileName, char **fileList, char **fileList2, 
bool t_Option, bool x_Option, bool v_Option, int filesCount){
    FILE* tarFile;

    tarFile = fopen(fileName, "r");
    if(tarFile == NULL){
        freeList(fileList2, filesCount);
        err(2,"tar: %s: Cannot open: No such file or directory\ntar:"
         " Error is not recoverable: exiting now", fileName);
    }

    //get size of tar archive
    if(fseek(tarFile, 0LL, SEEK_END)){
        fclose(tarFile);
        freeList(fileList2, filesCount);
        exit(2);
    }
    long int size = ftell(tarFile);
    if(size < 0){
        fclose(tarFile);
        freeList(fileList2, filesCount);
        exit(2);
    }
    if(fseek(tarFile, 0LL, SEEK_SET)){
        fclose(tarFile);
        freeList(fileList2, filesCount);
        exit(2);
    }

    //create buffer and copy tar archive header in it
    char buffer[BUFFERSIZE];
    int read = fread(buffer, 1, BUFFERSIZE, tarFile);
    if(read != BUFFERSIZE){
        fclose(tarFile);
        freeList(fileList2, filesCount);
        exit(2);
    }

    bool end = buffer[0] == '\0';
    size_t zeroBlocks = 0;
    long int offset = 0;

    while (!end){
        if(zeroBlocks == 2){
            fclose(tarFile);
            freeList(fileList2, filesCount);
            handleNotPresentFiles(fileList, filesCount);
            exit(0);
        }
        //missing 1 zero block
        if((offset == size) && (zeroBlocks == 1)){
            fclose(tarFile);
            freeList(fileList2, filesCount);
            fprintf(stderr, "mytar: A lone zero block at %zu", offset/BUFFERSIZE);
            exit(0);
        }
        //missing both zero blocks
        else if(offset == size){
            fclose(tarFile);
            freeList(fileList2, filesCount);
            exit(0);
        }

        char fileName[101];
        char fileSize[13];
        char magic[7];
        char fileType;

        strncpy(fileName, buffer, sizeof(fileName)-1);
        strncpy(fileSize, buffer+124, sizeof(fileSize)-1);
        strncpy(magic, buffer+257, sizeof(magic)-1);
        magic[6] = '\0';
        fileName[100] = '\0';
        fileSize[12] = '\0';
        fileType = buffer[156];

        if(fileName[0] == '\0'){
            checkForZeroBlock(&zeroBlocks, buffer);
            offset += BUFFERSIZE;
            read = fread(buffer, 1, BUFFERSIZE, tarFile);
            if(read != BUFFERSIZE && zeroBlocks == 1){
                fclose(tarFile);
                freeList(fileList2, filesCount);
                fprintf(stderr, "mytar: A lone zero block at %zu", offset/BUFFERSIZE);
                exit(0);
            }
        }
        else{
            //get size of file in dec
            int filesize = OctToDec(fileSize);
            int realFileSize = filesize;
            //get size of file with padding to BUFFERSIZE = 512B
            filesize = (filesize % BUFFERSIZE == 0) ? filesize :
             (filesize + (BUFFERSIZE - (filesize % BUFFERSIZE)));

            //check if it is .tar file
            if((strcmp(magic, "ustar ")) != 0){
                fclose(tarFile);
                freeList(fileList2, filesCount);
                fprintf(stderr,"mytar: This does not look like a tar"
                " archive\nmytar: Exiting with failure status due to"
                " previous errors");
                exit(2);
            }

            //check for truncated tar archive
            if(offset + BUFFERSIZE + filesize > size){
                if((filesCount == 0 && x_Option) || (v_Option && 
                x_Option && contains(fileName, filesCount, fileList2))){
                //get the real size of truncated file
                int newSize = size - (offset + BUFFERSIZE);
                createFile(tarFile, fileName, newSize, fileType);
                }
                if(v_Option || t_Option){
                    printf("%s\n", fileName);
                }
                fclose(tarFile);
                freeList(fileList2, filesCount);
                fprintf(stderr,"mytar: Unexpected EOF in archive\nmytar:"
                " Error is not recoverable: exiting now");
                exit(2);
            }

            //check if file is regular
            if(fileType != '0' && fileType != '\0'){
                fclose(tarFile);
                freeList(fileList2, filesCount);
                fprintf(stderr,"mytar: Unsupported header type: %d",fileType);
                exit(2);
            }

            if((t_Option && !x_Option) || (v_Option && x_Option)){
                handle_t_option(fileName, fileList, filesCount);
            }

            if(!x_Option){
                offset += BUFFERSIZE + filesize;
                if(fseek(tarFile, filesize, SEEK_CUR)){
                    fclose(tarFile);
                    freeList(fileList2, filesCount);
                    exit(2);
                }
                fread(buffer,1,BUFFERSIZE,tarFile);
            }
            else{
                if(filesCount == 0 || 
                (v_Option && contains(fileName, filesCount, fileList2))){
                createFile(tarFile, fileName, realFileSize, fileType);
                }

                offset += BUFFERSIZE + filesize;
                if(fseek(tarFile, offset, SEEK_SET)){
                    fclose(tarFile);
                    freeList(fileList2, filesCount);
                    exit(2);
                }
                fread(buffer, 1, BUFFERSIZE, tarFile);
            }
        }
    }
    freeList(fileList2, filesCount);
    fclose(tarFile);
}

void handleOptions(char **fileName, int argc, char **argv, 
bool *t_Option, bool *f_Option, bool *v_Option, bool *x_Option){
    if(argc == 1) {  //called without arguments
        err(2, "Need at least 1 option");   
    } 
    else{
        *t_Option = contains("-t", argc, argv);  //check for -t option
        *f_Option = contains("-f", argc, argv);  //check for -f option
        *v_Option = contains("-v", argc, argv);  //check for -v option
        *x_Option = contains("-x", argc, argv);  //check for -x option

        if(*f_Option){    //if -f option is in argv, get fileName
            *fileName = getFileName(argc, argv);
        }
        else{
            err(2, "Specify name of the tar file");
        }
    }
}

void handleFileList(char **fileName, int argc, char **argv,char **fileList,
 char **fileList2, int *filesCount, bool getFileList){
        if(getFileList){    //if -t option is in argv, get list of files
            int start = contains(*fileName, argc, argv);
            for(int i = start; i < argc; i++){
                fileList[i-start] = argv[i];
                //first fileList points right to argv, but since i will most 
                //likely "delete" some of the arguments using "contains()",
                //i need to allocate memory for fileList2 and copy each
                // argv argument
                fileList2[i-start] = malloc(strlen(argv[i])+1);
                if(fileList2[i-start] == NULL){
                    exit(2);
                }
                strncpy(fileList2[i - start], argv[i], strlen(argv[i])+1);
                (*filesCount)++;
            }
        }
}

int main(int argc, char *argv[]){
    setbuf(stdout, NULL);
    bool t_Option;   
    bool f_Option;
    bool v_Option;
    bool x_Option;
    char* fileName;
    char *fileList[argc];
    char *fileList2[argc];
    int filesCount = 0;
    char* supportedOptions[] = {"-t", 
                                "-f",
                                "-x",
                                "-v",};

    int suppOptSize = sizeof(supportedOptions)/sizeof(*supportedOptions);

    handleOptions(&fileName, argc, argv, &t_Option,
    &f_Option, &v_Option, &x_Option);

    bool getFileList = (t_Option || v_Option);

    handleFileList(&fileName, argc, argv, fileList,
    fileList2, &filesCount, getFileList);

    checkForWrongOption(argc, argv, supportedOptions, suppOptSize);

    processFile(fileName, fileList, fileList2, t_Option, 
    x_Option, v_Option, filesCount);
}