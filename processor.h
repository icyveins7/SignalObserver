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

class Processor
{
public:
    Processor(int in_fs);
    ~Processor();

    int initRawMemory(int len);

    int LoadRawFiles_int16(QStringList filepaths);

private:
    // parameters
    int fs;

    // array data
    Ipp16sc *rawdata;

};

#endif // PROCESSOR_H
