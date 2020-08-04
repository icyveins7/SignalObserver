#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_plot_zoomer.h>
#include "processor.h"
#include "mydialog.h"
#include <QFileDialog>
#include <QDebug>
#include <QFormLayout>
#include <QDialogButtonBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void sampleQwtPlot();

    void plotChannelisedTimeData();

private slots:
    void on_startBtn_clicked();

    void on_selectFilesBtn_clicked();

    void on_actionOptions_triggered();

    // all the processor waiting slots
    void on_ChanneliserFinished();
    void on_ChannelTimeDataFinished();

private:
    Ui::MainWindow *ui;

    QVector<double> data;

    Processor *processor;
};

#endif // MAINWINDOW_H
