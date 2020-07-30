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
    FILE *fp = fopen("F:\\SignalObserver\\data.bin","r");
    data.resize(4000000);
    fread(&data[0], sizeof(double), 4000000, fp);
    fclose(fp);

    QwtPlot *p = new QwtPlot(this);

    ui->centralWidget->layout()->addWidget(p);

    p->setTitle("eg title");

    p->setAxisTitle(p->xBottom, "x");
    p->setAxisTitle(p->yLeft, "x");

    QwtPlotCurve *curve = new QwtPlotCurve("FFT");

//    QwtSeriesData<

//    curve->setData()
    curve->setRawSamples(&data[0], &data[2000000], 2000000);

    curve->attach(p);

    // make the zoomer
    QwtPlotZoomer *zoomer = new QwtPlotZoomer(p->canvas());
    zoomer->setZoomBase();


    p->replot();


}

void MainWindow::samplePlot(){
    QLineSeries *s = new QLineSeries();

    FILE *fp = fopen("F:\\SignalObserver\\data.bin","r");
    data.resize(4000000);
    fread(&data[0], sizeof(double), 4000000, fp);
    fclose(fp);

    for (int i = 0; i < 2000000; i++){
        s->append(data.at(i), data.at(i+2000000));
    }

//    s->append(0,6);
//    s->append(2,7);
//    s->append(3,4);
//    s->append(7,8);

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(s);
    chart->createDefaultAxes();
    chart->setTitle("title");

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

//    ui->verticalLayout_2->addWidget(chartView);
    ui->centralWidget->layout()->addWidget(chartView);
}
