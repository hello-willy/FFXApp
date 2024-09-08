#pragma once
#include "FFXCore.h"
#include "FFXFileFilter.h"
#include "FFXAppConfig.h"

#include <QListView>
#include <QUndoCommand>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>
#include <QLineEdit>

class QShortcut;
class QToolButton;
class QHBoxLayout;
class QVBoxLayout;
class QActionGroup;

namespace FFX {
	class FileQuickView;
	class ClipboardPanel;

	enum OrderBy {
		OBName = 0,
		OBDate,
		OBSize,
		OBType,
	};

	class DefaultSortProxyModel : public QSortFilterProxyModel {
		Q_OBJECT
	public:
		DefaultSortProxyModel(QObject* parent = nullptr);
		
	public:
		OrderBy GetOrderBy() const { return mOrderBy; }
		Qt::SortOrder GetSortOrder() const { return mSortOrder; }

	public:
		void SetFilterExpr(const QString& filter);
		virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

	public:
		void Refresh();
		QString FilterExp() { return mFilterExp; }
		bool IsFilterSet() const;

	protected:
		virtual bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
		virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

	private:
		OrderBy mOrderBy = OBName;
		Qt::SortOrder mSortOrder = Qt::AscendingOrder;
		QString mFilterExp;
		FileFilterPtr mFileFilter;
	};

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
		explicit DefaultFileListViewEditDelegate(QSortFilterProxyModel* fileModel, QObject* parent = nullptr);
	public:
		virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
	protected:
		//! for remove the dash line when focus.
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	private:
		// for get the file info about QModelIndex.
		QSortFilterProxyModel* mViewModel;
		int mMargin = 5;
		int mItemHeight = 70;

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
		void SetSortBy(OrderBy ob, Qt::SortOrder = Qt::AscendingOrder);
		void SetFilter(const QString& filter);
		OrderBy GetOrderBy() const { return mSortProxyModel->GetOrderBy(); }
		Qt::SortOrder GetSortOrder() const { return mSortProxyModel->GetSortOrder(); }

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
		DefaultSortProxyModel* mSortProxyModel;
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

	class PathEditWidget : public QLineEdit {
		Q_OBJECT
	public:
		PathEditWidget(QWidget* parent = nullptr);

	protected:
		void focusInEvent(QFocusEvent* event) override;
	};

	class DefaultFileListViewNavigator : public QWidget {
		Q_OBJECT
	public:
		DefaultFileListViewNavigator(QWidget* parent = nullptr);
		friend class ChangeRootPathCommand;

	public:
		void Goto(const QString& path);
		void AddWidget(QWidget* widget);

	Q_SIGNALS:
		void RootPathChanged(const QString& newPath);

	private slots:
		void OnBackward();
		void OnForward();
		void OnUpward();
		void OnGoto();

	private:
		void ChangePath(const QString& path);
		void SetupUi();

	private:
		QUndoStack* mRootPathChangeStack;
		QToolButton* mBackwardButton;
		QToolButton* mForwardButton;
		QToolButton* mUpwardButton;
		PathEditWidget* mRootPathEdit;
		QHBoxLayout* mMainLayout;
		QString mCurrentPath;
		QShortcut* mBackwardShortcut;
		QShortcut* mGotoShortcut;
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

	class FFXCORE_EXPORT FileMainView : public QWidget, public Configurable {
		Q_OBJECT
	public:
		FileMainView(QWidget* parent = nullptr);

	public:
		virtual void Save(AppConfig* config) override;
		virtual void Restore(AppConfig* config) override;

	public:
		void Goto(const QString& path);
		QStringList SelectedFiles();
		QString RootPath();
		FileQuickView* FileQuickViewPtr() { return mFileQuickView; }

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
		QAction* ClearFolderAction() { return mClearFolderAction; }
		QAction* RenameAction() { return mRenameAction; }
		QAction* PropertyAction() { return mPropertyAction; }
		QAction* CopyFilePathAction() { return mCopyFilePathAction; }
		QAction* OpenCommandPromptAction() { return mOpenCommandPromptAction; }

	Q_SIGNALS:
		void CurrentPathChanged(const QString& newPath);
		void SelectionChanged(QStringList files);

	private slots:
		void OnFileDoubleClicked(const QFileInfo& file);
		void OnRootPathChanged(const QFileInfo& file);
		void OnFixedToQuickPanel();
		void OnEnvelopeFiles();
		void OnClearFolder();
		void OnRename();
		void OnFileProperty();
		void OnCopyFilePath();
		void OnOpenCommandPrompt();
		void OnSetOrderBy();
		void OnFileFilterChanged();
		void OnFocusFileFilterSet();
		void OnFileFilterEditTextChanged();
		void OnClearFileFilter();

	private:
		void SetupUi();

	private:
		DefaultFileListViewNavigator* mFileViewNavigator;
		DefaultFileListView* mFileListView;
		FileQuickView* mFileQuickView;
		ClipboardPanel* mClipboardPanel;
		QVBoxLayout* mMainLayout;
		//! Widgets
		QLineEdit* mFilterEdit;
		QAction* mClearFilterAction;
		QToolButton* mRefreshFileListButton;
		QToolButton* mSetFileListOrderButton;
		//QList<QAction*> mOrderByActions;
		QActionGroup* mOrderByActionGroup;
		QActionGroup* mSortActionGroup;

		//! Actions
		QAction* mMakeDirAction;
		QList<QAction*> mMakeFileActions;

		QAction* mMakeFileAction;
		QAction* mPasteFilesAction;
		QAction* mMoveFilesAction;
		QAction* mFixedToQuickPanelAction;
		QAction* mRefreshAction;
		
		QAction* mEnvelopeFilesAction;
		QAction* mClearFolderAction;
		QAction* mRenameAction;
		QAction* mPropertyAction;
		QAction* mCopyFilePathAction;
		QAction* mOpenCommandPromptAction;

		QShortcut* mSetFileFilterShortcut;
		//QShortcut* mRefreshShortcut;
	};
}

