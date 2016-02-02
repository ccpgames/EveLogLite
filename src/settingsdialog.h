#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    virtual void accept();
private:
    Ui::SettingsDialog *ui;
private slots:
    void browseForAutoSaveDirectory();
};

#endif // SETTINGSDIALOG_H
