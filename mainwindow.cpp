#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // sample plot
//    sampleQwtPlot();

    // initialise to null?
    processor = nullptr;

}

MainWindow::~MainWindow()
{
    // clear the processor?
//    delete processor;

    delete ui;
}

void MainWindow::plotChannelisedTimeData(){
    QwtPlot *p = new QwtPlot(this);

    ui->verticalLayout->addWidget(p);

    p->setTitle("Channel vs Time");

    p->setAxisTitle(p->xBottom, "Time (s)");
    p->setAxisTitle(p->yLeft, "Channel Magnitude");

    QwtPlotCurve *curve = new QwtPlotCurve("Channel vs Time");

    curve->setRawSamples(processor->chnl_t, processor->chnl_abs, processor->getNprimePts());

    curve->attach(p);

    // make the zoomer
    QwtPlotZoomer *zoomer = new QwtPlotZoomer(p->canvas());
    zoomer->setZoomBase();


    p->replot();

}

void MainWindow::plotChannelisedFreqData(){
    QwtPlot *p = new QwtPlot(this);

    ui->verticalLayout->addWidget(p);

    p->setTitle("Channel Spectrum");

    p->setAxisTitle(p->xBottom, "Frequency (Hz)");
    p->setAxisTitle(p->yLeft, "Channel Power (dB)");

    QwtPlotCurve *curve = new QwtPlotCurve("Channel vs Frequency");

    curve->setRawSamples(processor->chnl_f, processor->chnl_spectrum, processor->getNprimePts());

    curve->attach(p);

    // make the zoomer
    QwtPlotZoomer *zoomer = new QwtPlotZoomer(p->canvas());
    zoomer->setZoomBase();


    p->replot();

}



void MainWindow::on_startBtn_clicked()
{
    // init the processor
    if (processor == nullptr){
        processor = new Processor(ui->fsEdit->text().toInt(),
                                  ui->chnBWEdit->text().toInt(),
                                  ui->numTapsEdit->text().toInt(),
                                  ui->chnlIdxEdit->text().toInt());
    }
    else{
        qDebug()<<"processor already initialised";
    }

    // open the rawfiles
    QStringList filepaths;
    for (int i = 0; i < ui->filesList->count(); i++){
        filepaths.append(ui->filesList->item(i)->text());
    }
    qDebug()<<"Loading raw files" << filepaths;

    int retcode = processor->LoadRawFiles_int16(filepaths);
    qDebug() << retcode;

    // run the channeliser and detach thread so we dont freeze
    connect(processor, SIGNAL(ChanneliserFinished()), this, SLOT(on_ChanneliserFinished()));
    std::thread t(&Processor::ChanneliseStart, processor);
    t.detach();
}

void MainWindow::on_ChanneliserFinished(){
    qDebug()<<"picked up channeliser finishing";

//    // allocate memory for array first
//    ippsFree(chnl_t);
//    ippsFree(chnl_abs);
//    chnl_t = ippsMalloc_64f_L(processor->getNprimePts());
//    chnl_abs = ippsMalloc_64f_L(processor->getNprimePts());

    connect(processor, SIGNAL(ChannelTimeFreqDataFinished()), this, SLOT(on_ChannelTimeFreqDataFinished()));
    std::thread t(&Processor::makeChannelTimeFreqData, processor);
    t.detach();
}

void MainWindow::on_ChannelTimeFreqDataFinished(){
    qDebug()<<"picked up channel timefreq data finished";

    // let's check some data
    for (int i = 0; i < 10; i++){
        qDebug() << processor->chnl_f[i] << ", " << processor->chnl_spectrum[i];
    }

    plotChannelisedTimeData();
    plotChannelisedFreqData();
}



void MainWindow::on_selectFilesBtn_clicked()
{

    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Open Raw Files"), "D:\\SignalObserver", tr("Binary Files (*.bin *.dat)"));

    // clear the list
    ui->filesList->clear();

    // append to the list
    ui->filesList->addItems(filenames);
}

void MainWindow::on_actionOptions_triggered()
{
    qDebug()<<"in options";

    MyDialog *d = new MyDialog();

    QVector<QString> curroptlabels;
    QVector<int> curropts;

    // populate current options
    processor->getOptions(curroptlabels, curropts);

    qDebug()<<curroptlabels;
    qDebug()<<curropts;

    for (int i = 0; i < curroptlabels.size(); i++){
        d->addFormRow(curroptlabels.at(i), QString::number(curropts.at(i)));
    }


    // connect the slot
    connect(d, SIGNAL(dialogFormParams(QVector<int>)), processor, SLOT(setOptions(QVector<int>)));

    d->exec();
}



void MainWindow::on_preambleFileSelectBtn_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Raw Files"), "D:\\SignalObserver", tr("Binary Files (*.bin *.dat)"));

    // clear the list
    ui->preambleFileEdit->clear();

    // append to the list
    ui->preambleFileEdit->setText(filename);
}





void MainWindow::sampleQwtPlot(){
    FILE *fp = fopen("D:\\SignalObserver\\data.bin","r");
    if (fp!=NULL){
        data.resize(4000000);
        fread(&data[0], sizeof(double), 4000000, fp);
        fclose(fp);

        QwtPlot *p = new QwtPlot(this);

        ui->centralWidget->layout()->addWidget(p);

        p->setTitle("eg title");

        p->setAxisTitle(p->xBottom, "x");
        p->setAxisTitle(p->yLeft, "x");

        QwtPlotCurve *curve = new QwtPlotCurve("FFT");

        curve->setRawSamples(&data[0], &data[2000000], 2000000);

        curve->attach(p);

        // make the zoomer
        QwtPlotZoomer *zoomer = new QwtPlotZoomer(p->canvas());
        zoomer->setZoomBase();


        p->replot();

    }
}
