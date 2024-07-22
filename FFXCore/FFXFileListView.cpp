#include "FFXFileListView.h"
#include <QLineEdit>
#include <QDesktopServices>
#include <QUrl>
#include <QShortcut>
#include <QToolButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "FFXFileHandler.h"

namespace FFX {
	ChangeRootPathCommand::ChangeRootPathCommand(DefaultFileListView* fileView, const QFileInfo& newPath, const QFileInfo& oldPath, QUndoCommand* parent) 
		: QUndoCommand(parent)
		, mFileView(fileView)
		, mNewPath(newPath)
		, mOldPath(oldPath)	{

	}

	void ChangeRootPathCommand::undo() {
		QString s = mOldPath.absoluteFilePath();
		mFileView->ChangeRoot(mOldPath);
	}

	void ChangeRootPathCommand::redo() {
		mFileView->ChangeRoot(mNewPath);
	}

	DefaultFileListViewModel::DefaultFileListViewModel(QObject* parent)
		: QFileSystemModel(parent)
	{}

	bool DefaultFileListViewModel::setData(const QModelIndex& idx, const QVariant& value, int role) {
		if (!idx.isValid()
			|| idx.column() != 0
			|| role != Qt::EditRole
			|| (flags(idx) & Qt::ItemIsEditable) == 0) {
			return false;
		}
		QString newName = value.toString();
		QString oldName = idx.data().toString();
		if (newName == oldName)
			return true;
		const QString parentPath = filePath(parent(idx));

		if (newName.isEmpty() || QDir::toNativeSeparators(newName).contains(QDir::separator())) {
			return false;
		}
		// Just emit signal not execute rename operation
		emit fileRenamed(parentPath, oldName, newName);
		return true;
	}

	DefaultFileListViewEditDelegate::DefaultFileListViewEditDelegate(QFileSystemModel* fileModel, QObject* parent) 
		: QStyledItemDelegate(parent)
		, mFileModel(fileModel) {
	}

	QWidget* DefaultFileListViewEditDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		QLineEdit* editor = new QLineEdit(parent);
		// ignored illegal char
		QRegExpValidator* validator = new QRegExpValidator(QRegExp("^[^/\\\\:*?\"<>|]+$"));
		editor->setValidator(validator);
		return editor;
	}

	void DefaultFileListViewEditDelegate::setEditorData(QWidget* editor, const QModelIndex& idx) const {
		QStyledItemDelegate::setEditorData(editor, idx);
		QLineEdit* le = qobject_cast<QLineEdit*>(editor);
		if (le == nullptr)
			return;
		emit startEditing();

		QObject src;
		// NOTICE: make setSelection effect.
		// the lambda function is executed using a queued connection
		connect(&src, &QObject::destroyed, le, [le, this, idx]() {
			//set default selection in the line edit
			QFileInfo file = mFileModel->filePath(idx);
			int selectLen = 0;
			if (file.isDir())
				selectLen = file.fileName().size();
			else
				selectLen = file.completeBaseName().size();
			le->setSelection(0, selectLen);
			}, Qt::QueuedConnection);
	}

	DefaultFileListView::DefaultFileListView(QWidget* parent)
		: QListView(parent) {
		mFileModel = new DefaultFileListViewModel(this);
		mFileModel->setReadOnly(false); // Set list view editable
		setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed); // Set edit mode.
		setSelectionMode(QAbstractItemView::ExtendedSelection); // Multi selection.
		setModel(mFileModel);

		DefaultFileListViewEditDelegate* itemEditDelegate = new DefaultFileListViewEditDelegate(mFileModel);
		connect(itemEditDelegate, &DefaultFileListViewEditDelegate::startEditing, [=]() {
			mEditing = true;
			});
		setItemDelegate(itemEditDelegate);

		mRootPathChangeStack = new QUndoStack(this);

		connect(this, &QListView::doubleClicked, this, &DefaultFileListView::OnItemDoubleClicked);
		connect(mFileModel, &QFileSystemModel::fileRenamed, this, &DefaultFileListView::OnItemRenamed);
		connect(this, &QListView::customContextMenuRequested, this, &DefaultFileListView::OnCustomContextMenuRequested);

		mDeleteForceShortcut = new QShortcut(QKeySequence("Shift+Delete"), this);
		mDeleteForceShortcut->setContext(Qt::WidgetShortcut);
		connect(mDeleteForceShortcut, &QShortcut::activated, this, &DefaultFileListView::OnActionDelete);
		mMoveToTrashShortcut = new QShortcut(Qt::Key_Delete, this);
		mMoveToTrashShortcut->setContext(Qt::WidgetShortcut);
		connect(mMoveToTrashShortcut, &QShortcut::activated, this, &DefaultFileListView::OnActionMoveToTrash);
		mBackwardShortcut = new QShortcut(Qt::Key_Backspace, this);
		connect(mBackwardShortcut, &QShortcut::activated, this, [this]() { this->Forward(); });
	}

	DefaultFileListView::~DefaultFileListView()
	{}

	QStringList DefaultFileListView::SelectedFiles() {
		QList<QString> files;
		QItemSelectionModel* m = selectionModel();
		QModelIndexList selection = m->selectedIndexes();
		for (const QModelIndex& index : selection)
		{
			if (index.column() != 0)
				continue;
			QString file = mFileModel->filePath(index);
			files << file;
		}
		return files;
	}

	QString DefaultFileListView::CurrentDir() {
		return mFileModel->rootPath();
	}

	QModelIndex DefaultFileListView::IndexOf(const QString& file) {
		return mFileModel->index(file);
	}

	void DefaultFileListView::Refresh() {
		QString root = CurrentDir();
		mFileModel->setRootPath("");
		ChangeRoot(root);
	}

	void DefaultFileListView::SetRootPath(const QFileInfo& root) {
		if (CurrentDir() == root.absoluteFilePath())
			return;
		mRootPathChangeStack->push(new ChangeRootPathCommand(this, root, CurrentDir()));
	}

	void DefaultFileListView::Forward() {
		int count = mRootPathChangeStack->count();
		int index = mRootPathChangeStack->index();
		if (index - 1 > 0)
			mRootPathChangeStack->undo();
	}

	void DefaultFileListView::Backward() {
		int count = mRootPathChangeStack->count();
		int index = mRootPathChangeStack->index();
		if (index < count)
			mRootPathChangeStack->redo();
	}

	void DefaultFileListView::Upward() {
		QDir dir(CurrentDir());
		if (dir.cdUp()) {
			SetRootPath(QFileInfo(dir.absolutePath()));
		}
	}

	void DefaultFileListView::ChangeRoot(const QFileInfo& root) {
		if (root.isDir()) {
			QModelIndex index = mFileModel->setRootPath(root.absoluteFilePath());
			setRootIndex(index); // IMPORTANT! refresh the ui
		}
	}

	void DefaultFileListView::OnItemDoubleClicked(const QModelIndex& index) {
		QFileInfo currentFileInfo = mFileModel->fileInfo(index);
		if (currentFileInfo.isDir()) {
			SetRootPath(currentFileInfo);
		}
		else if (currentFileInfo.isFile()) {
			QDesktopServices::openUrl(QUrl::fromLocalFile(currentFileInfo.absoluteFilePath()));
		}
	}

	void DefaultFileListView::OnItemRenamed(const QString& path, const QString& oldName, const QString& newName) {
		if (newName.isEmpty())
			return;
		QStringList files = SelectedFiles();
		if (files.isEmpty())
			return;

		FFX::FileHandlerPtr handler = std::make_shared<FFX::FileRenameHandler>(std::make_shared<FFX::FileNameReplaceByExpHandler>("*", newName));
		std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>());
		handler->Handle(FileInfoList(files));
	}

	void DefaultFileListView::OnCustomContextMenuRequested(const QPoint& pos) {

	}

	void DefaultFileListView::OnActionDelete() {
		FFX::FileHandlerPtr handler = std::make_shared<FFX::FileDeleteHandler>(true);
		QStringList files = SelectedFiles();
		if (files.isEmpty())
			return;

		handler->Handle(FileInfoList(files));
	}

	void DefaultFileListView::OnActionMoveToTrash() {
		FFX::FileHandlerPtr handler = std::make_shared<FFX::FileDeleteHandler>();
		QStringList files = SelectedFiles();
		if (files.isEmpty())
			return;

		QFileInfoList fileInfoList;
		for (const QString& file : files) {
			fileInfoList << file;
		}
		handler->Handle(fileInfoList);
	}

	DefaultFileListViewNavigator::DefaultFileListViewNavigator(QWidget* parent) 
		: QWidget(parent) {
		SetupUi();
	}

	void DefaultFileListViewNavigator::SetupUi() {
		mBackwardButton = new QToolButton;
		mBackwardButton->setIcon(QIcon(":/ffx/res/image/angle-left.svg"));
		mForwardButton = new QToolButton;
		mForwardButton->setIcon(QIcon(":/ffx/res/image/angle-right.svg"));
		mUpwardButton = new QToolButton;
		mUpwardButton->setIcon(QIcon(":/ffx/res/image/angle-up.svg"));
		mRootPathEdit = new QLineEdit;
		mMainLayout = new QHBoxLayout;
		mMainLayout->addWidget(mBackwardButton);
		mMainLayout->addWidget(mForwardButton);
		mMainLayout->addWidget(mUpwardButton);
		mMainLayout->addWidget(mRootPathEdit, 1);
		setLayout(mMainLayout);
	}

	FileMainView::FileMainView(QWidget* parent) 
		: QWidget(parent) {
		SetupUi();
	}

	void FileMainView::SetRootPath(const QString& path) {
		mFileListView->SetRootPath(path);
	}

	void FileMainView::SetupUi() {
		mFileViewNavigator = new DefaultFileListViewNavigator;
		mFileListView = new DefaultFileListView;
		mMainLayout = new QVBoxLayout;
		mMainLayout->addWidget(mFileViewNavigator);
		mMainLayout->addWidget(mFileListView, 1);
		setLayout(mMainLayout);
	}
}

