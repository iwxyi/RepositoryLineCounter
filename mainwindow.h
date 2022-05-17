#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;


private slots:
    void on_addRepositoryButton_clicked();

    void on_removeRepositoryButton_clicked();

private:
    void saveRepositories();
    void restoreRepositories();

private:
    Ui::MainWindow *ui;
    QSettings settings;
};
#endif // MAINWINDOW_H
