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

void checkForWrongOption(int argc, char* argv[], char* supportedOptions[], int suppOptSize)
{
    for(int i = 1; i < argc; i++){
        if(*argv[i] == '-' && !contains(argv[i],suppOptSize,supportedOptions))
        {
            err(2,"mytar: Unknown option: %s",argv[i]);
            exit(2);
        }
    }
}

int main(int argc, char *argv[])
{
    bool t_Option;   
    bool f_Option;
    char* fileName;
    char *fileList[(argc >= 4) ? (argc - 4) : 0];   //at least 4 arguments for programName,-t,-f,fileName, and the rest are files or nothing
    char* supportedOptions[] = {"-t", 
                                "-f",};
    FILE* tarFile;

    if(argc == 1)   //called without arguments
        exit(2);
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
    

    
    
    char buf[2048];
    FILE *file3;
    size_t nread;
    size_t sz = sizeof buf;
    char s[] = "0123456789";
    file3 = fopen("arch.tar", "r");
    fread(buf,1536,1,file3);
    char sk[1200];
    for(int i = 1000; i < 2048; i++)
        sk[i - 1000] = buf[i];
    fclose(file3);

    
}