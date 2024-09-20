#include "MultiImageViewer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QVector<QImage> list;
    for (int i = 0; i < 10; ++i) {
        list.push_back(QImage(QString(":/1.jpg")));
    }
    MultiImageViewer w(list);
    w.show();
    return a.exec();
}
