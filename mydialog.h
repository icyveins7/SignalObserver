#ifndef MYDIALOG_H
#define MYDIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace Ui {
class MyDialog;
}

class MyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MyDialog(QWidget *parent = nullptr);
    ~MyDialog();

    void addFormRow(QString l, QString v);

signals:
    void dialogFormParams(QVector<int>);

private slots:
    void on_buttonBox_accepted();

private:
    Ui::MyDialog *ui;
};

#endif // MYDIALOG_H
