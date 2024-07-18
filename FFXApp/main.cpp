#include "FFXApp.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include "FFXFileHandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFX::FileHandlerPtr dh = std::make_shared<FFX::FileDuplicateHandler>();
    FFX::FileHandlerPtr rh = std::make_shared<FFX::FileNameRegExpReplace>("(\\d+)", "XoX", QRegExp::RegExp);
    dh->SetArg("Base", 8);
    QFileInfo fi("E:/Temp/data/Hello.xml");
    bool f = fi.isFile();
    bool dir = fi.isDir();
    QFileInfoList toMatch;
    for (int i = 0; i < 20; i++)
        toMatch << QFileInfo("E:/Temp/Hello");
    toMatch << QFileInfo("E:/Temp/data/Hello.xml");
    toMatch << QFileInfo("E:/Temp/data/Hello.xml");

    //FFX::FileHandlerPtr pipe = std::make_shared<FFX::PipelineFileHandler>(dh, rh);
    QFileInfoList result = dh->Handle(toMatch, std::make_shared<FFX::DebugProgress>());
    for (const QFileInfo& f : result)
        qDebug() << f.filePath();

    FFXApp w;
    w.show();
    return a.exec();
}
