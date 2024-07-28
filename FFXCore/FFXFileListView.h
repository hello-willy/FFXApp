#pragma once

#include <QListView>
#include <QUndoCommand>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QStyledItemDelegate>

class QShortcut;
class QToolButton;
class QLineEdit;
class QHBoxLayout;
class QVBoxLayout;

namespace FFX {
	class DefaultFileListViewModel : public QFileSystemModel
	{
		Q_OBJECT
	public:
		explicit DefaultFileListViewModel(QObject* parent = nullptr);

	public:
		// override from QFileSystemModel to take over rename operation
		bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	};

	class DefaultFileListViewEditDelegate : public QStyledItemDelegate
	{
		Q_OBJECT
	public:
		explicit DefaultFileListViewEditDelegate(QFileSystemModel* fileModel, QObject* parent = nullptr);
	public:
		virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
		//! for remove the dash line when focus.
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	private:
		// for get the file info about QModelIndex.
		QFileSystemModel* mFileModel;

	Q_SIGNALS:
		void startEditing() const;
	};

	
	class DefaultFileListView : public QListView {
		Q_OBJECT
	public:
		DefaultFileListView(QWidget* parent = nullptr);
		~DefaultFileListView();
		friend class ChangeRootPathCommand;
		friend class FileMainView;

	public:
		virtual QStringList SelectedFiles();
		virtual QString CurrentDir();
		virtual QModelIndex IndexOf(const QString& file);
		virtual void Refresh();

	public:
		void SetRootPath(const QFileInfo& root);

	public slots:
		void MakeDirAndEdit();
		void MakeFileAndEdit(const QString& name);
		
	protected:
		void keyPressEvent(QKeyEvent* event) override;
		void closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) override;

	private slots:
		void OnItemDoubleClicked(const QModelIndex& index);
		void OnItemRenamed(const QString& path, const QString& oldName, const QString& newName);
		void OnCustomContextMenuRequested(const QPoint& pos);
		void OnActionDelete();
		void OnActionMoveToTrash();
		void OnInvertSelect();
		void OnCollectFiles();
		void OnAppendCollectFiles();
		void OnCopyFiles();
		void OnMoveFiles();

	Q_SIGNALS:
		void FileDoubleClicked(const QFileInfo& file);

	private:
		DefaultFileListViewModel* mFileModel;
		
		bool mEditing = false;
		//! Shortcut
		QShortcut* mDeleteForceShortcut;
		QShortcut* mMoveToTrashShortcut;
		QShortcut* mInvertSelectShortcut;
		QShortcut* mPasteFilesShortcut;
		QShortcut* mMoveFilesShortcut;
		QShortcut* mCollectFilesShortcut;
		QShortcut* mAppendCollectFilesShortcut;
	};

	class DefaultFileListViewNavigator : public QWidget {
		Q_OBJECT
	public:
		DefaultFileListViewNavigator(QWidget* parent = nullptr);
		friend class ChangeRootPathCommand;

	public:
		void Goto(const QString& path);

	Q_SIGNALS:
		void RootPathChanged(const QString& newPath);

	private slots:
		void OnBackward();
		void OnForward();
		void OnUpward();

	private:
		void ChangePath(const QString& path);
		void SetupUi();

	private:
		QUndoStack* mRootPathChangeStack;
		QToolButton* mBackwardButton;
		QToolButton* mForwardButton;
		QToolButton* mUpwardButton;
		QLineEdit* mRootPathEdit;
		QHBoxLayout* mMainLayout;
		QString mCurrentPath;
		QShortcut* mBackwardShortcut;
	};

	class ChangeRootPathCommand : public QUndoCommand
	{
	public:
		ChangeRootPathCommand(DefaultFileListViewNavigator* navigator, const QFileInfo& newPath, const QFileInfo& oldPath, QUndoCommand* parent = nullptr);
		virtual ~ChangeRootPathCommand() = default;

	public:
		void undo() override;
		void redo() override;

	private:
		DefaultFileListViewNavigator* mFileListViewNavigator;
		QFileInfo mNewPath;
		QFileInfo mOldPath;
	};

	class FileMainView : public QWidget {
		Q_OBJECT
	public:
		FileMainView(QWidget* parent = nullptr);

	public:
		void Goto(const QString& path);
		QAction* MakeDirAction();
		void AddMakeFileAction(QAction* action);
		QList<QAction*> MakeFileActions();
		QAction* PasteFilesAction();
		QAction* MoveFilesAction();

	Q_SIGNALS:
		void CurrentPathChanged(const QString& newPath);

	private slots:
		void OnFileDoubleClicked(const QFileInfo& file);

	private:
		void SetupUi();

	private:
		DefaultFileListViewNavigator* mFileViewNavigator;
		DefaultFileListView* mFileListView;
		QVBoxLayout* mMainLayout;
		//! Actions
		QAction* mMakeDirAction;
		QList<QAction*> mMakeFileActions;

		QAction* mMakeFileActionDefault;
		QAction* mPasteFilesAction;
		QAction* mMoveFilesAction;
	};
}

