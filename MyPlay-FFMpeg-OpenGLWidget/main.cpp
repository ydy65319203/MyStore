#include "mainwindow.h"
#include "mylog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    LOG(Info, "main()... \n");
    QApplication a(argc, argv);

    LOG(Info, "main()---> CMyLog::instance()->setLogLevel(Info); \n");
    CMyLog::instance()->setLogLevel(Info);

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
    else
    {
        LOG(Warn, "main()---> qssFile.open(QFile::ReadOnly) = false; \n");
    }

    LOG(Info, "main()---> MainWindow.show; \n");
    MainWindow w;
    w.show();
    return a.exec();
}
