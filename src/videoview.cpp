/***************************************************************************
**                                                                        **
**  FlySight Viewer                                                       **
**  Copyright 2018 Michael Cooper                                         **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>. **
**                                                                        **
****************************************************************************
**  Contact: Michael Cooper                                               **
**  Website: http://flysight.ca/                                          **
****************************************************************************/

#include "videoview.h"
#include "ui_videoview.h"

#include <QDir>
#include <QFileDialog>

#include "common.h"
#include "mainwindow.h"

#define POSITION_DIV 10

VideoView::VideoView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoView),
    mMainWindow(0),
    mZeroPosition(0),
    mBusy(false)
{
    ui->setupUi(this);

    ui->playButton->setEnabled(false);
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play()));

    ui->zeroButton->setEnabled(false);
    connect(ui->zeroButton, SIGNAL(clicked()), this, SLOT(zero()));

    ui->positionSlider->setEnabled(false);
    ui->positionSlider->setRange(0, 0);
    ui->positionSlider->setSingleStep(200 / POSITION_DIV);
    ui->positionSlider->setPageStep(2000 / POSITION_DIV);
    connect(ui->positionSlider, SIGNAL(valueChanged(int)), this, SLOT(setPosition(int)));

    ui->scrubDial->setEnabled(false);
    ui->scrubDial->setRange(0, 1000);
    ui->scrubDial->setSingleStep(30);
    ui->scrubDial->setPageStep(300);
    connect(ui->scrubDial, SIGNAL(valueChanged(int)), this, SLOT(setScrubPosition(int)));

    mPlayer = new QMediaPlayer(this);
    mVideoWidget = new QVideoWidget(this);
    mPlayer->setVideoOutput(ui->videoWidget);

    connect(mPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(stateChanged(QMediaPlayer::State)));
    connect(mPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
    connect(mPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(durationChanged(qint64)));
}

VideoView::~VideoView()
{
    delete mVideoWidget;
    delete mPlayer;
    delete ui;
}

QSize VideoView::sizeHint() const
{
    // Keeps windows from being initialized as very short
    return QSize(400, 300);
}

void VideoView::setMedia(const QString &fileName)
{
    // Set media
    mPlayer->setMedia(QUrl::fromLocalFile(fileName));

    // Update buttons
    ui->playButton->setEnabled(true);
    ui->zeroButton->setEnabled(true);
    ui->positionSlider->setEnabled(true);
    ui->scrubDial->setEnabled(true);
}

void VideoView::showEvent(
        QShowEvent *event)
{
    mMainWindow->mediaCursorAddRef(this);
}

void VideoView::hideEvent(
        QHideEvent *event)
{
    mBusy = true;

    mMainWindow->mediaCursorRemoveRef(this);

    mBusy = false;
}

void VideoView::play()
{
    switch(mPlayer->state())
    {
    case QMediaPlayer::PlayingState:
        mPlayer->pause();
        break;
    default:
        mMainWindow->pauseMedia();
        mPlayer->play();
        break;
    }
}

void VideoView::stateChanged(QMediaPlayer::State newState)
{
    switch(newState)
    {
    case QMediaPlayer::PlayingState:
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        break;
    default:
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    }
}

void VideoView::positionChanged(qint64 position)
{
    mBusy = true;

    // Update controls
    ui->positionSlider->setValue(position / POSITION_DIV);
    ui->scrubDial->setValue(position % 1000);

    // Update text label
    double time = (double) (position - mZeroPosition) / 1000;
    ui->timeLabel->setText(QString("%1 s").arg(time, 0, 'f', 3));

    // Update other views
    mMainWindow->setMediaCursor(time);

    mBusy = false;
}

void VideoView::durationChanged(qint64 duration)
{
    ui->positionSlider->setRange(0, duration / POSITION_DIV);
}

void VideoView::setPosition(int position)
{
    if (!mBusy)
    {
        // Update video position
        mPlayer->setPosition(position * POSITION_DIV);
        positionChanged(position * POSITION_DIV);
    }
}

void VideoView::setScrubPosition(int position)
{
    if (!mBusy)
    {
        int oldPosition = mPlayer->position();
        int newPosition = oldPosition - oldPosition % 1000 + position;

        while (newPosition <= oldPosition - 500) newPosition += 1000;
        while (newPosition >  oldPosition + 500) newPosition -= 1000;

        // Update video position
        mPlayer->setPosition(newPosition);
        positionChanged(newPosition);
    }
}

void VideoView::zero()
{
    mZeroPosition = mPlayer->position();

    // Update text label
    double time = (double) (mZeroPosition - mZeroPosition) / 1000;
    ui->timeLabel->setText(QString("%1 s").arg(time, 0, 'f', 3));
}

void VideoView::updateView()
{
    if (mBusy) return;
    if (mMainWindow->dataSize() == 0) return;

    // Get media cursor
    const DataPoint &dp = mMainWindow->interpolateDataT(mMainWindow->mediaCursor());

    // Get playback position
    int position = dp.t * 1000 + mZeroPosition;

    // If playback position is within video bounds
    if (0 <= position && position <= mPlayer->duration())
    {
        // Update video position
        mPlayer->setPosition(position);
        positionChanged(position);
    }
}

void VideoView::pauseMedia()
{
    mPlayer->pause();
}
