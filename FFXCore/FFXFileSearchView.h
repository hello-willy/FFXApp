#pragma once
#include "FFXFileListView.h"

#include <QWidget>
#include <QAbstractListModel>
#include <QListView>
#include <QStyledItemDelegate>

class QGridLayout;
namespace FFX {
	class SearchFileListViewModel : public QAbstractListModel {
		Q_OBJECT
	public:
		explicit SearchFileListViewModel(QObject* parent = 0);
		friend class SearchFileListView;

	public:
		int rowCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;

	public:
		void Append(const QString& file);
		void Clear();
		QModelIndex IndexOf(const QString& file);

	private:
		QStringList mListFileLoaded;
	};

	class SearchFileListViewItemDelegate : public QStyledItemDelegate {
	public:
		SearchFileListViewItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	};

	class SearchFileListView : public DefaultFileListView {
		Q_OBJECT
	public:
		SearchFileListView(QWidget* parent = nullptr);

	public:
		virtual QStringList SelectedFiles() override;
		virtual QString CurrentDir() override;
		virtual QModelIndex IndexOf(const QString& file) override;

	public:
		void AddItem(const QString& file);
		void RemoveAll();

	private slots:
		void OnOpenFileFolder();
		void OnCustomContextMenuRequested(const QPoint& pos);

	private:
		SearchFileListViewModel* mFileSearchModel;
		SearchFileListViewItemDelegate* mFileSearchItemDelegate;
		QAction* mOpenFileFolderAction;
	};

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

	private:
		void SetupUi();
		void SetWorking(bool work = true);

	private:
		QString mSearchDir;
		int mSearchTaskId = -1;
		SearchFileListView* mSearchFileListView;
		QAction* mSearchAction;
		QLineEdit* mSearchEdit;
		QToolButton* mSearchFileOnlyButton;
		QToolButton* mSearchCaseButton;
		QGridLayout* mMainLayout;
	};
}

