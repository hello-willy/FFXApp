#include <QtWidgets/QApplication>
#include <QDebug>
#include "FFXFileHandler.h"
#include "FFXMainWindow.h"
#include "FFXAppConfig.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile file(":/FFXApp/res/style/default.qss");
    file.open(QFile::ReadOnly);
    a.setStyleSheet(file.readAll());
    file.close();
    // 
    //QDir root("D:\\Temp");
    //bool flag = QFile::rename("D:\\Temp\\新文件夹\\1.txt", "E:\\Temp\\新文件夹\\2.txt");
    //QFileInfoList toMatch = root.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
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
    //FFX::FileHandlerPtr del = std::make_shared<FFX::FileDeleteHandler>();
    //del->Handle(toMatch);
    FFX::MainWindow w;
    w.setWindowTitle(QStringLiteral("Final File X-V1.0 (轮子哥出品)"));
    w.setWindowIcon(QIcon(":/FFXApp/FFXApp.ico"));
    w.Restore(w.AppConfigPtr());
    w.show();
    return a.exec();
}
