#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      settings("settings.ini", QSettings::IniFormat, this)
{
    ui->setupUi(this);

    restoreRepositories();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    this->restoreState(settings.value("windows/state").toByteArray());
    this->restoreGeometry(settings.value("windows/geometry").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    settings.setValue("windows/state", this->saveState());
    settings.setValue("windows/geometry", this->saveGeometry());
    QMainWindow::closeEvent(e);
}

void MainWindow::on_addRepositoryButton_clicked()
{
    QString lastPath = settings.value("path/selected_repository").toString();
    QString path = QFileDialog::getExistingDirectory(this, "选择 Git 仓库", lastPath);
    if (path.isEmpty())
        return ;
    settings.setValue("path/selected_repository", path);

    QDir repDir(path);
    if (!QFileInfo(repDir.absoluteFilePath(".git")).exists())
    {
        QMessageBox::warning(this, "选择 Git 仓库", "您选择的路径不是一个 Git 仓库");
        return ;
    }

    QListWidgetItem* item = new QListWidgetItem(path);
    item->setData(Qt::DisplayRole, path);
    item->setData(Qt::CheckStateRole, Qt::Checked);
    ui->repositoriesListWidget->addItem(item);

    saveRepositories();
}

void MainWindow::on_removeRepositoryButton_clicked()
{
    if (ui->repositoriesListWidget->currentRow() == -1)
        return ;

    ui->repositoriesListWidget->takeItem(ui->repositoriesListWidget->currentRow());
    saveRepositories();
}

void MainWindow::saveRepositories()
{
    int count = ui->repositoriesListWidget->count();
    settings.setValue("repository/count", count);
    for (int i = 0; i < count; i++)
    {
        settings.setValue("repository/path_" + QString::number(i), ui->repositoriesListWidget->item(i)->data(Qt::DisplayRole));
        settings.setValue("repository/check_" + QString::number(i), true);
    }
}

void MainWindow::restoreRepositories()
{
    ui->repositoriesListWidget->clear();
    int count = settings.value("repository/count").toInt();
    for (int i = 0; i < count; i++)
    {
        QString path = settings.value("repository/path_" + QString::number(i)).toString();
        if (path.isEmpty())
            continue;
        bool checked = settings.value("repository/check_" + QString::number(i)).toBool();

        QListWidgetItem* item = new QListWidgetItem(path);
        item->setData(Qt::DisplayRole, path);
        item->setData(Qt::CheckStateRole, checked ? Qt::Checked : Qt::Unchecked);
        ui->repositoriesListWidget->addItem(item);
    }
}
