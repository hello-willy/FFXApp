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

	class DefaultFileListView;
	class ChangeRootPathCommand : public QUndoCommand
	{
	public:
		ChangeRootPathCommand(DefaultFileListView* fileView, const QFileInfo& newPath, const QFileInfo& oldPath, QUndoCommand* parent = nullptr);
		virtual ~ChangeRootPathCommand() = default;
	public:
		void undo() override;
		void redo() override;

	private:
		DefaultFileListView* mFileView;
		QFileInfo mNewPath;
		QFileInfo mOldPath;
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
		explicit DefaultFileListViewEditDelegate(QFileSystemModel* fileModel, QObject* parent = nullptr);
	public:
		virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
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

	public:
		virtual QStringList SelectedFiles();
		virtual QString CurrentDir();
		virtual QModelIndex IndexOf(const QString& file);
		virtual void Refresh();

	public:
		void SetRootPath(const QFileInfo& root);
		void Forward();
		void Backward();
		void Upward();

	private:
		void ChangeRoot(const QFileInfo& root);

	private slots:
		void OnItemDoubleClicked(const QModelIndex& index);
		void OnItemRenamed(const QString& path, const QString& oldName, const QString& newName);
		void OnCustomContextMenuRequested(const QPoint& pos);
		void OnActionDelete();
		void OnActionMoveToTrash();

	private:
		DefaultFileListViewModel* mFileModel;
		QUndoStack* mRootPathChangeStack;
		bool mEditing = false;
		//! Shortcut
		QShortcut* mBackwardShortcut;
		QShortcut* mDeleteForceShortcut;
		QShortcut* mMoveToTrashShortcut;
	};

	class DefaultFileListViewNavigator : public QWidget {
		Q_OBJECT
	public:
		DefaultFileListViewNavigator(QWidget* parent = nullptr);

	private:
		void SetupUi();

	private:
		QToolButton* mBackwardButton;
		QToolButton* mForwardButton;
		QToolButton* mUpwardButton;
		QLineEdit* mRootPathEdit;
		QHBoxLayout* mMainLayout;
	};

	class FileMainView : public QWidget {
		Q_OBJECT
	public:
		FileMainView(QWidget* parent = nullptr);
	public:
		void Goto(const QString& path);

	private:
		void SetupUi();
	private:
		DefaultFileListViewNavigator* mFileViewNavigator;
		DefaultFileListView* mFileListView;
		QVBoxLayout* mMainLayout;
	};
}

