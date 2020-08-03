#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    sampleQwtPlot();

    // initialise to null?
    processor = nullptr;
}

MainWindow::~MainWindow()
{
    // clear the processor?
//    delete processor;

    delete ui;
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

void MainWindow::on_startBtn_clicked()
{
    // init the processor
    if (processor == nullptr){
        processor = new Processor(ui->fsEdit->text().toInt());
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
}

void MainWindow::on_selectFilesBtn_clicked()
{

    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("Open Raw Files"), "C:\\", tr("Binary Files (*.bin *.dat)"));

    // clear the list
    ui->filesList->clear();

    // append to the list
    ui->filesList->addItems(filenames);
}
