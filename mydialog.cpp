#include "mydialog.h"
#include "ui_mydialog.h"

MyDialog::MyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyDialog)
{
    ui->setupUi(this);
}


void MyDialog::addFormRow(QString l, QString v){
    ui->formlayout->addRow(l, new QLineEdit(v));
}

MyDialog::~MyDialog()
{
    delete ui;
}

void MyDialog::on_buttonBox_accepted()
{
    // take the whole form and append all of it
    QVector<int> params;
    for (int i = 0; i < ui->formlayout->rowCount(); i++){
        QLineEdit *e = (QLineEdit*)ui->formlayout->itemAt(i, QFormLayout::FieldRole)->widget();
        params.push_back(e->text().toInt());
    }

    emit(dialogFormParams(params));
}
