#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      settings("settings.ini", QSettings::IniFormat, this)
{
    ui->setupUi(this);

    restoreConfigs();
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

void MainWindow::on_repositoriesListWidget_itemChanged(QListWidgetItem *)
{
    saveRepositories();
}

void MainWindow::saveRepositories()
{
    int count = ui->repositoriesListWidget->count();
    settings.setValue("repository/count", count);
    for (int i = 0; i < count; i++)
    {
        QListWidgetItem* item = ui->repositoriesListWidget->item(i);
        settings.setValue("repository/path_" + QString::number(i), item->data(Qt::DisplayRole));
        settings.setValue("repository/check_" + QString::number(i), item->data(Qt::CheckStateRole) == Qt::Checked);
    }
}

void MainWindow::restoreConfigs()
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

    ui->sinceTimeLineEdit->setText(settings.value("statistic/since").toString());
    ui->untilTimeLineEdit->setText(settings.value("statistic/until").toString());
    ui->authorsLineEdit->setText(settings.value("statistic/authors").toString());
    ui->excludesTextEdit->setPlainText(settings.value("statistic/excludes").toString());
}

void MainWindow::on_sinceTimeLineEdit_editingFinished()
{
    settings.setValue("statistic/since", ui->sinceTimeLineEdit->text());
}

void MainWindow::on_untilTimeLineEdit_editingFinished()
{
    settings.setValue("statistic/until", ui->untilTimeLineEdit->text());
}

void MainWindow::on_authorsLineEdit_editingFinished()
{
    settings.setValue("statistic/authors", ui->authorsLineEdit->text());
}

void MainWindow::on_excludesTextEdit_textChanged()
{
    settings.setValue("statistic/excludes", ui->excludesTextEdit->toPlainText());
}

void MainWindow::on_startButton_clicked()
{
    // 检验时间
    QString since = ui->sinceTimeLineEdit->text();
    if (!since.contains(QRegularExpression("^\\d{4}-\\d{1,2}-\\d{1,2}$")))
    {
        QMessageBox::warning(this, "开始时间错误", since + "\n不符合时间格式：yyyy-MM-dd");
        return ;
    }
    QString until = ui->untilTimeLineEdit->text();
    if (until.isEmpty())
        until = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    if (!until.contains(QRegularExpression("^\\d{4}-\\d{1,2}-\\d{1,2}$")))
    {
        QMessageBox::warning(this, "结束时间错误", until + "\n不符合时间格式：yyyy-MM-dd");
        return ;
    }
    QString expSince = "--since=\"" + since + "\"";
    QString expUntil = "--until=\"" + until + "\"";


    // 获取姓名
    statisticResults.clear();
    statisticUserResults.clear();
    QStringList expAuthorList;
    QStringList namess = ui->authorsLineEdit->text().split(",", QString::SkipEmptyParts);
    QStringList displayNames;
    for (int i = 0; i < namess.size(); i++)
    {
        QStringList names = namess.at(i).trimmed().split(" ", QString::SkipEmptyParts);
        displayNames.append(names.first());
        statisticUserResults.insert(names.first(), QList<int>{0, 0, 0});

        QStringList nameList;
        for (int j = 0; j < names.size(); j++)
            nameList.append("--author=\"" + names.at(j).trimmed() + "\"");
        expAuthorList.append(nameList.join(" "));
    }

    // 获取exclude
    QStringList excludes = ui->excludesTextEdit->toPlainText().split(" ");
    for (int i = 0; i < excludes.size(); i++)
        excludes[i] = "\":(exclude)" + excludes.at(i) + "\"";
    QString expExclude = excludes.join(" ");


    // 遍历仓库
    QStringList repositoryList;
    for (int i = 0; i < ui->repositoriesListWidget->count(); i++)
    {
        QListWidgetItem* item = ui->repositoriesListWidget->item(i);
        if (item->data(Qt::CheckStateRole).toInt() == Qt::Checked)
        {
            QString path = item->data(Qt::DisplayRole).toString();
            if (!QDir(path).exists())
                continue;
            repositoryList.append(path);
        }
    }

    // 输出
//    qInfo() << "repositories:" << repositoryList;
//    qInfo() << "authors:" << expAuthorList;
//    qInfo() << "times:" << expSince << expUntil;
//    qInfo() << "excludes:" << expExclude;

    if (expAuthorList.empty() || repositoryList.empty())
        return ;

    // 开始统计
    ui->startButton->setText("正在统计");
    for (int i = 0; i < repositoryList.size(); i++) // 遍历仓库
    {

        for (int j = 0; j < expAuthorList.size(); j++) // 遍历开发者
        {
            QString script = "awk '{ add += $1; subs += $2; loc += $1 - $2 } END { printf \"%s,%s,%s\", add, subs, loc }'";
            QString cmd = QString("git log %1 %2 %3 --pretty=tformat: --numstat -- . %4 | %5")
                    .arg(expAuthorList.at(j))
                    .arg(expSince)
                    .arg(expUntil)
                    .arg(expExclude)
                    .arg(script);

//            qInfo() << cmd;
            ui->logEdit->appendPlainText(cmd);
            QProcess p;
            p.setWorkingDirectory(repositoryList.at(i));
            p.start("A:\\Git\\bin\\bash.exe", QStringList() << "-c" << cmd);
            if (p.waitForFinished(30000))
            {
                QString result = p.readAll();
                qInfo() << QDir(repositoryList.at(i)).dirName() << displayNames.at(j) << result;
                ui->logEdit->appendPlainText(result);

                QStringList nums = result.split(",");
                if (nums.size() != 3)
                {
                    qWarning() << "无效的Git结果";
                    continue;
                }

                long long added = nums.at(0).toLongLong();
                long long removed = nums.at(1).toLongLong();
                long long local = nums.at(2).toLongLong();
                QList<int>& vals = statisticUserResults[displayNames.at(j)];
                vals[0] += added;
                vals[1] += removed;
                vals[2] += local;
            }
            else
            {
                qWarning() << "QProcess.error:" << p.error() << p.readAllStandardError();
            }
        }
    }
    ui->startButton->setText("开始统计");

    // 显示结果
    qInfo() << "--------- result ----------";
    for (int i = 0; i < displayNames.size(); i++)
    {
        QList<int> vals = statisticUserResults[displayNames.at(i)];
        qInfo() << displayNames.at(i) << vals;
        ui->logEdit->appendPlainText(QString("%1: %2 %3 %4").arg(displayNames.at(i)).arg(vals[0]).arg(vals[1]).arg(vals[2]));
    }
    qInfo() << "--------- ^^^^^^ ----------";
}


