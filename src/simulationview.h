#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include <QTemporaryFile>
#include <QWidget>

#include <QMediaPlayer>
#include <QVideoWidget>

#include "config.h"
#include "tone.h"
#include "ubx.h"

namespace Ui {
class SimulationView;
}

class DataPoint;
class MainWindow;

class SimulationView : public QWidget
{
    Q_OBJECT

public:
    explicit SimulationView(QWidget *parent = 0);
    ~SimulationView();

    virtual QSize sizeHint() const;

    void setMainWindow(MainWindow *mainWindow);

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private:
    Ui::SimulationView *ui;
    MainWindow         *mMainWindow;

    QMediaPlayer       *mPlayer;
    QVideoWidget       *mVideoWidget;

    bool               mBusy;

    QTemporaryFile     *mAudioFile;

    void setMedia(const QString &fileName);

public slots:
    void play();
    void updateView();
    void pauseMedia();

private slots:
    void on_rootBrowseButton_clicked();
    void on_selectedBrowseButton_clicked();
    void on_audioBrowseButton_clicked();

    void on_selectedCheckBox_stateChanged(int state);
    void on_audioCheckBox_stateChanged(int state);

    void on_processButton_clicked();

    void stateChanged(QMediaPlayer::State newState);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void setPosition(int position);
    void setScrubPosition(int position);
};

#endif // SIMULATIONVIEW_H
