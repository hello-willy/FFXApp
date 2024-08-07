#pragma once
#include "FFXCore.h"

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
	class FileQuickView;

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
		
	public:
		void SetRootPath(const QFileInfo& root);

	public slots:
		void MakeDirAndEdit();
		void MakeFileAndEdit(const QString& name);
		
	protected:
		void keyPressEvent(QKeyEvent* event) override;
		void closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) override;

	private:
		void CopyFiles(bool overwrite = false);
		void MoveFiles(bool overwrite = false);

	private slots:
		void OnItemDoubleClicked(const QModelIndex& index);
		void OnItemRenamed(const QString& path, const QString& oldName, const QString& newName);
		virtual void OnCustomContextMenuRequested(const QPoint& pos);
		void OnActionDelete();
		void OnActionMoveToTrash();
		void OnInvertSelect();
		void OnCollectFiles();
		void OnAppendCollectFiles();
		virtual void Refresh();

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
		QShortcut* mOverwritePasteFilesShortcut;
		QShortcut* mMoveFilesShortcut;
		QShortcut* mOverwriteMoveFilesShortcut;
		QShortcut* mCollectFilesShortcut;
		QShortcut* mAppendCollectFilesShortcut;
		//! Actions
		
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

	class FFXCORE_EXPORT FileMainView : public QWidget {
		Q_OBJECT
	public:
		FileMainView(QWidget* parent = nullptr);

	public:
		void Goto(const QString& path);
		QStringList SelectedFiles();
		QString RootPath();
	public slots:
		void RefreshFileListView();

	public:
		QAction* MakeDirAction();
		void AddMakeFileAction(QAction* action);
		QList<QAction*> MakeFileActions();
		QAction* PasteFilesAction();
		QAction* MoveFilesAction();
		QAction* FixedToQuickPanelAction();
		QAction* RefreshAction() { return mRefreshAction; }
		QAction* EnvelopeFilesAction() { return mEnvelopeFilesAction; }

	Q_SIGNALS:
		void CurrentPathChanged(const QString& newPath);
		void SelectionChanged(QStringList files);

	private slots:
		void OnFileDoubleClicked(const QFileInfo& file);
		void OnRootPathChanged(const QFileInfo& file);
		void OnFixedToQuickPanel();
		void OnEnvelopeFiles();

	private:
		void SetupUi();

	private:
		DefaultFileListViewNavigator* mFileViewNavigator;
		DefaultFileListView* mFileListView;
		FileQuickView* mFileQuickView;
		QVBoxLayout* mMainLayout;
		//! Actions
		QAction* mMakeDirAction;
		QList<QAction*> mMakeFileActions;

		QAction* mMakeFileAction;
		QAction* mPasteFilesAction;
		QAction* mMoveFilesAction;
		QAction* mFixedToQuickPanelAction;
		QAction* mRefreshAction;
		QAction* mEnvelopeFilesAction;
	};
}

