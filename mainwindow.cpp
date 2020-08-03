#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    samplePlot();
    sampleQwtPlot();
}

MainWindow::~MainWindow()
{
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
