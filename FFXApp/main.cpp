#include "FFXApp.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include "FFXFileHandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFX::FileHandlerPtr dh = std::make_shared<FFX::DuplicateHandler>();
    FFX::FileHandlerPtr rh = std::make_shared<FFX::RegExpReplaceHandler>("(\\d+)", "XoX", QRegExp::RegExp);
    dh->SetArg("Base", 8);
    QFileInfo fi("E:/Temp/data/Hello.xml");
    bool f = fi.isFile();
    bool dir = fi.isDir();
    FFX::FileList toMatch;
    for (int i = 0; i < 20; i++)
        toMatch << FFX::FileInfo("E:/Temp/Hello");
    toMatch << FFX::FileInfo("E:/Temp/data/Hello.xml");
    toMatch << FFX::FileInfo("E:/Temp/data/Hello.xml");

    FFX::FileHandlerPtr pipe = std::make_shared<FFX::PipelineFileHandler>(dh, rh);
    FFX::FileList result = pipe->Handle(toMatch);
    for (const FFX::FileInfo& f : result)
        qDebug() << f.Path();

    FFXApp w;
    w.show();
    return a.exec();
}
