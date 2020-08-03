#include "processor.h"


Processor::Processor(int in_fs)
{
    fs = in_fs;

    // initialise pointers to null
    rawdata = nullptr;

    // print?
    printf("Processor initialized successfully.\n");
}

Processor::~Processor(){
    printf("Processor starting to clear..\n");

    // clear all used memory
    ippsFree(rawdata);

    // print?
    printf("Processor destroyed successfully.\n");
}

int Processor::LoadRawFiles_int16(QStringList filepaths){
    // initialise raw ipp memory first
    printf("Allocating %i length for rawfiles\n", filepaths.size() * fs);
    rawdata = ippsMalloc_16sc_L(filepaths.size() * fs);

    if (rawdata == nullptr){
        printf("failed to allocate raw file memory\n");
        return 2;
    }

    FILE *fp;

    for (int i = 0; i < filepaths.size(); i++){
        fp = fopen(filepaths.at(i).toUtf8().constData(), "rb");

        if (fp == NULL){
            return 1;
        }
        else{
            size_t totalread = fread(&rawdata[i*fs],sizeof(Ipp16sc),fs,fp);

            fclose(fp);

//            printf("read %i elements, supposed to be %i\n", (int)totalread, fs);
        }
    }

    // debug read files
    printf("%hi, %hi // %hi, %hi", rawdata[0].re, rawdata[0].im, rawdata[fs-1].re, rawdata[fs-1].im);

    return 0;
}


