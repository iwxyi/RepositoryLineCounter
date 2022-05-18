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
#include <QScrollArea>
#include <QTableWidget>
#include <QScrollBar>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      settings("settings.ini", QSettings::IniFormat, this)
{
    ui->setupUi(this);

    connect(this, &MainWindow::progressChanged, this, [=](int current, int total) {

    });

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

void MainWindow::on_pushButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择 Git 安装目录");
    if (path.isEmpty())
        return ;

    QDir dir(path);
    if (dir.exists("bin"))
    {
        path = dir.absoluteFilePath("bin");
        dir = QDir(path);
    }

    if (!dir.exists("git.exe"))
    {

        return ;
    }

    ui->gitPathEdit->setText(path);
    settings.setValue("path/git_bin", path);
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

    ui->gitPathEdit->setText(settings.value("path/git_bin").toString());
    ui->sinceTimeLineEdit->setText(settings.value("statistic/since").toString());
    ui->untilTimeLineEdit->setText(settings.value("statistic/until").toString());
    ui->authorsLineEdit->setText(settings.value("statistic/authors").toString());
    ui->excludesLineEdit->setText(settings.value("statistic/excludes").toString());
    ui->paramsTextEdit->setPlainText(settings.value("statistic/params").toString());
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

void MainWindow::on_startButton_clicked()
{
    const int VAL_NUM = 3;

    /// 解析配置
    if (ui->gitPathEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "需要获取 Git 路径", "请先配置 Git/bin 所在的文件夹");
        return ;
    }
    if (!QDir(ui->gitPathEdit->text()).exists("bash.exe"))
    {
        QMessageBox::warning(this, "找不到 bash.exe", "文件 " + QDir(ui->gitPathEdit->text()).absoluteFilePath("bash.exe") + " 不存在！");
        return ;
    }
    QString bashPath = QDir(ui->gitPathEdit->text()).absoluteFilePath("bash.exe");

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
    statisticRepoResults.clear();
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
    QStringList excludes = ui->excludesLineEdit->text().split(" ", QString::SkipEmptyParts);
    for (int i = 0; i < excludes.size(); i++)
        excludes[i] = "\":(exclude)" + excludes.at(i) + "\"";
    QString expExclude = excludes.join(" ");

    // 其余命令
    QStringList params = ui->paramsTextEdit->toPlainText().split("\n", QString::SkipEmptyParts);
    QString expCustom = params.join(" ");


    // 遍历仓库
    QStringList repositoryList, repositoryNameList;
    for (int i = 0; i < ui->repositoriesListWidget->count(); i++)
    {
        QListWidgetItem* item = ui->repositoriesListWidget->item(i);
        if (item->data(Qt::CheckStateRole).toInt() == Qt::Checked)
        {
            QString path = item->data(Qt::DisplayRole).toString();
            if (!QDir(path).exists())
                continue;
            repositoryList.append(path);
            repositoryNameList.append(QDir(path).dirName());
        }
    }

    // 输出
//    qInfo() << "repositories:" << repositoryList;
//    qInfo() << "authors:" << expAuthorList;
//    qInfo() << "times:" << expSince << expUntil;
//    qInfo() << "excludes:" << expExclude;

    if (expAuthorList.empty() || repositoryList.empty())
        return ;

    /// 开始统计
    ui->startButton->setText("正在统计");
    ui->progressBar->setRange(0, repositoryList.size() * expAuthorList.size());
    int progressCount = 0;
    for (int i = 0; i < repositoryList.size(); i++) // 遍历仓库
    {
        QString repo = repositoryNameList.at(i);
        for (int j = 0; j < expAuthorList.size(); j++) // 遍历开发者
        {
            QString user = displayNames.at(j);
            QString script = "awk '{ add += $1; subs += $2; loc += $1 - $2 } END { printf \"%s,%s,%s\", add, subs, loc }'";
            QString cmd = QString("git log %1 %2 %3 --pretty=tformat: --numstat -- . %4 %6 | %5")
                    .arg(expAuthorList.at(j))
                    .arg(expSince)
                    .arg(expUntil)
                    .arg(expExclude)
                    .arg(script)
                    .arg(expCustom);

//            qInfo() << cmd;
            ui->logEdit->appendPlainText(cmd);
            QProcess p;
            p.setWorkingDirectory(repositoryList.at(i));
            p.start(bashPath, QStringList() << "-c" << cmd);
            QEventLoop loop;
            connect(&p,static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), &loop, &QEventLoop::quit);
            loop.exec();//可操作界面
            // loop.exec(QEventLoop::ExcludeUserInputEvents);//不可操作界面
            // if (p.waitForFinished(30000))
            if (p.error() == QProcess::ProcessError::UnknownError)
            {
                QString result = p.readAll();
                qInfo() << repo << user << result;
                ui->logEdit->appendPlainText(repo + " " + user + " " + result);

                QStringList nums = result.split(",");
                if (nums.size() != VAL_NUM)
                {
                    qWarning() << "无效的Git结果";
                    continue;
                }

                QList<int>& sums = statisticUserResults[user];
                QList<int> vals;
                for (int k = 0; k < VAL_NUM; k++)
                {
                    sums[k] += nums.at(k).toInt();
                    vals.append(nums.at(k).toInt());
                }

                statisticRepoResults[repo][user] = vals;
            }
            else
            {
                qWarning() << "QProcess.error:" << p.error() << p.readAllStandardError();
            }
            ui->progressBar->setValue(++progressCount);
        }
    }
    ui->startButton->setText("开始统计");

    /// 打印结果
    qInfo() << "--------- result ----------";
    for (int i = 0; i < displayNames.size(); i++)
    {
        QList<int> vals = statisticUserResults[displayNames.at(i)];
        qInfo() << displayNames.at(i) << vals;
        ui->logEdit->appendPlainText(QString("total %1: %2 %3 %4").arg(displayNames.at(i)).arg(vals[0]).arg(vals[1]).arg(vals[2]));
    }
    qInfo() << "--------- ^^^^^^ ----------";

    // 添加总计到输出中
    if (repositoryList.size() > 1)
    {
        repositoryNameList.append("[total]");
        statisticRepoResults["[total]"] = statisticUserResults;
    }

    /// 输出表格
    QDialog* dialog = new QDialog(this);
    QVBoxLayout* dialogLayout = new QVBoxLayout(dialog);
    QScrollArea* area = new QScrollArea(dialog);
    dialogLayout->addWidget(area);
    QWidget* contentWidget = new QWidget(dialog);
    area->setWidget(contentWidget);
    QVBoxLayout* vlayout = new QVBoxLayout(contentWidget);

    // 遍历仓库，打印每个仓库每个人员的代码量
    for (int i = 0; i < repositoryNameList.size(); i++)
    {
        QString repo = repositoryNameList.at(i);
        vlayout->addWidget(new QLabel(repo, dialog));

        QTableWidget* table = new QTableWidget(dialog);
        vlayout->addWidget(table);
        table->setRowCount(displayNames.size());
        table->setColumnCount(VAL_NUM);
        table->setHorizontalHeaderLabels({"创建行", "删除行", "有效行"});
        table->setVerticalHeaderLabels(displayNames);

        for (int j = 0; j < displayNames.size(); j++)
        {
            QList<int>& vals = statisticRepoResults[repo][displayNames.at(j)];
            if (vals.empty())
                continue;
            for (int k = 0; k < VAL_NUM; k++)
            {
                table->setItem(j, k, new QTableWidgetItem(QString::number(vals[k])));
            }
        }

        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        table->adjustSize();
        table->setFixedHeight(table->horizontalHeader()->height() * (displayNames.size() + 1) + table->contentsMargins().top() + table->contentsMargins().bottom());
    }

    // 打印最终代码量

    contentWidget->adjustSize();
    dialog->setWindowTitle("统计结果");
    dialog->setGeometry(this->geometry());
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::on_repositoriesListWidget_itemClicked(QListWidgetItem *item)
{
    if (!item)
        return ;
    bool checked = item->data(Qt::CheckStateRole).toBool();
    item->setData(Qt::CheckStateRole, checked ? Qt::Unchecked : Qt::Checked);
}

void MainWindow::on_excludesLineEdit_editingFinished()
{
    settings.setValue("statistic/excludes", ui->excludesLineEdit->text());
}

void MainWindow::on_paramsTextEdit_textChanged()
{
    settings.setValue("statistic/params", ui->paramsTextEdit->toPlainText());
}
