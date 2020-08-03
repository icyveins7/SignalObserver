#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_plot_zoomer.h>
#include "processor.h"
#include <QFileDialog>
#include <QDebug>

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

private slots:
    void on_startBtn_clicked();

    void on_selectFilesBtn_clicked();

private:
    Ui::MainWindow *ui;

    QVector<double> data;

    Processor *processor;
};

#endif // MAINWINDOW_H
