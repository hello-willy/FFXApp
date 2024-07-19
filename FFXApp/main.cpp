#include "FFXApp.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include "FFXFileHandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir root("D:\\Temp\\GeosDemo");
    QFileInfoList toMatch = root.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    /*
    FFX::FileHandlerPtr handler = std::make_shared<FFX::FileRenameHandler>(std::make_shared<FFX::FileNameReplaceByExpHandler>("(\\d+)", "", QRegExp::RegExp));
    std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileNameReplaceByExpHandler>("_", ""));
    std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::CaseTransformHandler>(true, false));
    std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>());

    FFX::FileHandlerPtr h2 = handler->Clone();
    QFileInfoList result = h2->Handle(toMatch, std::make_shared<FFX::DebugProgress>());
    for (const QFileInfo& f : result)
        qDebug() << f.filePath();
    */
    //FFX::FileFilterPtr filter = std::make_shared<FFX::RegExpFileFilter>("*.dll");
    //FFX::FileHandlerPtr finder = std::make_shared<FFX::FileSearchHandler>(filter);
    //QFileInfoList r = finder->Handle(toMatch);
   // for (const QFileInfo& f : r)
    //    qDebug() << f.filePath();
    FFX::FileHandlerPtr copy = std::make_shared<FFX::FileCopyHandler>(QStringLiteral("E:\\tmp\\新建文件夹"), true);
    copy->Handle(toMatch);
    FFXApp w;
    w.show();
    return a.exec();
}
