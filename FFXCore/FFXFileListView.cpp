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
		QRegExpValidator* validator = new QRegExpValidator(QRegExp(G_FILE_VALIDATOR));
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
				selectLen = File(file).BaseName().size();
			le->setSelection(0, selectLen);
			}, Qt::QueuedConnection);
	}

	DefaultFileListView::DefaultFileListView(QWidget* parent)
		: QListView(parent) {
		mFileModel = new DefaultFileListViewModel(this);
		mFileModel->setReadOnly(false); // Set list view editable
		setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed); // Set edit mode.
		setSelectionMode(QAbstractItemView::ExtendedSelection); // Multi selection.
		setSelectionRectVisible(true); // Set select rubber bound visible.
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
		mBackwardShortcut->setContext(Qt::WidgetShortcut);
		connect(mBackwardShortcut, &QShortcut::activated, this, [this]() { this->Forward(); });
		mInvertSelectShortcut = new QShortcut(QKeySequence("Ctrl+Alt+A"), this);
		mInvertSelectShortcut->setContext(Qt::WidgetShortcut);
		connect(mInvertSelectShortcut, &QShortcut::activated, this, &DefaultFileListView::OnInvertSelect);
	}

	DefaultFileListView::~DefaultFileListView()
	{}

	QStringList DefaultFileListView::SelectedFiles() {
		QList<QString> files;
		QItemSelectionModel* m = selectionModel();
		QModelIndexList selection = m->selectedIndexes();
		for (const QModelIndex& index : selection) {
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

	/// <summary>
	/// Rules of rename:
	/// 1. if suffix changed, the suffix included is true
	/// 2. if suffix not changed, the suffix indclude is false, and use the base name to match
	/// </summary>
	/// <param name="path"></param>
	/// <param name="oldName"></param>
	/// <param name="newName"></param>
	void DefaultFileListView::OnItemRenamed(const QString& path, const QString& oldName, const QString& newName) {
		if (newName.isEmpty())
			return;
		QStringList files = SelectedFiles();
		if (files.isEmpty())
			return;
		bool suffixInc = false;
		QString after = newName;
		QString oldsuffix = File(oldName).Suffix();
		QString newsuffix = File(newName).Suffix();
		if (oldsuffix != newsuffix) {
			suffixInc = true;
		} else if (!newsuffix.isEmpty()) {
			after = File(newName).BaseName();
		}
		FFX::FileHandlerPtr handler = std::make_shared<FFX::FileRenameHandler>(std::make_shared<FFX::FileNameReplaceByExpHandler>("*", after, QRegExp::Wildcard, true, suffixInc));
		std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>());
		QFileInfoList result = handler->Handle(FileInfoList(files));

		//! Set new file selected
		QModelIndex first = mFileModel->index(QDir(path).absoluteFilePath(newName));
		for (const QFileInfo& fi : result) {
			QModelIndex idx = mFileModel->index(fi.absoluteFilePath());
			if (idx.isValid() && !first.isValid()) {
				first = idx;
				break;
			}
		}
		setCurrentIndex(first);
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

	void DefaultFileListView::OnInvertSelect() {
		QModelIndexList selection = selectionModel()->selectedIndexes();
		selectAll();
		for (const QModelIndex& index : selection) {
			if (index.column() != 0)
				continue;
			selectionModel()->select(index, QItemSelectionModel::Deselect);
		}
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
		mMainLayout->setMargin(0);
		setLayout(mMainLayout);
	}

	FileMainView::FileMainView(QWidget* parent) 
		: QWidget(parent) {
		SetupUi();
	}

	void FileMainView::Goto(const QString& path) {
		mFileListView->SetRootPath(path);
	}

	void FileMainView::SetupUi() {
		mFileViewNavigator = new DefaultFileListViewNavigator;
		mFileListView = new DefaultFileListView;
		mMainLayout = new QVBoxLayout;
		mMainLayout->addWidget(mFileViewNavigator);
		mMainLayout->addWidget(mFileListView, 1);
		mMainLayout->setMargin(0);
		setLayout(mMainLayout);
	}
}

