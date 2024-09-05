#pragma once
#include "FFXCore.h"
#include "FFXFileHandler.h"

#include <QString>
#include <QtPlugin>

class QPluginLoader;
class QAction;
namespace FFX {
	class Application;
	class FFXCORE_EXPORT Plugin {
	public:
		virtual ~Plugin() {}
	public:
		virtual void Install() = 0;
		virtual void Uninstall() = 0;
		virtual QString Id() = 0;
		virtual QString Name() { return "Unknown"; }
		virtual QString Description() { return ""; }

	public:
		Application* App();
	};

	class FFXCORE_EXPORT PluginManager : public QObject {
		Q_OBJECT
	public:
		PluginManager(QObject* parent = nullptr);
		~PluginManager() = default;

	public:
		static QString PluginPath();

	public:
		void AutoLoad();
		void LoadPlugin(const QString& pluginPath);
		void UnloadPlugin(const QString& pluginId);
		inline QAction* InstallPluginAction() { return mInstallPluginAction; }

	private slots:
		void OnLoadPlugin();
		void OnPluginUnzipped(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);

	private:
		QMap<QString, QPluginLoader*> mPluginMap;
		QAction* mInstallPluginAction;
		int mInstallTask = -1;
	};

	class PluginInstallHandler : public FileHandler {
	public:
		PluginInstallHandler();

	public:
		virtual QFileInfoList Filter(const QFileInfoList& files) override;
		virtual QFileInfoList Handle(const QFileInfoList& files, ProgressPtr progress = G_DebugProgress) override;
		virtual std::shared_ptr<FileHandler> Clone() override;
		virtual QString Name() { return QStringLiteral("UnzipHandler"); }
		virtual QString DisplayName() { return QObject::tr("UnzipHandler"); }
		virtual QString Description() { return QObject::tr("Unzip file."); }
		virtual void Cancel() { mCancelled = true; }

	private:
		QString MakeOutputDir(const QFileInfo& zipFile);
		void UnzipFile(const QFileInfo& zipFile, const QString& outputDir, ProgressPtr progress);

	protected:
		bool mCancelled = false;
		FileFilterPtr mFileFilter;
	};
}

#define FFX_Plugin_IID "FFX.Plugin.Interface"

Q_DECLARE_INTERFACE(FFX::Plugin, FFX_Plugin_IID)


