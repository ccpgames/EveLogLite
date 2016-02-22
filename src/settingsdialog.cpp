#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QStandardPaths>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowTitle("Settings");
    QSettings settings;
    ui->background->setCurrentIndex(settings.value("colorBackground", 0).toInt());
    ui->breakLines->setChecked(settings.value("breakLines", 0).toBool());
    ui->maxMessages->setValue(settings.value("maxMessages", 10000).toInt());
    if (settings.contains("autoSaveDirectory"))
    {
        ui->autoSaveDirectory->setText(settings.value("autoSaveDirectory").toString());
    }
    else
    {
        ui->autoSaveDirectory->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    ui->timestampFormat->setCurrentIndex(settings.value("timestampPrecision", 0).toInt());
    ui->monospaceFont->setChecked(settings.value("monospaceFont", 0).toBool());

    connect(ui->browseAutoSave, &QPushButton::clicked, this, &SettingsDialog::browseForAutoSaveDirectory);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    auto autoSave = QFileInfo(ui->autoSaveDirectory->text());
    if (!autoSave.exists())
    {
        QDir().mkpath(autoSave.absoluteFilePath());

    }
    if (!autoSave.exists() || !autoSave.isDir() || !autoSave.isWritable())
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Warning);
        msg.setText("Can not access auto save directory");
        msg.setInformativeText(QString("Can not access auto save directory %1").arg(autoSave.absoluteFilePath()));
        msg.exec();
        return;
    }
    QSettings settings;
    settings.setValue("colorBackground", ui->background->currentIndex());
    settings.setValue("breakLines", ui->breakLines->isChecked());
    settings.setValue("maxMessages", ui->maxMessages->value());
    settings.setValue("autoSaveDirectory", ui->autoSaveDirectory->text());
    settings.setValue("timestampPrecision", ui->timestampFormat->currentIndex());
    settings.setValue("monospaceFont", ui->monospaceFont->isChecked());
    QDialog::accept();
}

void SettingsDialog::browseForAutoSaveDirectory()
{
    auto dir = QFileDialog::getExistingDirectory(this, "Choose Auto Save Directory", ui->autoSaveDirectory->text());
    if (dir.length())
    {
        ui->autoSaveDirectory->setText(dir);
    }
}
