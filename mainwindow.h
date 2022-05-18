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
    void on_pushButton_clicked();

    void on_addRepositoryButton_clicked();

    void on_removeRepositoryButton_clicked();

    void on_repositoriesListWidget_itemChanged(QListWidgetItem *);

    void on_sinceTimeLineEdit_editingFinished();

    void on_untilTimeLineEdit_editingFinished();

    void on_authorsLineEdit_editingFinished();

    void on_startButton_clicked();

    void on_repositoriesListWidget_itemClicked(QListWidgetItem *item);

    void on_excludesLineEdit_editingFinished();

    void on_paramsTextEdit_textChanged();

private:
    void saveRepositories();
    void restoreConfigs();

signals:
    void progressChanged(int current, int total);

private:
    Ui::MainWindow *ui;
    QSettings settings;

    QMap<QString, QMap<QString, QList<int>>> statisticRepoResults;
    QMap<QString, QList<int>> statisticUserResults;
};
#endif // MAINWINDOW_H
