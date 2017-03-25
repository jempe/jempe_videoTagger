#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmediaplayer.h>

#include <QMainWindow>
#include <QProcess>
#include <QToolButton>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void onPlayButtonClicked();

private:
    Ui::MainWindow *ui;
    QMediaPlayer *player;
    QString folderPath;
    QString fileName;
    QProcess *myProcess;
    QVector<qint64> scenes;
    QToolButton *sceneButtons[];

private slots:
    void positionChanged(qint64 progress);
    void durationChanged(qint64 progress);
    void stateChanged(QMediaPlayer::State);
    void setPosition(int position);
    void setPausedPosition(int position);
    void openFile();
    void setUrl(const QUrl &url);
    void saveScreenshot();
    void readyReadStandardOutput();
    void addScene();
    bool loadJSON();
    bool saveJSON();
};

#endif // MAINWINDOW_H
