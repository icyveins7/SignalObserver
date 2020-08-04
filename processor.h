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
#include <QObject>
#include <QVector>

//#include <windows.h>
//#include <process.h>

#define NON_INT_DEC_ERR -1

class Processor : public QObject
{
    Q_OBJECT

public:
    Processor(int in_fs, int in_chnBW, int in_numTaps, int in_chnlIdx);
    ~Processor();

    int initRawMemory(int len);

    int LoadRawFiles_int16(QStringList filepaths);

    void makeFilterTaps();

    int ChanneliseStart();

    void ChanneliseThread(int t_ID, Ipp32fc *y, int L,
                         int N, int Dec, int nprimePts,
                         Ipp32f *f_tap,
                         Ipp8u *pDFTBuffer, IppsDFTSpec_C_32fc *pDFTSpec,
                         Ipp32fc *out, int NUM_THREADS);

    // parameter extraction
    int getNprimePts(){return nprimePts;}

    // data / plot funcs
    void makeChannelTimeFreqData();


    // options extraction
    void getOptions(QVector<QString> &optlabels, QVector<int> &opts);


    // array data
    Ipp16sc *rawdata;
    Ipp32fc *y; // converted from raw

    Ipp32f *f_tap;
    Ipp32fc *out; // the channelised data

    // plot data
    Ipp32fc *chnl;
    Ipp64f *chnl_t;
    Ipp64f *chnl_abs;
    Ipp64f *chnl_f;
    Ipp64f *chnl_spectrum;

public slots:
    // options setting
    void setOptions(QVector<int> opts);

signals:
    void ChanneliserFinished();
    void ChannelTimeFreqDataFinished();


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



    // options
    int WOLA_NUMTHREADS;

};

#endif // PROCESSOR_H
