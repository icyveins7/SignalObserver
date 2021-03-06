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
    XCORR_NUMTHREADS = 4;

    // initialise pointers to null
    rawdata = nullptr;
    y = nullptr;
    f_tap = nullptr;
    out = nullptr;

    cutout = nullptr;
    productpeaks = nullptr;
    freqlist_inds = nullptr;

    chnl = nullptr;
    chnl_t = nullptr;
    chnl_absSq = nullptr;
    chnl_f = nullptr;
    chnl_spectrum = nullptr;

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

    ippsFree(cutout);
    ippsFree(shifts);
    ippsFree(productpeaks);
    ippsFree(freqlist_inds);

    ippsFree(chnl_t);
    ippsFree(chnl_absSq);
    ippsFree(chnl_f);
    ippsFree(chnl_spectrum);

    // print?
    printf("Processor destroyed successfully.\n");
}

void Processor::makeChannelTimeFreqData(){
    qDebug() << "Entered get channel time data";

    // re allocate the arrays
    ippsFree(chnl);
    ippsFree(chnl_t);
    ippsFree(chnl_absSq);
    ippsFree(chnl_f);
    ippsFree(chnl_spectrum);
    chnl = ippsMalloc_32fc_L(nprimePts);
    chnl_t = ippsMalloc_64f_L(nprimePts);
    chnl_absSq = ippsMalloc_64f_L(nprimePts);
    chnl_f = ippsMalloc_64f_L(nprimePts);
    chnl_spectrum = ippsMalloc_64f_L(nprimePts);

//    qDebug() << "in processor, resized to " << t.size() << " and " << x.size();

    // then downsample to get the correct one
    Ipp32f *m = ippsMalloc_32f_L(nprimePts);
    ippsSampleDown_32fc(out, nprimePts * N, chnl, &nprimePts, N, &chnlIdx); // extract the channel
    ippsPowerSpectr_32fc(chnl, m, nprimePts); // get the magnitude
    ippsConvert_32f64f(m, &chnl_absSq[0], nprimePts); // convert to double

    // write values for t
    ippsVectorSlope_64f(&chnl_t[0], nprimePts, 0, 1.0/chnBW);

    // fft to get the spectrum
    // ===== IPP DFT Allocations =====
    int sizeSpec = 0, sizeInit = 0, sizeBuf = 0;
    ippsDFTGetSize_C_32fc(nprimePts, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuf); // this just fills the 3 integers
    /* memory allocation */
    IppsDFTSpec_C_32fc *pDFTSpec = (IppsDFTSpec_C_32fc*)ippMalloc(sizeSpec);
    Ipp8u *pDFTBuffer = (Ipp8u*)ippMalloc(sizeBuf);
    Ipp8u *pDFTMemInit = (Ipp8u*)ippMalloc(sizeInit);
    Ipp32fc *chnl_fft = ippsMalloc_32fc_L(nprimePts);
    Ipp32f *chnl_fft_abslog = ippsMalloc_32f_L(nprimePts);
    ippsDFTInit_C_32fc(nprimePts, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone,  pDFTSpec, pDFTMemInit);
    ippsDFTFwd_CToC_32fc(chnl, chnl_fft, pDFTSpec, pDFTBuffer); // run it
    ippsPowerSpectr_32fc(chnl_fft, m, nprimePts); // can reuse the m vector
    ippsLog10_32f_A24(m, chnl_fft_abslog, nprimePts); // convert to log
    ippsMulC_32f_I(10.0f, chnl_fft_abslog, nprimePts); // x 10

//    ippsConvert_32f64f(m, chnl_spectrum, nprimePts); // convert to 64f in the holding array
    ippsConvert_32f64f(chnl_fft_abslog, chnl_spectrum, nprimePts); // convert to 64f in the holding array

    // write values for f
    ippsVectorSlope_64f(&chnl_f[0], nprimePts, 0, (double)chnBW/(double)nprimePts);
    for (int i = 0; i < nprimePts; i++){
        if (chnl_f[i] >= chnBW/2){
            chnl_f[i] = chnl_f[i] - chnBW;
        }
    }


    // freeing
    ippsFree(m);
    ippsFree(chnl_fft);
    ippsFree(chnl_fft_abslog);

    ippFree(pDFTSpec);
    ippFree(pDFTBuffer);
    ippFree(pDFTMemInit);

    emit(ChannelTimeFreqDataFinished());
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

int Processor::LoadCutout_32fc(QString filepath){
    // start opening files
    FILE *fp;

    fp = fopen(filepath.toUtf8().constData(), "rb");
    if (fp==NULL){
        return 1;
    }
    else{
        // get size first
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);

        // malloc for the array
        ippsFree(cutout);
        cutoutlen = len/sizeof(Ipp32fc);
        cutout = ippsMalloc_32fc_L(cutoutlen);

        fseek(fp, 0, SEEK_SET);
        fread(cutout,sizeof(Ipp32fc),cutoutlen,fp);

        fclose(fp);

        // calculate the cutout_pwr
        ippsNorm_L2_32fc64f(cutout, cutoutlen, &cutout_pwr);
        cutout_pwr = cutout_pwr * cutout_pwr;

        return 0;
    }

}

int Processor::XcorrStart(QString cutoutfilepath, bool alreadyConj){
    qDebug() << "Entered xcorr";
    int NUM_THREADS = XCORR_NUMTHREADS;
    int t;
    std::vector<std::thread> threadPool(NUM_THREADS);

    // load cutout first
    if (LoadCutout_32fc(cutoutfilepath) != 0){
        qDebug() << "error loading cutout";
        return -1;
    };

    if (!alreadyConj){ // then we conjugate it
        ippsConj_32fc_I(cutout, cutoutlen);
    }

//    printf("cutout:\n");
//    for (int i = 0; i < 5; i++){
//        printf("%f, %f \n", cutout[i].re, cutout[i].im);
//    }


    // permanently allocate for output first
    shiftPts = nprimePts - cutoutlen + 1;
    shifts = ippsMalloc_32s_L(shiftPts);
    ippsVectorSlope_32s(shifts, shiftPts, 0, 1.0);

    productpeaks = ippsMalloc_64f_L(shiftPts);
    freqlist_inds = ippsMalloc_32s_L(shiftPts);


    // ===== IPP DFT Allocations =====
    int sizeSpec = 0, sizeInit = 0, sizeBuf = 0;
    ippsDFTGetSize_C_32fc(cutoutlen, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuf); // this just fills the 3 integers
    /* memory allocation */
    IppsDFTSpec_C_32fc **pDFTSpec = (IppsDFTSpec_C_32fc**)ippMalloc(sizeof(IppsDFTSpec_C_32fc*)*NUM_THREADS);
    Ipp8u **pDFTBuffer = (Ipp8u**)ippMalloc(sizeof(Ipp8u*)*NUM_THREADS);
    Ipp8u **pDFTMemInit = (Ipp8u**)ippMalloc(sizeof(Ipp8u*)*NUM_THREADS);
    for (t = 0; t<NUM_THREADS; t++){ // make one for each thread
        pDFTSpec[t] = (IppsDFTSpec_C_32fc*)ippMalloc(sizeSpec); // this is analogue of the fftw plan
        pDFTBuffer[t] = (Ipp8u*)ippMalloc(sizeBuf);
        pDFTMemInit[t] = (Ipp8u*)ippMalloc(sizeInit);
        ippsDFTInit_C_32fc(cutoutlen, IPP_FFT_NODIV_BY_ANY, ippAlgHintNone,  pDFTSpec[t], pDFTMemInit[t]); // kinda like making the fftw plan?
    }


    // use c++ threads instead?
    qDebug()<<"Starting xcorr threads\n";

    for (t = 0; t < NUM_THREADS; t++){

        threadPool[t] = std::thread(&Processor::XcorrThread, this,
                                    t, cutout, chnl,
                                    chnl_absSq, cutout_pwr, shifts,
                                    shiftPts, cutoutlen,
                                    pDFTBuffer[t], pDFTSpec[t],
                                    productpeaks, freqlist_inds, NUM_THREADS);
    }

    for (t = 0; t< NUM_THREADS; t++){
        threadPool[t].join(); // wait for them to finish
    }

    // === FINAL CLEANUP ===
    for (t=0; t<NUM_THREADS; t++){
        ippFree(pDFTSpec[t]);
        ippFree(pDFTBuffer[t]);
        ippFree(pDFTMemInit[t]);
    }
    ippFree(pDFTSpec);
    ippFree(pDFTBuffer);
    ippFree(pDFTMemInit);

    emit(XcorrFinished());

    return 0;
}

void Processor::XcorrThread(int t_ID, Ipp32fc *cutout, Ipp32fc *y,
                            Ipp64f *y_absSq, Ipp64f cutout_pwr, int *shifts,
                            int shiftPts, int fftlen,
                            Ipp8u *pBuffer, IppsDFTSpec_C_32fc *pSpec,
                            Ipp64f *productpeaks, int *freqlist_inds, int NUM_THREADS){

    // computations
    int i, curr_shift;
    Ipp64f y_pwr;
    Ipp32fc *dft_in = (Ipp32fc*)ippsMalloc_32fc_L(fftlen);
    Ipp32fc *dft_out = (Ipp32fc*)ippsMalloc_32fc_L(fftlen);
    Ipp32f *magnSq = (Ipp32f*)ippsMalloc_32f_L(fftlen);
    Ipp32f maxval;
    int maxind;

    // pick point based on thread number
    for (i = t_ID; i<shiftPts; i=i+NUM_THREADS){
        curr_shift = shifts[i];

        if (curr_shift == 100){
            for (int a = 0; a < 5; a++){
                printf("%f, %f \n", y[curr_shift+a].re, y[curr_shift+a].im);
            }

        }

        ippsSum_64f(&y_absSq[curr_shift], fftlen, &y_pwr);

        ippsMul_32fc(cutout,&y[curr_shift], dft_in, fftlen);

        ippsDFTFwd_CToC_32fc(dft_in, dft_out, pSpec, pBuffer);

        ippsPowerSpectr_32fc(dft_out, magnSq, fftlen);

        ippsMaxIndx_32f(magnSq, fftlen, &maxval, &maxind);

        productpeaks[i] = (Ipp64f)maxval/cutout_pwr/y_pwr;
        freqlist_inds[i] = maxind;
    }

    ippsFree(dft_in); ippsFree(dft_out);
    ippsFree(magnSq);

}


void Processor::getOptions(QVector<QString> &optlabels, QVector<int> &opts){
    optlabels.push_back("Channeliser Threads");
    opts.push_back(WOLA_NUMTHREADS);
}

void Processor::setOptions(QVector<int> opts){
    qDebug() << "setting new options";
    WOLA_NUMTHREADS = opts.at(0);
}

