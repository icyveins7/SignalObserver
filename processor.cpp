#include "processor.h"


Processor::Processor(int in_fs, int in_chnBW, int in_numTaps, int in_chnlIdx)
{
    // assign vars
    fs = in_fs;
    chnBW = in_chnBW;
    L = in_numTaps;
    chnlIdx = in_chnlIdx;

    // compute dependent vars
    Dec = fs / chnBW;
    if (fs % chnBW != 0){
        throw NON_INT_DEC_ERR;
    }
    siglen = 0;
    N = Dec * 2; // hard coded overlaps for now

    // initialise options
    WOLA_NUMTHREADS = 4;

    // initialise pointers to null
    rawdata = nullptr;
    y = nullptr;
    f_tap = nullptr;
    out = nullptr;

    // print?
    printf("Processor initialized successfully.\n");
}

Processor::~Processor(){
    printf("Processor starting to clear..\n");

    // clear all used memory
    ippsFree(rawdata);
    ippsFree(y);
    ippsFree(f_tap);
    ippsFree(out);

    // print?
    printf("Processor destroyed successfully.\n");
}

int Processor::LoadRawFiles_int16(QStringList filepaths){
    // set some vars
    siglen = filepaths.size() * fs;
    nprimePts = siglen/Dec;

    // alloc mem
    printf("Allocating %i length for rawfiles\n", siglen);
    rawdata = ippsMalloc_16sc_L(siglen);

    if (rawdata == nullptr){
        printf("failed to allocate raw file memory\n");
        return 2;
    }

    y = ippsMalloc_32fc_L(siglen);

    if (y == nullptr){
        printf("failed to alloc raw file 32fc memory\n");
        return 3;
    }

    // start opening files
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
    printf("%hi, %hi // %hi, %hi\n", rawdata[0].re, rawdata[0].im, rawdata[fs-1].re, rawdata[fs-1].im);

    // convert to 32fc immediately and toss the rawdata format
    ippsConvert_16s32f((Ipp16s*)rawdata, (Ipp32f*)y, 2 * siglen);
    ippsFree(rawdata);
    printf("%g, %g // %g, %g\n", y[0].re, y[0].im, y[fs-1].re, y[fs-1].im);

    return 0;
}

void Processor::makeFilterTaps(){
    qDebug() << "init making of taps of length " << L;
    if (f_tap == nullptr){
        qDebug() << "initialise taps memory";
        f_tap = ippsMalloc_32f_L(L);
    }

    // temporary 64f pTaps
    Ipp64f *pTaps = (Ipp64f*)ippsMalloc_64f_L(L);

    // start making filter taps
    int gen_bufSize;
    double Wn = (double)chnBW / (double)fs;
    ippsFIRGenGetBufferSize(L, &gen_bufSize);
    Ipp8u *gen_pBuffer = ippsMalloc_8u_L(gen_bufSize);
    ippsFIRGenLowpass_64f(Wn/2, pTaps, L, ippWinHamming, ippTrue, gen_pBuffer); // generate the filter coefficients

    // copy them over to 32f storage
    ippsConvert_64f32f(pTaps, f_tap, L);

    qDebug() << "ftaps = " << f_tap[0] << f_tap[1] << f_tap[L-2] << f_tap[L-1];

    // freeing temporary things
    ippsFree(pTaps);
    ippsFree(gen_pBuffer);

    qDebug() << "Init'ed f_tap";
}

int Processor::ChanneliseStart(){
    qDebug()<<"Entered channeliser";

    int NUM_THREADS = WOLA_NUMTHREADS;

    // make the filter taps
    makeFilterTaps();

    // make the out vector
    out = ippsMalloc_32fc_L(nprimePts * N);

    // the actual wola
    int t;
    std::vector<std::thread> threadPool(NUM_THREADS);

    // ===== IPP DFT Allocations =====

    int sizeSpec = 0, sizeInit = 0, sizeBuf = 0;
    ippsDFTGetSize_C_32fc(N, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuf); // this just fills the 3 integers
    /* memory allocation */
    IppsDFTSpec_C_32fc **pDFTSpec = (IppsDFTSpec_C_32fc**)ippMalloc(sizeof(IppsDFTSpec_C_32fc*)*NUM_THREADS);
    Ipp8u **pDFTBuffer = (Ipp8u**)ippMalloc(sizeof(Ipp8u*)*NUM_THREADS);
    Ipp8u **pDFTMemInit = (Ipp8u**)ippMalloc(sizeof(Ipp8u*)*NUM_THREADS);
    for (t = 0; t<NUM_THREADS; t++){ // make one for each thread
        pDFTSpec[t] = (IppsDFTSpec_C_32fc*)ippMalloc(sizeSpec); // this is analogue of the fftw plan
        pDFTBuffer[t] = (Ipp8u*)ippMalloc(sizeBuf);
        pDFTMemInit[t] = (Ipp8u*)ippMalloc(sizeInit);
        ippsDFTInit_C_32fc(N, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone,  pDFTSpec[t], pDFTMemInit[t]); // kinda like making the fftw plan?
    }

    // use c++ threads instead?
    qDebug()<<"Starting wola threads\n";

    qDebug() << nprimePts;
    qDebug() << L;
    qDebug() << N;
    qDebug() << Dec;
    for (t = 0; t < NUM_THREADS; t++){

        threadPool[t] = std::thread(&Processor::ChanneliseThread, this,
                                    t, y, L,
                                    N, Dec, nprimePts,
                                    f_tap,
                                    pDFTBuffer[t], pDFTSpec[t],
                                    out, NUM_THREADS);
    }

    for (t = 0; t< NUM_THREADS; t++){
        threadPool[t].join(); // wait for them to finish
    }

    qDebug()<<"Wola threads finished\n";

//    // check some data?
//    for (int c = 0; c < 10; c++){
//        printf("%f, %f\n", out[c*N].re, out[c*N].im);
//    }

//    for (int c = 0; c < 10; c++){
//        printf("%f, %f\n", out[c*N + chnlIdx].re, out[c*N + chnlIdx].im);
//    }

    // === FINAL CLEANUP ===
    for (t=0; t<NUM_THREADS; t++){
        ippFree(pDFTSpec[t]);
        ippFree(pDFTBuffer[t]);
        ippFree(pDFTMemInit[t]);
    }
    ippFree(pDFTSpec);
    ippFree(pDFTBuffer);
    ippFree(pDFTMemInit);

    emit(ChanneliserFinished());

    return 0;
}

void Processor::ChanneliseThread(int t_ID, Ipp32fc *y, int L,
                                 int N, int Dec, int nprimePts,
                                 Ipp32f *f_tap,
                                 Ipp8u *pDFTBuffer, IppsDFTSpec_C_32fc *pDFTSpec,
                                 Ipp32fc *out, int NUM_THREADS){

    int nprime, n, a, b; // declare to simulate threads later
    int k;

    // allocate for FFTs
    Ipp32fc *dft_in = (Ipp32fc*)ippsMalloc_32fc_L(N);
    Ipp32fc *dft_out = (Ipp32fc*)ippsMalloc_32fc_L(N);

    // pick point based on thread number
    for (nprime = t_ID; nprime<nprimePts; nprime=nprime+NUM_THREADS){
        n = nprime*Dec;

        ippsZero_32fc(dft_in, N);

        for (a = 0; a<N; a++){
            for (b = 0; b<L/N; b++){
                if (n - (b*N+a) >= 0){
                    dft_in[a].re = dft_in[a].re + y[n-(b*N+a)].re * f_tap[b*N+a];
                    dft_in[a].im = dft_in[a].im + y[n-(b*N+a)].im * f_tap[b*N+a];
                }
            }
        }

        ippsDFTInv_CToC_32fc(dft_in, &out[nprime*N], pDFTSpec, pDFTBuffer);

        if (Dec*2 == N && nprime % 2 != 0){ // only if using overlapping channels, do some phase corrections when nprime is odd
            for (k=1; k<N; k=k+2){ //  all even k are definitely even in the product anyway
                out[nprime*N + k].re = -out[nprime*N + k].re;
                out[nprime*N + k].im = -out[nprime*N + k].im;
            }
        }
    }

    ippsFree(dft_in);
    ippsFree(dft_out);
}


void Processor::getOptions(QVector<QString> &optlabels, QVector<int> &opts){
    optlabels.push_back("Channeliser Threads");
    opts.push_back(WOLA_NUMTHREADS);
}

void Processor::setOptions(QVector<int> opts){
    qDebug() << "setting new options";
    WOLA_NUMTHREADS = opts.at(0);
}

