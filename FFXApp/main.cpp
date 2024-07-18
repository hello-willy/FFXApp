#include "FFXApp.h"
#include <QtWidgets/QApplication>
#include <QDebug>
#include "FFXFileHandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir root("E:\\tmp");
    QFileInfoList toMatch = root.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    //FFX::FileHandlerPtr dup = std::make_shared<FFX::FileDuplicateHandler>();
    //FFX::FileHandlerPtr pipe = std::make_shared<FFX::PipeFileHandler>(std::make_shared<FFX::FileNameRegExpReplace>("-", ""), std::make_shared<FFX::FileNameRegExpReplace>("-", ""));
    //FFX::FileHandlerPtr pipe2 = std::make_shared<FFX::PipeFileHandler>(pipe, dup);

    FFX::FileHandlerPtr handler = std::make_shared<FFX::FileRenameHandler>(std::make_shared<FFX::FileNameReplaceByExpHandler>("(\\d+)", "", QRegExp::RegExp));
    std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileNameReplaceByExpHandler>("_", ""));
    std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::CaseTransformHandler>(true, false));
    std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>());
    QFileInfoList result = handler->Handle(toMatch, std::make_shared<FFX::DebugProgress>());
    for (const QFileInfo& f : result)
        qDebug() << f.filePath();
    FFXApp w;
    w.show();
    return a.exec();
}
