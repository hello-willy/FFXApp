#pragma once


#include <QWidget>
#include <QAbstractListModel>
#include <QListView>
#include <QStyledItemDelegate>
#include <QFileInfo>

class QGridLayout;
class QToolButton;
namespace FFX {
	class CommonFileListView;
	class FileSearchView : public QWidget
	{
		Q_OBJECT

	public:
		FileSearchView(QWidget* parent);
		~FileSearchView();
		friend class MainWindow;
	public:
		void SetSearchDir(const QString& dir);

	private slots:
		void OnSearch();
		void OnSearchActionTriggered();
		void OnSearchComplete(int taskId, bool success);
		void OnSearchFileMatched(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);
		void OnGotoParentDir();
		void OnActiveSearch();

	private:
		void SetupUi();
		void SetWorking(bool work = true);

	private:
		QString mSearchDir;
		int mSearchTaskId = -1;
		CommonFileListView* mSearchFileListView;
		QAction* mSearchAction;
		QLineEdit* mSearchEdit;
		QToolButton* mSearchFileOnlyButton;
		QToolButton* mSearchCaseButton;
		QGridLayout* mMainLayout;
		QAction* mGotoFileParentDirAction;
		QShortcut* mActiveSearchShortcut;
	};
}

