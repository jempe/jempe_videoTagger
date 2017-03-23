#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    player = new QMediaPlayer;
    player->setVideoOutput(ui->player);

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    ui->screenshotButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    ui->slider->setRange(0,0);
    connect(ui->slider, SIGNAL(sliderMoved(int)), SLOT(setPosition(int)));

    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);

    connect(player, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    connect(player, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(stateChanged(QMediaPlayer::State)));

    connect(ui->elapsed, SIGNAL(valueChanged(int)), SLOT(setPausedPosition(int)));

    connect(ui->openButton, SIGNAL(clicked(bool)), SLOT(openFile()));
    connect(ui->saveButton, SIGNAL(clicked(bool)), SLOT(saveJSON()));

    connect(ui->screenshotButton, SIGNAL(clicked(bool)), SLOT(saveScreenshot()));

    myProcess = new QProcess(this);
    connect(myProcess,SIGNAL(readyReadStandardOutput()),SLOT(readyReadStandardOutput()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onPlayButtonClicked()
{
    if(player->state() == QMediaPlayer::PlayingState)
    {
       player->pause();
    }
    else
    {
        player->play();
    }
}

void MainWindow::positionChanged(qint64 progress)
{
    ui->slider->setValue(progress);
    ui->elapsed->setValue(progress);
}

void MainWindow::durationChanged(qint64 duration)
{
    ui->slider->setRange(0, duration);
    ui->elapsed->setRange(0, duration);
}

void MainWindow::stateChanged(QMediaPlayer::State)
{
    if(player->state() == QMediaPlayer::PlayingState)
    {
       ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
    else
    {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void MainWindow::setPosition(int position)
{
    player->setPosition(position);
}

void MainWindow::setPausedPosition(int position)
{
    if(player->state() == QMediaPlayer::PausedState)
    {
        player->setPosition(position);
    }
}

void MainWindow::openFile()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Movie"));
    QStringList supportedMimeTypes;
    supportedMimeTypes << "video/mp4";

    if (!supportedMimeTypes.isEmpty())
        fileDialog.setMimeTypeFilters(supportedMimeTypes);
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
    if (fileDialog.exec() == QDialog::Accepted)
        setUrl(fileDialog.selectedUrls().constFirst());
}

void MainWindow::setUrl(const QUrl &url)
{
    setWindowFilePath(url.isLocalFile() ? url.toLocalFile() : QString());
    player->setMedia(url);
    ui->playButton->setEnabled(true);
    ui->screenshotButton->setEnabled(true);

    fileName = url.fileName();
    folderPath = url.path().replace(QRegExp(fileName + "$"), "");

    loadJSON();
}

void MainWindow::saveScreenshot()
{
    qint64 position = player->position();
    if( position > 1000)
    {
        QString videoFile = folderPath + fileName;
        QString screenshotFile = videoFile + ".jpg";

        qDebug() << videoFile;

        /*QString program = "ffplay";
        QStringList arguments;
        arguments << videoFile;
        myProcess->start(program, arguments);*/

        QString program = "ffmpeg";

        QString positionTime = QString::number(position,10);

        QString floatTime = positionTime.left(positionTime.length() - 3);
        floatTime.append(".");
        floatTime.append(positionTime.right(3));

        qDebug() << floatTime;

        QStringList arguments;
        arguments.append("-ss");
        arguments.append(floatTime);
        arguments.append("-i");
        arguments.append(videoFile);
        arguments.append("-vframes");
        arguments.append("1");
        arguments.append("-q:v");
        arguments.append("2");
        arguments.append(screenshotFile);

        myProcess->start(program, arguments);
    }
}

bool MainWindow::loadJSON()
{
    QString sourceFile;
    QString videoFileName = fileName;
    sourceFile = folderPath + videoFileName.replace(QRegExp(".mp4$"), ".json");

    QFile loadFile(sourceFile);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open file.");
        return false;
    }

    QByteArray videoData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(videoData));

    QJsonObject videoJSON = loadDoc.object();

    ui->title->setText(videoJSON["title"].toString());
    ui->url->setText(videoJSON["url"].toString());
    ui->image->setText(videoJSON["image"].toString());
    ui->description->setText(videoJSON["description"].toString());
    ui->md5->setText(videoJSON["md5sum"].toString());
    ui->release->setDate(QDate::fromString(videoJSON["releaseDate"].toString(), "yyyy-MM-dd"));

    QString actors = "";

    QJsonArray actorsList = videoJSON["actors"].toArray();
    for (int actorIndex = 0; actorIndex < actorsList.size(); ++actorIndex) {
        if(actorIndex > 0)
        {
            actors.append(", ");
        }

        actors.append(actorsList[actorIndex].toString());
    }

    ui->actors->setText(actors);

    QString categories = "";

    QJsonArray categoriesList = videoJSON["categories"].toArray();
    for (int categoryIndex = 0; categoryIndex < categoriesList.size(); ++categoryIndex) {
        if(categoryIndex > 0)
        {
            categories.append(", ");
        }

        categories.append(categoriesList[categoryIndex].toString());
    }

    ui->categories->setText(categories);

    return true;

}

bool MainWindow::saveJSON()
{
    QString targetFile;
    QString videoFileName = fileName;
    targetFile = folderPath + videoFileName.replace(QRegExp(".mp4$"), ".json");

    QFile saveFile(targetFile);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject videoJSON;
    videoJSON["title"] = ui->title->text();
    videoJSON["url"] = ui->url->text();
    videoJSON["image"] = ui->image->text();
    videoJSON["description"] = ui->description->toPlainText();
    videoJSON["releaseDate"] = ui->release->text();
    videoJSON["md5sum"] = ui->md5->text();

    QStringList actors = ui->actors->text().split(",");
    for (int actorIndex = 0; actorIndex < actors.size(); ++actorIndex) {
        QString actorName = actors.at(actorIndex);
        actors[actorIndex] = actorName.simplified();
    }

    QJsonArray actorsList = QJsonArray::fromStringList(actors);

    videoJSON["actors"] = actorsList;

    QStringList categories = ui->categories->text().split(",");
    for (int categoryIndex = 0; categoryIndex < categories.size(); ++categoryIndex) {
        QString categoryName = categories.at(categoryIndex);
        categories[categoryIndex] = categoryName.simplified();
    }

    QJsonArray categoriesList = QJsonArray::fromStringList(categories);

    videoJSON["categories"] = categoriesList;

    QJsonDocument saveDoc(videoJSON);
    saveFile.write(saveDoc.toJson());

    return true;
}

void MainWindow::readyReadStandardOutput()
{
    qDebug() << myProcess->readAllStandardOutput();
}
