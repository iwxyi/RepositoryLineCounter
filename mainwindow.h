#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QListWidgetItem>

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

    void on_repositoriesListWidget_itemChanged(QListWidgetItem *);

    void on_sinceTimeLineEdit_editingFinished();

    void on_untilTimeLineEdit_editingFinished();

    void on_authorsLineEdit_editingFinished();

    void on_excludesTextEdit_textChanged();

    void on_startButton_clicked();

private:
    void saveRepositories();
    void restoreConfigs();

private:
    Ui::MainWindow *ui;
    QSettings settings;

    QMap<QString, QMap<QString, QList<int>>> statisticResults;
    QMap<QString, QList<int>> statisticUserResults;
};
#endif // MAINWINDOW_H
