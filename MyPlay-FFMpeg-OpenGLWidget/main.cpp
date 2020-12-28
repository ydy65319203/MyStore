#include "mainwindow.h"
#include "mylog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    LOG(Info, "main()... \n");
    QApplication a(argc, argv);

    QFileInfo fileInfo(argv[0]);
    QString qstrFilePath = fileInfo.absolutePath() + ("/stylesheet.qss");
    LOG(Info, "main()---> qssFile = %s \n", qstrFilePath.toStdString().c_str());

    //加载QSS样式: stylesheet.qss
    QFile qssFile(qstrFilePath);
    if(qssFile.open(QFile::ReadOnly))
    {
        LOG(Info, "main()---> QApplication::setStyleSheet(qssFile); \n");
        a.setStyleSheet(qssFile.readAll());
        qssFile.close();
    }

    LOG(Info, "main()---> MainWindow; \n");
    MainWindow w;
    w.show();
    return a.exec();
}
