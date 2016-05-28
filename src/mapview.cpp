#include "mapview.h"

#include <QVector>
#include <QWebFrame>
#include <QWebElement>

#include "common.h"
#include "mainwindow.h"

MapView::MapView(QWidget *parent) :
    QWebView(parent),
    mMainWindow(0)
{
    setUrl(QUrl("qrc:/html/mapview.html"));
}

QSize MapView::sizeHint() const
{
    // Keeps windows from being intialized as very short
    return QSize(175, 175);
}

void MapView::mouseMoveEvent(
        QMouseEvent *event)
{
    // Get map view bounds
    QString js = QString("var bounds = map.getBounds();") +
                 QString("var ne = bounds.getNorthEast();") +
                 QString("var sw = bounds.getSouthWest();");

    page()->currentFrame()->documentElement().evaluateJavaScript(js);

    const double latMin = page()->currentFrame()->documentElement().evaluateJavaScript("sw.lat();").toDouble();
    const double latMax = page()->currentFrame()->documentElement().evaluateJavaScript("ne.lat();").toDouble();
    const double lonMin = page()->currentFrame()->documentElement().evaluateJavaScript("sw.lng();").toDouble();
    const double lonMax = page()->currentFrame()->documentElement().evaluateJavaScript("ne.lng();").toDouble();

    double lower = mMainWindow->rangeLower();
    double upper = mMainWindow->rangeUpper();

    double resultTime;
    double resultDistance = std::numeric_limits<double>::max();

    for (int i = 0; i + 1 < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp1 = mMainWindow->dataPoint(i);
        const DataPoint &dp2 = mMainWindow->dataPoint(i + 1);

        if (lower <= dp1.t && dp1.t <= upper &&
            lower <= dp2.t && dp2.t <= upper)
        {
            QPointF pt1 = QPointF(width() * (dp1.lon - lonMin) / (lonMax - lonMin),
                                  height() * (latMax - dp1.lat) / (latMax - latMin));
            QPointF pt2 = QPointF(width() * (dp2.lon - lonMin) / (lonMax - lonMin),
                                  height() * (latMax - dp2.lat) / (latMax - latMin));

            double mu;
            double dist = sqrt(distSqrToLine(pt1, pt2, event->pos(), mu));

            if (dist < resultDistance)
            {
                double t1 = dp1.t;
                double t2 = dp2.t;

                resultTime = t1 + mu * (t2 - t1);
                resultDistance = dist;
            }
        }
    }

    const int selectionTolerance = 8;
    if (resultDistance < selectionTolerance)
    {
        mMainWindow->setMark(resultTime);
    }
    else
    {
        mMainWindow->clearMark();
    }

    // Call base class
    QWebView::mouseMoveEvent(event);
}

void MapView::initView()
{
    double xMin, xMax;
    double yMin, yMax;

    for (int i = 0; i < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp = mMainWindow->dataPoint(i);

        if (i == 0)
        {
            xMin = xMax = dp.lon;
            yMin = yMax = dp.lat;
        }
        else
        {
            if (dp.lon < xMin) xMin = dp.lon;
            if (dp.lon > xMax) xMax = dp.lon;

            if (dp.lat < yMin) yMin = dp.lat;
            if (dp.lat > yMax) yMax = dp.lat;
        }
    }

    // Resize map
    QString js = QString("var bounds = new google.maps.LatLngBounds();") +
                 QString("bounds.extend(new google.maps.LatLng(%1, %2));").arg(yMin).arg(xMin) +
                 QString("bounds.extend(new google.maps.LatLng(%1, %2));").arg(yMax).arg(xMax) +
                 QString("map.fitBounds(bounds);");

    page()->currentFrame()->documentElement().evaluateJavaScript(js);
}

void MapView::updateView()
{
    double lower = mMainWindow->rangeLower();
    double upper = mMainWindow->rangeUpper();

    double xMin, xMax;
    double yMin, yMax;

    bool first = true;

    // Distance threshold
    const double earthCircumference = 40075000; // m
    const double zoom = page()->currentFrame()->documentElement().evaluateJavaScript("map.getZoom();").toDouble();
    const double threshold = earthCircumference / pow(2, zoom) / width();

    // Add track to map
    QString js = QString("var path = poly.getPath();") +
                 QString("while (path.length > 0) { path.pop(); }");

    double distPrev;
    for (int i = 0; i < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp = mMainWindow->dataPoint(i);

        if (i > 0 && dp.dist2D - distPrev < threshold) continue;
        distPrev = dp.dist2D;

        if (lower <= dp.t && dp.t <= upper)
        {
            if (first)
            {
                xMin = xMax = dp.lon;
                yMin = yMax = dp.lat;
                first = false;
            }
            else
            {
                if (dp.lon < xMin) xMin = dp.lon;
                if (dp.lon > xMax) xMax = dp.lon;

                if (dp.lat < yMin) yMin = dp.lat;
                if (dp.lat > yMax) yMax = dp.lat;
            }

            js += QString("path.push(new google.maps.LatLng(%1, %2));").arg(dp.lat, 0, 'f').arg(dp.lon, 0, 'f');
        }
    }

    page()->currentFrame()->documentElement().evaluateJavaScript(js);

    if (mMainWindow->markActive())
    {
        // Add marker to map
        const DataPoint &dpEnd = mMainWindow->interpolateDataT(mMainWindow->markEnd());

        js = QString("marker.setPosition(new google.maps.LatLng(%1, %2));").arg(dpEnd.lat, 0, 'f').arg(dpEnd.lon, 0, 'f') +
             QString("marker.setVisible(true);");

        page()->currentFrame()->documentElement().evaluateJavaScript(js);
    }
    else
    {
        // Clear marker
        js = QString("marker.setVisible(false);");

        page()->currentFrame()->documentElement().evaluateJavaScript(js);
    }

    // Draw annotations on map
    mMainWindow->prepareMapView(this);
}
