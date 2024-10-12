#include "FFXPlugin.h"
#include "FFXMainWindow.h"
#include "FFXMainWindow.h"
#include "FFXTaskPanel.h"

#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QAction>
#include <QFileDialog>

#include <bit7z/bitarchivereader.hpp>
#include <bit7z/bitfilecompressor.hpp>
#include <bit7z/bitfileextractor.hpp>
#include <bit7z/bitarchivewriter.hpp>
using namespace bit7z;

namespace FFX {
    Application* Plugin::App() {
        return MainWindow::Instance();
    }

    PluginManager::PluginManager(QObject* parent) 
        : QObject(parent) {
        mInstallPluginAction = new QAction(QIcon(":/ffx/res/image/plug-install.svg"), QObject::tr("Install..."));
        connect(mInstallPluginAction, &QAction::triggered, this, &PluginManager::OnLoadPlugin);

        connect(MainWindow::Instance()->TaskPanelPtr(), &TaskPanel::TaskFileHandled, this, &PluginManager::OnPluginUnzipped);
    }

    QString PluginManager::PluginPath() {
        QDir currentDir(QCoreApplication::applicationDirPath());
        currentDir.cd("ffxplugins");
        return currentDir.absolutePath();
    }

	void PluginManager::LoadPlugin(const QString& pluginPath) {
        if (!QLibrary::isLibrary(pluginPath))
            return;

        QPluginLoader* loader = new QPluginLoader(pluginPath);
        if (loader->load()) {
            Plugin* plugin = qobject_cast<Plugin*>(loader->instance());
            if (plugin) {
                plugin->Install();
                mPluginMap[plugin->Id()] = loader;
            } else {
                delete loader;
                loader = nullptr;
            }
        }
	}

	void PluginManager::UnloadPlugin(const QString& pluginId) {
        QMap<QString, QPluginLoader*>::iterator it = mPluginMap.find(pluginId);
        if (it == mPluginMap.end())
            return;
        QPluginLoader* loader = it.value();
        Plugin* p = qobject_cast<Plugin*>(loader->instance());
        if (p) {
            p->Uninstall();
        }
        loader->unload();
        delete loader;
        mPluginMap.erase(it);
	}

    void PluginManager::AutoLoad() {
        QDir currentDir(QCoreApplication::applicationDirPath());
        currentDir.cd("ffxplugins");
        currentDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        QFileInfoList fileList = currentDir.entryInfoList();
        foreach(QFileInfo fileInfo, fileList) {
            QDir pluginDir(fileInfo.absoluteFilePath());
            QString pluginPath = pluginDir.absoluteFilePath(fileInfo.fileName() + ".dll");
            LoadPlugin(pluginPath);
        }
    }

    void PluginManager::OnLoadPlugin() {
        QStringList pluginFiles = QFileDialog::getOpenFileNames(nullptr, QObject::tr("Open FFX plugins"), 
            QCoreApplication::applicationDirPath(), QString("FFX plugin (*.ffx)"));
        if (pluginFiles.isEmpty())
            return;
        mInstallTask = MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(pluginFiles), std::make_shared<PluginInstallHandler>());
    }

    void PluginManager::OnPluginUnzipped(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message) {
        if (mInstallTask != taskId)
            return;
        if (!success)
            return;
        QDir pluginDir(fileOutput.absoluteFilePath());
        QString pluginPath = pluginDir.absoluteFilePath(fileOutput.fileName() + ".dll");
        MainWindow::Instance()->PluginManagerPtr()->LoadPlugin(pluginPath);
    }

    std::shared_ptr<FileHandler> PluginInstallHandler::Clone() {
        return FileHandlerPtr(new PluginInstallHandler(*this));
    }

	bool UnzipProgressCallback(uint64_t size, const QFileInfo& file, ProgressPtr p)
	{
		uint64_t totalSize = FileSize(file);
		double process = ((1.0 * size) / totalSize);
		p->OnProgress((int)(process * 100), QObject::tr("Unzipping: %1").arg(file.absoluteFilePath()));
		return true;
	}

	PluginInstallHandler::PluginInstallHandler() {
		FileFilterPtr fileOnlyFilter = std::make_shared<OnlyFileFilter>();
		FileFilterPtr ffxFilter = std::make_shared<RegExpFileFilter>("*.ffx", QRegExp::Wildcard, false);
		mFileFilter = std::make_shared<AndFileFilter>(fileOnlyFilter, ffxFilter);
	}

	QFileInfoList PluginInstallHandler::Filter(const QFileInfoList& files) {
		QFileInfoList filesTodo;
		for (const QFileInfo& file : files) {
			if (mFileFilter->Accept(file)) {
				filesTodo << file;
			}
		}
		return filesTodo;
	}

	QFileInfoList PluginInstallHandler::Handle(const QFileInfoList& files, ProgressPtr progress) {
		QFileInfoList result;
		QFileInfoList zipFiles = Filter(files);
		int size = zipFiles.size();
		double step = 100. / size;
		for (int i = 0; i < size && !mCancelled; i++) {
			const QFileInfo& file = zipFiles[i];
			progress->OnProgress(i * step, QObject::tr("Installing: %1").arg(file.absoluteFilePath()));
			QString outputDir = MakeOutputDir(file);
			UnzipFile(file, outputDir, progress);
			result << outputDir;
		}
		progress->OnComplete(true, "Finish.");
		return result;
	}

	QString PluginInstallHandler::MakeOutputDir(const QFileInfo& zipFile) {
		QString outputDir = PluginManager::PluginPath();
		File file(zipFile);
		QString fileName = file.BaseName();
		QDir d(outputDir);
		d.mkdir(fileName);
		return d.absoluteFilePath(fileName);
	}

	void PluginInstallHandler::UnzipFile(const QFileInfo& zipFile, const QString& outputDir, ProgressPtr progress) {
		Bit7zLibrary lib{ "7z.dll" };
		try {
			BitFileExtractor extractor{ lib, BitFormat::Auto };
			extractor.test(zipFile.absoluteFilePath().toStdString());
			//! bind progress callback function: prototype is <bool calback(uint64_t size)>
			extractor.setProgressCallback(std::bind(UnzipProgressCallback, std::placeholders::_1, zipFile, progress));
			extractor.extract(zipFile.absoluteFilePath().toStdString(), outputDir.toStdString());
			progress->OnFileComplete(zipFile, outputDir);
		}
		catch (const bit7z::BitException& ex) {
			progress->OnFileComplete(zipFile, outputDir, false, ex.what());
		}
	}
}