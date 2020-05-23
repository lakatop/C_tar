#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

//returns position + 1 in argv if s is in argv, 0 if s is not in argv
int contains(const char* s, int argc, char*argv[]){
    for(int i = 0; i < argc; i++)
        if(strcmp(s,argv[i]) == 0)
            return i + 1;
    return 0;
}

//returns argument that is right after -f option
char* getFileName(int argc, char* argv[]){
    int position = contains("-f", argc, argv);
    return argv[position];
}

//exit with return value 2 if there is some unrecognized option
void checkForWrongOption(int argc, char* argv[], char* supportedOptions[], int suppOptSize)
{
    for(int i = 1; i < argc; i++){
        if(*argv[i] == '-' && !contains(argv[i],suppOptSize,supportedOptions))
        {
            err(2,"mytar: Unknown option: %s",argv[i]);
        }
    }
}

void checkForZeroBlock(size_t* zeroBlocks, char* buffer, int offset){
    bool zero = true;
    for(int i = 0; i < 512; i++)    //checking if it is zero block
        if(buffer[offset + i] != '\0')
            zero = false;
    if(zero)
        (*zeroBlocks)++;
    else{
        err(2,"Unexpected file format");    //no zero block, some corrupted data
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

void handle_t_option(int argc, char* fileName, char *fileList[]){
    if(argc > 4)    //if true, fileList is not empty
    {
        int pos;
        if((pos = contains(fileName,argc-4,fileList)) > 0){
            fileList[pos-1] = "";
            printf("%s\n",fileName);
        }
    }
    else
    {
        printf("%s\n",fileName);
    }
}

void handleNotPresentFiles(char* fileList[], int count){
    bool quit = false;
    if(count > 0){
        for(int i=0; i< count; i++){
            if(strcmp(fileList[i],"") != 0)
            {
                fprintf(stderr,"mytar: %s: Not found in archive\n",fileList[i]);
                quit = true;
            }
        }
    }
    if(quit){
        fprintf(stderr,"mytar: Exiting with failure status due to previous errors\n");
        exit(2);
    }
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    bool t_Option;   
    bool f_Option;
    char* fileName;
    char *fileList[(argc >= 4) ? (argc - 4) : 0];   //at least 4 arguments for programName,-t,-f,fileName, and the rest are files or nothing
    char* supportedOptions[] = {"-t", 
                                "-f",};
    FILE* tarFile;

    if(argc == 1)   //called without arguments
        err(2,"Need at least 1 option");    
    else
    {
        t_Option = contains("-t", argc, argv);  //check for -t option
        f_Option = contains("-f", argc, argv);  //check for -f option

        if(f_Option)    //if -f option is in argv, get fileName
            fileName = getFileName(argc, argv);
        else{
            fileName = getenv("TAPE");
            if(fileName == NULL)
                fileName = "/dev/tu00";
        }
    
        if(t_Option)    //if -t option is in argv, get list of files
            for(int i = 4; i < argc; i++)
                fileList[i-4] = argv[i];
        
        //check for wrong option in argv
        int suppOptSize = sizeof(supportedOptions)/sizeof(*supportedOptions);
        checkForWrongOption(argc, argv, supportedOptions, suppOptSize);
    }

    tarFile = fopen(fileName,"r");
    if(tarFile == NULL)
        err(2,"tar: %s: Cannot open: No such file or directory\ntar: Error is not recoverable: exiting now",fileName);
    if(tarFile != NULL)
    {
        //get size of tar archive
        fseek(tarFile,0L,SEEK_END);
        size_t size = ftell(tarFile);
        fseek(tarFile,0L,SEEK_SET);

        //create buffer and copy tar archive data in it
        char buffer[size];
        fread(buffer,1,size,tarFile);

        bool end = buffer[0] == '\0';
        size_t zeroBlocks = 0;
        size_t offset = 0;
        while (!end){
            if(zeroBlocks == 2)
            {
                handleNotPresentFiles(fileList,argc-4);
                exit(0);
            }
            //missing 1 zero block
            if((offset == size) && (zeroBlocks == 1)){
                fprintf(stderr,"mytar: A lone zero block at %zu",offset/512);
                exit(0);
            }
            //missing both zero blocks
            else if(offset == size)
                exit(0);

            char fileName[100];
            char fileSize[12];
            char fileType;

            strncpy(fileName,buffer+offset,100);
            strncpy(fileSize,buffer+offset+124,12);
            fileType = buffer[offset+156];

            if(fileName[0] == '\0')
            {
                checkForZeroBlock(&zeroBlocks,buffer,offset);
                offset += 512;
            }
            else{
                //get size of file in dec
                int filesize = OctToDec(fileSize);
                //get size of file with padding to 512B
                filesize = (filesize % 512 == 0) ? filesize : (filesize + (512 - (filesize % 512)));

                //check for truncated tar archive
                if(offset + 512 + filesize > size)
                {
                    printf("%s\n",fileName);
                    fprintf(stderr,"mytar: Unexpected EOF in archive\nmytar: Error is not recoverable: exiting now");
                    exit(2);
                }
                //check if file is regular
                if(fileType != '0')
                {
                    fprintf(stderr,"mytar: Unsupported header type: %d",fileType);
                    exit(2);
                }

                if(t_Option){
                    handle_t_option(argc,fileName,fileList);
                }
                offset += 512 + filesize;
            }
        }
    }
}