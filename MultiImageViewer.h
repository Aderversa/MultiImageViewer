#ifndef MULTIIMAGEVIEWER_H
#define MULTIIMAGEVIEWER_H

#include <QAbstractScrollArea>
#include <QHash>
#include <QVector>

class MultiImageViewer : public QAbstractScrollArea
{
    Q_OBJECT
private:
    struct DocumentLayout
    {
        QSize documentSize;
        QHash<int, QPair<QRect, qreal>> pageGeometryAndScale;
    };
    DocumentLayout calculateDocumentLayout() const;
    void changeCursor(Qt::CursorShape);

public:
    MultiImageViewer(const QVector<QImage>& list, QWidget *parent = nullptr);
    ~MultiImageViewer();

    void updateScrollBars();
    void updateDocumentLayout();
    void setViewport(QRect viewport);
    void calculateViewport();

protected:
    void paintEvent(QPaintEvent * event);
    void resizeEvent(QResizeEvent* event);
    void scrollContentsBy(int dx, int dy);
    void wheelEvent(QWheelEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);

private:

    int m_pageSpacing;
    QMargins m_documentMargins;
    DocumentLayout m_documentLayout;
    QVector<QImage> m_document;
    qreal m_zoomFactor;
    qreal m_minZoomFactor;
    QPoint lastPos;
    QRect m_viewport;
};
#endif // MULTIIMAGEVIEWER_H
