#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "ipp.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <stdint.h>
#include <QString>
#include <QStringList>
#include <QDebug>

//#include <windows.h>
//#include <process.h>

#define NON_INT_DEC_ERR -1

class Processor
{
public:
    Processor(int in_fs, int in_chnBW, int in_numTaps, int in_chnlIdx);
    ~Processor();

    int initRawMemory(int len);

    int LoadRawFiles_int16(QStringList filepaths);

    void makeFilterTaps();

    int ChanneliseStart(int NUM_THREADS=4);

    void ChanneliseThread(int t_ID, Ipp32fc *y, int L,
                         int N, int Dec, int nprimePts,
                         Ipp32f *f_tap,
                         Ipp8u *pDFTBuffer, IppsDFTSpec_C_32fc *pDFTSpec,
                         Ipp32fc *out, int NUM_THREADS);


private:
    // parameters
    int fs;
    int chnBW;
    int L;
    int chnlIdx;

    int Dec;
    int N;
    int siglen;
    int nprimePts;

    // array data
    Ipp16sc *rawdata;
    Ipp32fc *y;

    Ipp32f *f_tap;
    Ipp32fc *out;

};

#endif // PROCESSOR_H
