#include "FFXFileListView.h"
#include "FFXFileHandler.h"
#include "FFXMainWindow.h"
#include "FFXTaskPanel.h"
#include "FFXFileQuickView.h"

#include <QLineEdit>
#include <QDesktopServices>
#include <QUrl>
#include <QShortcut>
#include <QToolButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAction>
#include <QMessageBox>
#include <QKeyEvent>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QSplitter>
#include <QMenu>


namespace FFX {
	/************************************************************************************************************************
	 * Class： ChangeRootPathCommand
	 *
	 *
	/************************************************************************************************************************/
	ChangeRootPathCommand::ChangeRootPathCommand(DefaultFileListViewNavigator* navigator, const QFileInfo& newPath, const QFileInfo& oldPath, QUndoCommand* parent)
		: QUndoCommand(parent)
		, mFileListViewNavigator(navigator)
		, mNewPath(newPath)
		, mOldPath(oldPath)	{

	}

	void ChangeRootPathCommand::undo() {
		mFileListViewNavigator->ChangePath(mOldPath.absoluteFilePath());
	}

	void ChangeRootPathCommand::redo() {
		mFileListViewNavigator->ChangePath(mNewPath.absoluteFilePath());
	}

	/************************************************************************************************************************
	 * Class： DefaultFileListViewModel
	 *
	 *
	/************************************************************************************************************************/
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

	/************************************************************************************************************************
	 * Class： DefaultFileListViewEditDelegate
	 *
	 *
	/************************************************************************************************************************/
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

	void DefaultFileListViewEditDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		QStyleOptionViewItem opt = option;
		if (opt.state & QStyle::State_HasFocus && !(opt.state & QStyle::State_Selected))
			opt.state &= ~QStyle::State_HasFocus;
		QStyledItemDelegate::paint(painter, opt, index);
	}

	/************************************************************************************************************************
	 * Class： DefaultFileListView
	 *
	 *
	/************************************************************************************************************************/
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

		connect(this, &QListView::doubleClicked, this, &DefaultFileListView::OnItemDoubleClicked);
		connect(mFileModel, &QFileSystemModel::fileRenamed, this, &DefaultFileListView::OnItemRenamed);
		connect(this, &QListView::customContextMenuRequested, this, &DefaultFileListView::OnCustomContextMenuRequested);

		mDeleteForceShortcut = new QShortcut(QKeySequence("Shift+Delete"), this);
		mDeleteForceShortcut->setContext(Qt::WidgetShortcut);
		connect(mDeleteForceShortcut, &QShortcut::activated, this, &DefaultFileListView::OnActionDelete);
		mMoveToTrashShortcut = new QShortcut(Qt::Key_Delete, this);
		mMoveToTrashShortcut->setContext(Qt::WidgetShortcut);
		connect(mMoveToTrashShortcut, &QShortcut::activated, this, &DefaultFileListView::OnActionMoveToTrash);
		mInvertSelectShortcut = new QShortcut(QKeySequence("Ctrl+Alt+A"), this);
		mInvertSelectShortcut->setContext(Qt::WidgetShortcut);
		connect(mInvertSelectShortcut, &QShortcut::activated, this, &DefaultFileListView::OnInvertSelect);
		mCollectFilesShortcut = new QShortcut(QKeySequence("Ctrl+C"), this);
		mCollectFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mCollectFilesShortcut, &QShortcut::activated, this, &DefaultFileListView::OnCollectFiles);
		mAppendCollectFilesShortcut = new QShortcut(QKeySequence("Ctrl+Shift+C"), this);
		mAppendCollectFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mAppendCollectFilesShortcut, &QShortcut::activated, this, &DefaultFileListView::OnAppendCollectFiles);
		mPasteFilesShortcut = new QShortcut(QKeySequence("Ctrl+V"), this);
		mPasteFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mPasteFilesShortcut, &QShortcut::activated, this, &DefaultFileListView::OnCopyFiles);
		mMoveFilesShortcut = new QShortcut(QKeySequence("Ctrl+X"), this);
		mMoveFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mMoveFilesShortcut, &QShortcut::activated, this, &DefaultFileListView::OnMoveFiles);

		setContextMenuPolicy(Qt::CustomContextMenu);
		//! Set row spacing to 2.
		setSpacing(2);
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
		SetRootPath(root);
	}

	void DefaultFileListView::SetRootPath(const QFileInfo& root) {
		if (CurrentDir() == root.absoluteFilePath())
			return;

		if (root.isDir()) {
			QModelIndex index = mFileModel->setRootPath(root.absoluteFilePath());
			setRootIndex(index); // IMPORTANT! refresh the ui
		}
	}

	void DefaultFileListView::OnItemDoubleClicked(const QModelIndex& index) {
		QFileInfo currentFileInfo = mFileModel->fileInfo(index);
		emit FileDoubleClicked(currentFileInfo);

	}

	void DefaultFileListView::keyPressEvent(QKeyEvent* event) {
		if (!mEditing && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
			QModelIndex idx = selectionModel()->currentIndex();
			if (idx.isValid()) {
				OnItemDoubleClicked(idx);
				return;
			}
		}
		QListView::keyPressEvent(event);
	}

	void DefaultFileListView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) {
		QAbstractItemView::closeEditor(editor, hint);
		mEditing = false;
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
		QMenu* menu = new QMenu(this);
		menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
		menu->setAttribute(Qt::WA_TranslucentBackground);
		menu->setMinimumWidth(200);
		QList<QString> selectFiles = SelectedFiles();
		
		if (selectFiles.isEmpty()) {
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->RefreshAction());
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->FixedToQuickPanelAction());
			menu->addSeparator();
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->MakeDirAction());
			QMenu* makefileMenu = new QMenu(QObject::tr("Make File..."));
			makefileMenu->setIcon(QIcon(":/ffx/res/image/plus.svg"));
			menu->addAction(makefileMenu->menuAction());
			QList<QAction*> makefileActions = MainWindow::Instance()->FileMainViewPtr()->MakeFileActions();
			for (int i = 0; i < makefileActions.size(); i++) {
				makefileMenu->addAction(makefileActions[i]);
			}

			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->PasteFilesAction());
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->MoveFilesAction());
		} else if(selectFiles.size() == 1) {
			if (QFileInfo(selectFiles[0]).isDir()) {
				menu->addAction(MainWindow::Instance()->FileMainViewPtr()->FixedToQuickPanelAction());
			}
		}
		menu->exec(QCursor::pos());
		delete menu;
	}

	void DefaultFileListView::OnActionDelete() {
		QMessageBox::StandardButton r = QMessageBox::warning(this, QObject::tr("Warning"), QStringLiteral("These files will be delete completely, Are you sure?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (r == QMessageBox::No)
			return;
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

	void DefaultFileListView::OnCollectFiles() {
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->clear();
		QList<QString> files = SelectedFiles();
		if (files.isEmpty())
			return;

		QList<QUrl> urls;
		for (const QString& file : files)
			urls << QUrl::fromLocalFile(file);
		QMimeData* mimeData = new QMimeData;
		mimeData->setUrls(urls);
		clipboard->setMimeData(mimeData);
	}

	void DefaultFileListView::OnAppendCollectFiles() {
		QList<QString> files = SelectedFiles();
		if (files.isEmpty())
			return;

		QList<QUrl> urls;
		for (const QString& file : files)
			urls << QUrl::fromLocalFile(file);

		QSet<QUrl> urlset(urls.begin(), urls.end());
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		QList<QUrl> curUrls = mimeData->urls();
		if (mimeData != nullptr && mimeData->hasUrls())
			urlset += QSet<QUrl>(curUrls.begin(), curUrls.end());

		QMimeData* newMimeData = new QMimeData;
		newMimeData->setUrls(urlset.values());
		clipboard->clear();
		clipboard->setMimeData(newMimeData);
	}

	void DefaultFileListView::OnCopyFiles() {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls())
			return;

		QList<QUrl> urls = mimeData->urls();
		QString targetDir = CurrentDir();
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(urls), std::make_shared<FileCopyHandler>(targetDir));
	}

	void DefaultFileListView::OnMoveFiles() {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls())
			return;

		QList<QUrl> urls = mimeData->urls();
		QString targetDir = CurrentDir();
		QFileInfoList files;
		for (const QUrl& url : urls)
		{
			QFileInfo file(url.toLocalFile());
			//! if the target dir equals with the file's parent, ignored!
			if (file.absolutePath() == targetDir)
				continue;
			files << file;
		}
		if (files.isEmpty())
			return;
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(urls), std::make_shared<FileMoveHandler>(targetDir));
	}

	void DefaultFileListView::MakeDirAndEdit() {
		QDir dir(CurrentDir());
		QString baseNewDirName(QObject::tr("New Directory"));
		int count = 1;
		QString newDirName = baseNewDirName;
		while (QFileInfo::exists(dir.absoluteFilePath(newDirName))) {
			newDirName = QStringLiteral("%1_%2").arg(baseNewDirName).arg(count++);
		}
		dir.mkdir(newDirName);
		QModelIndex idx = IndexOf(dir.absoluteFilePath(newDirName));
		setCurrentIndex(idx);
		edit(idx);
	}

	void DefaultFileListView::MakeFileAndEdit(const QString& name) {
		QDir dir(CurrentDir());
		QString baseNewFileName(name.isEmpty() ? QObject::tr("New File") : name);
		int count = 1;
		QString newFileName = baseNewFileName;
		while (QFileInfo::exists(dir.absoluteFilePath(newFileName))) {
			newFileName = QStringLiteral("%1_%2").arg(baseNewFileName).arg(count++);
		}
		
		QFile file(dir.absoluteFilePath(newFileName));
		if (!file.open(QIODevice::WriteOnly)) {
			QMessageBox::critical(this, QObject::tr("Error"), QObject::tr("Create file failed:%1").arg(file.errorString()));
			return;
		}
		file.close();

		QModelIndex idx = IndexOf(dir.absoluteFilePath(newFileName));
		setCurrentIndex(idx);
		edit(idx);
	}

	/************************************************************************************************************************
	 * Class： DefaultFileListViewNavigator
	 *
	 *
	/************************************************************************************************************************/
	DefaultFileListViewNavigator::DefaultFileListViewNavigator(QWidget* parent)
		: QWidget(parent) {
		SetupUi();
	}

	void DefaultFileListViewNavigator::SetupUi() {
		mBackwardButton = new QToolButton;
		mBackwardButton->setIcon(QIcon(":/ffx/res/image/angle-left.svg"));
		mBackwardButton->setFixedSize(QSize(32, 32));
		connect(mBackwardButton, &QToolButton::clicked, this, &DefaultFileListViewNavigator::OnBackward);
		mForwardButton = new QToolButton;
		mForwardButton->setIcon(QIcon(":/ffx/res/image/angle-right.svg"));
		mForwardButton->setFixedSize(QSize(32, 32));
		connect(mForwardButton, &QToolButton::clicked, this, &DefaultFileListViewNavigator::OnForward);
		mUpwardButton = new QToolButton;
		mUpwardButton->setIcon(QIcon(":/ffx/res/image/angle-up.svg"));
		mUpwardButton->setFixedSize(QSize(32, 32));
		connect(mUpwardButton, &QToolButton::clicked, this, &DefaultFileListViewNavigator::OnUpward);
		mRootPathEdit = new QLineEdit;
		mRootPathEdit->setFixedHeight(32);
		mMainLayout = new QHBoxLayout;
		mMainLayout->addWidget(mBackwardButton);
		mMainLayout->addWidget(mForwardButton);
		mMainLayout->addWidget(mUpwardButton);
		mMainLayout->addWidget(mRootPathEdit, 1);
		mMainLayout->setContentsMargins(0, 9, 0, 0);
		setLayout(mMainLayout);

		mRootPathChangeStack = new QUndoStack(this);
		connect(mRootPathEdit, &QLineEdit::returnPressed, this, [=]() { Goto(mRootPathEdit->text()); });

		mBackwardShortcut = new QShortcut(Qt::Key_Backspace, this);
		mBackwardShortcut->setContext(Qt::WindowShortcut);
		connect(mBackwardShortcut, &QShortcut::activated, this, &DefaultFileListViewNavigator::OnBackward);
	}

	void DefaultFileListViewNavigator::Goto(const QString& path) {
		if (path == mCurrentPath)
			return;
		if (!QFileInfo::exists(path)) {
			QMessageBox::warning(this, QObject::tr("Warning"), QObject::tr("Invalid directory path."));
			return;
		}
		mRootPathChangeStack->push(new ChangeRootPathCommand(this, path, mCurrentPath));
	}

	void DefaultFileListViewNavigator::ChangePath(const QString& path) {
		mCurrentPath = path;
		mRootPathEdit->setText(mCurrentPath);
		emit RootPathChanged(mCurrentPath);
	}

	void DefaultFileListViewNavigator::OnBackward() {
		int count = mRootPathChangeStack->count();
		int index = mRootPathChangeStack->index();
		if (index - 1 > 0)
			mRootPathChangeStack->undo();
	}

	void DefaultFileListViewNavigator::OnForward() {
		int count = mRootPathChangeStack->count();
		int index = mRootPathChangeStack->index();
		if (index < count)
			mRootPathChangeStack->redo();
	}

	void DefaultFileListViewNavigator::OnUpward() {
		QString root = mCurrentPath;
		QDir dir(root);
		if (dir.cdUp()) {
			Goto(dir.absolutePath());
			//SetRootPath(QFileInfo(dir.absolutePath()));
		}
	}

	/************************************************************************************************************************
	 * Class： FileMainView
	 *
	 *
	/************************************************************************************************************************/
	FileMainView::FileMainView(QWidget* parent) 
		: QWidget(parent) {
		SetupUi();
	}

	void FileMainView::Goto(const QString& path) {
		if (path.isEmpty()) {
			return;
		}
		mFileViewNavigator->Goto(path);
	}

	void FileMainView::SetupUi() {
		mFileViewNavigator = new DefaultFileListViewNavigator;
		mFileListView = new DefaultFileListView;
		mFileQuickView = new FileQuickView;
		mMainLayout = new QVBoxLayout;
		mMakeDirAction = new QAction(QIcon(":/ffx/res/image/mk-folder.svg"), QObject::tr("Make Directory"));
		mMakeFileAction = new QAction(QIcon(":/ffx/res/image/mk-file.svg"), QObject::tr("New File"));
		mPasteFilesAction = new QAction(QIcon(":/ffx/res/image/paste-files.svg"), QObject::tr("Copy Files"));
		mRefreshAction = new QAction(QIcon(":/ffx/res/image/refresh.svg"), QObject::tr("Refresh"));
		mMoveFilesAction = new QAction(QIcon(":/ffx/res/image/move-files.svg"), QObject::tr("Move Files"));
		//mMoveFilesAction->setShortcut(QKeySequence("Ctrl+X"));
		mFixedToQuickPanelAction = new QAction(QIcon(":/ffx/res/image/pin.svg"), QObject::tr("Fix to quick panel"));
		connect(mFixedToQuickPanelAction, &QAction::triggered, this, &FileMainView::OnFixedToQuickPanel);

		mMakeFileActions.append(mMakeFileAction);
		mMakeFileActions.append(new QAction("New Zip File"));

		mMainLayout->addWidget(mFileViewNavigator);
		QSplitter* splitter = new QSplitter(Qt::Horizontal);
		splitter->addWidget(mFileQuickView);
		splitter->addWidget(mFileListView);
		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 4);
		mMainLayout->addWidget(splitter, 1);

		mMainLayout->setContentsMargins(5, 0, 0, 0);
		setLayout(mMainLayout);

		connect(mFileViewNavigator, &DefaultFileListViewNavigator::RootPathChanged, this, [=](const QString& path) {
			mFileListView->SetRootPath(path);
			emit CurrentPathChanged(path); // Transfer the signals for 
			});
		connect(mFileQuickView->QuickNaviPanelPtr(), &QuickNavigatePanel::RootPathChanged, this, &FileMainView::OnRootPathChanged);
		connect(mFileQuickView->FileTreeNaviPanelPtr(), &FileTreeNavigatePanel::RootPathChanged, this, &FileMainView::OnRootPathChanged);
		connect(mFileListView, &DefaultFileListView::FileDoubleClicked, this, &FileMainView::OnFileDoubleClicked);

		connect(mMakeDirAction, &QAction::triggered, mFileListView, &DefaultFileListView::MakeDirAndEdit);
		connect(mMakeFileAction, &QAction::triggered, mFileListView, [=]() { mFileListView->MakeFileAndEdit(""); });

		connect(mPasteFilesAction, &QAction::triggered, mFileListView, &DefaultFileListView::OnCopyFiles);
		connect(mMoveFilesAction, &QAction::triggered, mFileListView, &DefaultFileListView::OnMoveFiles);

		connect(mRefreshAction, &QAction::triggered, mFileListView, &DefaultFileListView::Refresh);
	}

	QStringList FileMainView::SelectedFiles() {
		return mFileListView->SelectedFiles();
	}

	QString FileMainView::RootPath() {
		return mFileListView->CurrentDir();
	}

	QAction* FileMainView::MakeDirAction() {
		return mMakeDirAction;
	}

	void FileMainView::AddMakeFileAction(QAction* action) {
		mMakeFileActions.append(action);
	}

	QList<QAction*> FileMainView::MakeFileActions() {
		return mMakeFileActions;
	}

	QAction* FileMainView::PasteFilesAction() {
		return mPasteFilesAction;
	}

	QAction* FileMainView::MoveFilesAction() {
		return mMoveFilesAction;
	}

	QAction* FileMainView::FixedToQuickPanelAction() {
		return mFixedToQuickPanelAction;
	}

	void FileMainView::OnFileDoubleClicked(const QFileInfo& file) {
		if (file.isDir()) {
			Goto(file.absoluteFilePath());
		} else if (file.isFile()) {
			QDesktopServices::openUrl(QUrl::fromLocalFile(file.absoluteFilePath()));
		}
	}

	void FileMainView::OnRootPathChanged(const QFileInfo& file) {
		Goto(file.absoluteFilePath());
	}

	void FileMainView::OnFixedToQuickPanel() {
		QString root = mFileListView->CurrentDir();
		QStringList selectedFiles = mFileListView->SelectedFiles();
		if (selectedFiles.size() == 1 && QFileInfo(selectedFiles[0]).isDir()) {
			root = selectedFiles[0];
		}

		if (mFileQuickView->QuickNaviPanelPtr()->IsDirFixed(root)) {
			mFileQuickView->QuickNaviPanelPtr()->RemoveItem(root);
			return;
		}

		if(!mFileQuickView->QuickNaviPanelPtr()->IsFull()) {
			mFileQuickView->QuickNaviPanelPtr()->AddItem(root);
		}
	}
}

