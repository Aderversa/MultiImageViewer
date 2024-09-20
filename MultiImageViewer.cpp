#include "MultiImageViewer.h"
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>
#include <QApplication>

MultiImageViewer::MultiImageViewer(const QVector<QImage>& list, QWidget *parent)
    : QAbstractScrollArea(parent)
    , m_document(list)
    , m_documentMargins(6, 6, 6, 6)
    , m_pageSpacing(6)
{
    QImage img;
    if (m_document.size() > 0) {
        img = m_document[0].scaled(QSize(1024, 512));
        m_zoomFactor = qreal(img.size().width()) / m_document[0].size().width();
        m_minZoomFactor = m_zoomFactor;
    }
    verticalScrollBar()->setSingleStep(20);
    horizontalScrollBar()->setSingleStep(20);
    setMouseTracking(true);
    calculateViewport();
    updateDocumentLayout();
    if (!img.isNull()) {
        QSize size = m_documentLayout.documentSize;
        size.setHeight(img.height());
        resize(size);
    }
}

MultiImageViewer::~MultiImageViewer() {}

void MultiImageViewer::updateScrollBars()
{
    const QSize p = viewport()->size();
    const QSize v = m_documentLayout.documentSize;
    horizontalScrollBar()->setRange(0, v.width() - p.width());
    horizontalScrollBar()->setPageStep(p.width());
    verticalScrollBar()->setRange(0, v.height() - p.height());
    verticalScrollBar()->setPageStep(p.height());
}

MultiImageViewer::DocumentLayout MultiImageViewer::calculateDocumentLayout() const
{
    DocumentLayout documentLayout;

    QHash<int, QPair<QRect, qreal>> pageGeometryAndScale;

    const int pageCount = m_document.size();
    int totalWidth = 0;
    const int startPage = 0;
    const int endPage = pageCount;
    // 只是计算页面本身的大小
    for (int page = startPage; page < endPage; ++page) {
        QSize pageSize;
        qreal pageScale = m_zoomFactor;
        // 计算出缩放后的大小
        pageSize = m_document[page].size() * m_zoomFactor;
        // 考虑到不同页面的宽度可能不一，这里取大的页面的宽度
        totalWidth = qMax(totalWidth, pageSize.width());
        pageGeometryAndScale[page] = {QRect(QPoint(0,0), pageSize), pageScale};
    }
    // margins也会占用一定的viewport宽度
    totalWidth += m_documentMargins.left() + m_documentMargins.right();
    // 第一个页面的top的起始y坐标
    int pageY = m_documentMargins.left();
    // 根据页面的大小，为每个页面定义一个自定义坐标系中的位置
    for (int page = startPage; page < endPage; ++page) {
        const QSize pageSize = pageGeometryAndScale[page].first.size();
        // 如果viewpor.width > totalWidth，说明margins.left不能让页面刚好居中，应该取(viewport.width() - pageSize.width() )/ 2
        // 这样相当于调整了margins.left，让页面居中
        // 这样看来，在viewport太大的时候，totalWidth只是作为一层参考？
        const int pageX = (qMax(totalWidth, m_viewport.width()) - pageSize.width()) / 2;
        pageGeometryAndScale[page].first.moveTopLeft(QPoint(pageX, pageY));
        pageY += pageSize.height() + m_pageSpacing;
        // m_pageSpacing是垂直层面上，页面之间的距离。
    }
    pageY += m_documentMargins.bottom();

    documentLayout.pageGeometryAndScale = pageGeometryAndScale;
    documentLayout.documentSize = QSize(totalWidth, pageY);

    return documentLayout;
}

void MultiImageViewer::updateDocumentLayout()
{
    m_documentLayout = calculateDocumentLayout();
    updateScrollBars();
}

void MultiImageViewer::calculateViewport()
{
    const int x = horizontalScrollBar()->value();
    const int y = verticalScrollBar()->value();
    const int width = viewport()->width();
    const int height = viewport()->height();

    setViewport(QRect(x, y, width, height));
}

void MultiImageViewer::setViewport(QRect viewport)
{
    if (m_viewport == viewport)
        return;
    const QSize oldSize = m_viewport.size();

    m_viewport = viewport;
    if (oldSize != m_viewport.size()) { // viewport大小发生变化，调整布局
        updateDocumentLayout();
    }
}

void MultiImageViewer::paintEvent(QPaintEvent * event)
{
    QPainter painter(viewport());
    // 将背景涂黑，区分页面和背景
    painter.fillRect(event->rect(), palette().brush(QPalette::Dark));
    // 定位viewport
    painter.translate(-m_viewport.x(), -m_viewport.y());

    for (auto it = m_documentLayout.pageGeometryAndScale.cbegin();
         it != m_documentLayout.pageGeometryAndScale.cend(); ++it) {
        const QRect pageGeometry = it.value().first;
        const qreal scale = it.value().second;
        if (pageGeometry.intersects(m_viewport)) {
            painter.fillRect(pageGeometry, Qt::white);
            const int page = it.key();
            QImage img = m_document[page].scaled(m_document[page].size() * scale);
            painter.drawImage(pageGeometry, img);
        }
    }
}

void MultiImageViewer::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBars();
    calculateViewport();
}

void MultiImageViewer::scrollContentsBy(int dx, int dy)
{
    QAbstractScrollArea::scrollContentsBy(dx, dy);
    calculateViewport();
}

void MultiImageViewer::wheelEvent(QWheelEvent * event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        QPoint mousePos = event->position().toPoint() + m_viewport.topLeft();
        mousePos = mousePos / m_zoomFactor;
        QPoint diff = event->position().toPoint();
        if (event->angleDelta().y() > 0) {
            m_zoomFactor = qMin(3.0, m_zoomFactor + 0.05);
        }
        else {
            m_zoomFactor = qMax(m_minZoomFactor, m_zoomFactor - 0.05);
        }
        mousePos = mousePos * m_zoomFactor;
        QPoint topLeft = mousePos - diff;
        updateDocumentLayout();
        horizontalScrollBar()->setValue(topLeft.x());
        verticalScrollBar()->setValue(topLeft.y());
        calculateViewport();
    }
    else {
        QAbstractScrollArea::wheelEvent(event);
        updateDocumentLayout();
    }
    update();
}

void MultiImageViewer::changeCursor(Qt::CursorShape shape)
{
    QCursor cursor;
    cursor.setShape(shape);
    QApplication::setOverrideCursor(cursor);
}

void MultiImageViewer::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        if (horizontalScrollBar()->isVisible() || verticalScrollBar()->isVisible()) {
            changeCursor(Qt::ClosedHandCursor);
        }
        lastPos = event->pos();
        event->accept();
    }
}

void MultiImageViewer::mouseMoveEvent(QMouseEvent * event)
{
    Q_ASSERT(event->isAccepted());
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
        QPoint diff = event->pos() - lastPos;
        lastPos = event->pos();
        diff = -diff;
        verticalScrollBar()->setValue(qMax(verticalScrollBar()->minimum(),
                                           qMin(verticalScrollBar()->maximum(),
                                                diff.y() + verticalScrollBar()->value())));
        horizontalScrollBar()->setValue(qMax(horizontalScrollBar()->minimum(),
                                           qMin(horizontalScrollBar()->maximum(),
                                                diff.x() + horizontalScrollBar()->value())));
        calculateViewport();
        update();
    }
}

void MultiImageViewer::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        if (horizontalScrollBar()->isVisible() || verticalScrollBar()->isVisible()) {
            changeCursor(Qt::ArrowCursor);
        }
    }
}











