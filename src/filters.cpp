#include "filters.h"
#include "ui_filters.h"

Filters::Filters(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Filters)
{
    ui->setupUi(this);
}

Filters::~Filters()
{
    delete ui;
}
