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

    int LoadCutout_32fc(QString filepath);

    void makeFilterTaps();

    int ChanneliseStart();

    void ChanneliseThread(int t_ID, Ipp32fc *y, int L,
                         int N, int Dec, int nprimePts,
                         Ipp32f *f_tap,
                         Ipp8u *pDFTBuffer, IppsDFTSpec_C_32fc *pDFTSpec,
                         Ipp32fc *out, int NUM_THREADS);

    int XcorrStart(QString cutoutfilepath, bool alreadyConj);

    void XcorrThread(int t_ID, Ipp32fc *cutout, Ipp32fc *y,
                     Ipp64f *y_absSq, Ipp64f cutout_pwr, int *shifts,
                     int shiftPts, int fftlen,
                     Ipp8u *pBuffer, IppsDFTSpec_C_32fc *pSpec,
                     Ipp64f *productpeaks, int *freqlist_inds, int NUM_THREADS);

    // parameter extraction
    int getNprimePts(){return nprimePts;}

    // data / plot funcs
    void makeChannelTimeFreqData();
    void makeXcorrData();


    // options extraction
    void getOptions(QVector<QString> &optlabels, QVector<int> &opts);


    // array data
    Ipp16sc *rawdata;
    Ipp32fc *y; // converted from raw

    Ipp32f *f_tap;
    Ipp32fc *out; // the channelised data

    Ipp32fc *cutout;
    int cutoutlen;
    Ipp64f cutout_pwr;
    int *shifts;
    int shiftPts;
    Ipp64f *productpeaks;
    int *freqlist_inds;
//    Ipp64f *freqlist;

    // plot data
    Ipp32fc *chnl;
    Ipp64f *chnl_t;
    Ipp64f *chnl_absSq;
    Ipp64f *chnl_f;
    Ipp64f *chnl_spectrum;

public slots:
    // options setting
    void setOptions(QVector<int> opts);

signals:
    void ChanneliserFinished();
    void ChannelTimeFreqDataFinished();
    void XcorrFinished();


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
    int XCORR_NUMTHREADS;

};

#endif // PROCESSOR_H
