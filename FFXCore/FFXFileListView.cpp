#include "FFXFileListView.h"
#include "FFXFileHandler.h"
#include "FFXMainWindow.h"
#include "FFXTaskPanel.h"
#include "FFXFileQuickView.h"
#include "FFXRenameDialog.h"
#include "FFXFilePropertyDialog.h"
#include "FFXFile.h"
#include "FFXString.h"
#include "FFXFileFilterExpr.h"
#include "FFXClipboardPanel.h"

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
#include <QProcess>
#include <QTimer>
#include <QCollator>
#include <QActionGroup>
#include <QPainter>
#include <QFileIconProvider>
#include <QDebug>

#ifdef Q_OS_WIN
#include <cstdlib>
#include <Windows.h>
#endif

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

	DefaultSortProxyModel::DefaultSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {
		setFilterKeyColumn(0);
	}
	
	void DefaultSortProxyModel::Refresh() {
		//! invalidate will clear all order and filter, so we must sort again.
		QSortFilterProxyModel::invalidate();
		sort((int)mOrderBy, mSortOrder);
	}
	
	void DefaultSortProxyModel::sort(int column, Qt::SortOrder order) {
		mOrderBy = (OrderBy)column;
		mSortOrder = order;
		QSortFilterProxyModel::sort(column, order);
	}

	bool DefaultSortProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
		QFileSystemModel* model = (QFileSystemModel*)sourceModel();

		QFileInfo leftInfo = model->fileInfo(source_left);
		QFileInfo rightInfo = model->fileInfo(source_right);
		bool left = leftInfo.isDir();
		bool right = rightInfo.isDir();
		if (left ^ right)
			return (mSortOrder == Qt::AscendingOrder) ? left : right;

		if (mOrderBy == OBName) {
			QCollator collator;
			return collator.compare(leftInfo.fileName(), rightInfo.fileName()) < 0;
		}
		if (mOrderBy == OBDate) {
			return leftInfo.lastModified() < rightInfo.lastModified();
		}
		if (mOrderBy == OBSize) {
			return leftInfo.size() < rightInfo.size();
		}
		if (mOrderBy == OBType) {
			QCollator collator;
			int compare = collator.compare(model->type(source_left), model->type(source_right));
			if (compare == 0)
				return collator.compare(leftInfo.fileName(), rightInfo.fileName()) < 0;
			return compare < 0;
		}

		return false;
	}
	
	bool DefaultSortProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
		if (mFileFilter == nullptr)
			return true;

		QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
		QFileSystemModel* model = (QFileSystemModel*)sourceModel();
		QString root = model->rootPath();
		QString filePath = model->filePath(idx);
		return filePath == root || mFileFilter->Accept(filePath);
	}
	
	void DefaultSortProxyModel::SetFilterExpr(const QString& filter) {
		mFilterExp = filter;
		FileFilterExpr fe(filter.toStdString(), true);
		mFileFilter = fe.Filter();
	}

	bool DefaultSortProxyModel::IsFilterSet() const {
		return mFileFilter != nullptr && mFilterExp != "*";
	}

	/************************************************************************************************************************
	 * Class： DefaultFileListViewModel
	 *
	 *
	/************************************************************************************************************************/
	DefaultFileListViewModel::DefaultFileListViewModel(QObject* parent)
		: QFileSystemModel(parent) {
		//! Do not watch the changed by QT, we watch it by hand.
		setOptions(QFileSystemModel::DontWatchForChanges);
	}

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
	DefaultFileListViewEditDelegate::DefaultFileListViewEditDelegate(QSortFilterProxyModel* fileModel, QObject* parent)
		: QStyledItemDelegate(parent)
		, mViewModel(fileModel) {
	}

	QWidget* DefaultFileListViewEditDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		QLineEdit* editor = new QLineEdit(parent);
		QRect rect = option.rect;

		//QRect editRect(rect.left() + 32 + mMargin * 8, rect.top() + mMargin, rect.right() - mMargin, 40);
		editor->setFixedSize(QSize(rect.width() - 32 - mMargin * 2, 40));
		//editor->setGeometry(editRect);
		
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
			QModelIndex index = mViewModel->mapToSource(idx);
			QFileInfo file = ((QFileSystemModel*)mViewModel->sourceModel())->filePath(index);
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

		QRect rect = option.rect;

		if (option.state.testFlag(QStyle::State_MouseOver)) {
			painter->fillRect(rect, QColor("#E5F3FF"));
		}
		if (option.state.testFlag(QStyle::State_Selected)) {
			painter->fillRect(rect, QColor("#CCE8FF"));
		}

		QSortFilterProxyModel* model = (QSortFilterProxyModel*)index.model();
		QModelIndex idx = model->mapToSource(index);
		QAbstractItemModel* fm = model->sourceModel();
		QFileSystemModel* fileModel = (QFileSystemModel*)model->sourceModel();

		QFileInfo fi = fileModel->fileInfo(idx);

		QFileIconProvider fip;
		QIcon icon = fip.icon(fi);
		QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(32, 32)));
		QRect iconRect(rect.left() + mMargin, rect.top() + mMargin, 32, 32);
		painter->drawPixmap(iconRect, pixmap);

		painter->setFont(QFont("Microsoft YaHei", 9));
		QRect fileNameRect(iconRect.right() + mMargin, rect.top() + mMargin, rect.width() - iconRect.width() - mMargin * 3, 30);
		painter->drawText(fileNameRect, Qt::AlignVCenter | Qt::AlignLeft, fi.fileName());

		painter->setFont(QFont("Microsoft YaHei", 6));
		QRect pathNameRect(iconRect.right() + mMargin, fileNameRect.bottom() + mMargin, rect.width() - 2 * mMargin, 30);

		QString subtitle = fi.lastModified().toString("yyyy-MM-dd hh:mm:ss");
		if (!fi.isDir()) {
			subtitle = QString("%1 \t %2").arg(subtitle).arg(String::BytesHint(FileSize(fi)));
		}
		painter->drawText(pathNameRect, Qt::AlignVCenter | Qt::AlignLeft, subtitle);

		// QStyledItemDelegate::paint(painter, opt, index);
	}

	QSize DefaultFileListViewEditDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
		Q_UNUSED(index)
		return QSize(option.rect.width(), mItemHeight); // Set the item height fixed.
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
		setSelectionBehavior(QAbstractItemView::SelectItems);
		setSelectionRectVisible(true); // Set select rubber bound visible.
		
		mSortProxyModel = new DefaultSortProxyModel;
		mSortProxyModel->setSourceModel(mFileModel);

		setModel(mSortProxyModel);
		//mSortProxyModel->setFilterRegExp()

		DefaultFileListViewEditDelegate* itemEditDelegate = new DefaultFileListViewEditDelegate(mSortProxyModel);
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
		connect(mPasteFilesShortcut, &QShortcut::activated, this, [=]() { CopyFiles(); });

		mOverwritePasteFilesShortcut = new QShortcut(QKeySequence("Ctrl+Shift+V"), this);
		mOverwritePasteFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mOverwritePasteFilesShortcut, &QShortcut::activated, this, [=]() { CopyFiles(true); });

		mMoveFilesShortcut = new QShortcut(QKeySequence("Ctrl+X"), this);
		mMoveFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mMoveFilesShortcut, &QShortcut::activated, this, [=]() { MoveFiles(); });

		mOverwriteMoveFilesShortcut = new QShortcut(QKeySequence("Ctrl+Shift+X"), this);
		mOverwriteMoveFilesShortcut->setContext(Qt::WidgetShortcut);
		connect(mOverwriteMoveFilesShortcut, &QShortcut::activated, this, [=]() { MoveFiles(true); });
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
			QModelIndex i = mSortProxyModel->mapToSource(index);
			QString file = mFileModel->filePath(i);
			files << file;
		}
		return files;
	}

	QString DefaultFileListView::CurrentDir() {
		return mFileModel->rootPath();
	}

	QModelIndex DefaultFileListView::IndexOf(const QString& file) {
		//return mFileModel->index(file);
		return mSortProxyModel->mapFromSource(mFileModel->index(file));
	}

	void DefaultFileListView::Refresh() {
		QString root = CurrentDir();
		mFileModel->setRootPath("");
		SetRootPath(root);
	}

	void DefaultFileListView::SetRootPath(const QFileInfo& root) {
		if (CurrentDir() == root.absoluteFilePath()) {
			return;
		}

		if (root.isDir()) {
			if (mSortProxyModel->IsFilterSet()) {
				QString oldFilterExp = mSortProxyModel->FilterExp();
				SetFilter("*");
			}
			
			QModelIndex index = mFileModel->setRootPath(root.absoluteFilePath());
			setRootIndex(mSortProxyModel->mapFromSource(index)); // IMPORTANT! refresh the ui

			qDebug() << "SetRootPath: " << root;
			if (mSortProxyModel->IsFilterSet()) {
				SetFilter(mSortProxyModel->FilterExp());
			}
		}
	}

	void DefaultFileListView::OnItemDoubleClicked(const QModelIndex& index) {
		QModelIndex idx = mSortProxyModel->mapToSource(index);
		QFileInfo currentFileInfo = mFileModel->fileInfo(idx);
		emit FileDoubleClicked(currentFileInfo);

	}

	void DefaultFileListView::keyPressEvent(QKeyEvent* event) {
		if (!mEditing && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
			QModelIndex idx = selectionModel()->currentIndex();
			//mSortProxyModel->mapSelectionFromSource(idx);
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
		FFX::FileHandlerPtr handler = std::make_shared<FFX::FileRenameHandler>(after, true, suffixInc);
		// std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>());
		QFileInfoList result = handler->Handle(FileInfoList(files));

		Refresh();
		//! Set new file selected
		QModelIndex first = mFileModel->index(QDir(path).absoluteFilePath(newName));
		first = mSortProxyModel->mapFromSource(first);
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
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->OpenCommandPromptAction());
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
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->CopyFilePathAction());
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->PropertyAction());
		} else {
			menu->addAction(MainWindow::Instance()->FileMainViewPtr()->PropertyAction());
		}
		menu->exec(QCursor::pos());
		delete menu;
	}

	void DefaultFileListView::OnActionDelete() {
		QMessageBox::StandardButton r = QMessageBox::warning(this, QObject::tr("Warning"), QStringLiteral("These files will be delete completely, Are you sure?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (r == QMessageBox::No)
			return;
		QStringList files = SelectedFiles();
		if (files.isEmpty())
			return;

		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<FFX::FileDeleteHandler>(true));
	}

	void DefaultFileListView::OnActionMoveToTrash() {
		FFX::FileHandlerPtr handler = std::make_shared<FFX::FileDeleteHandler>();
		QStringList files = SelectedFiles();
		if (files.isEmpty())
			return;

		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(files), std::make_shared<FFX::FileDeleteHandler>());
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

	void DefaultFileListView::CopyFiles(bool overwrite) {
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		if (mimeData == nullptr || !mimeData->hasUrls())
			return;

		QList<QUrl> urls = mimeData->urls();
		QString targetDir = CurrentDir();
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(urls), std::make_shared<FileCopyHandler>(targetDir, overwrite));
	}

	void DefaultFileListView::MoveFiles(bool overwrite) {
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
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(urls), std::make_shared<FileMoveHandler>(targetDir, overwrite));
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

	void DefaultFileListView::SetSortBy(OrderBy ob, Qt::SortOrder sort) {
		mSortProxyModel->sort(ob, sort);
	}

	void DefaultFileListView::SetFilter(const QString& filter) {
		mSortProxyModel->SetFilterExpr(filter);
		mSortProxyModel->Refresh();
	}

	PathEditWidget::PathEditWidget(QWidget* parent)
		: QLineEdit(parent) {

	}

	void PathEditWidget::focusInEvent(QFocusEvent* event) {
		QLineEdit::focusInEvent(event);
		QTimer::singleShot(0, this, &QLineEdit::selectAll);
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->setText(text());
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
		mRootPathEdit = new PathEditWidget;
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

		mGotoShortcut = new QShortcut(QKeySequence("Ctrl+G"), this);
		mGotoShortcut->setContext(Qt::WindowShortcut);
		connect(mGotoShortcut, &QShortcut::activated, this, &DefaultFileListViewNavigator::OnGoto);

		mRootPathChangeStack->setUndoLimit(15);
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

	void DefaultFileListViewNavigator::AddWidget(QWidget* widget) {
		mMainLayout->addWidget(widget);
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
		}
	}

	void DefaultFileListViewNavigator::OnGoto() {
		mRootPathEdit->setFocus();
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
		setObjectName("FileMainView");

		mFileViewNavigator = new DefaultFileListViewNavigator;
		mFileListView = new DefaultFileListView;
		mFileQuickView = new FileQuickView;
		mClipboardPanel = new ClipboardPanel;

		mMainLayout = new QVBoxLayout;
		mMakeDirAction = new QAction(QIcon(":/ffx/res/image/mk-folder.svg"), QObject::tr("Make Directory"));
		mMakeFileAction = new QAction(QIcon(":/ffx/res/image/mk-file.svg"), QObject::tr("New File"));
		mMakeFileActions.append(mMakeFileAction);
		mMakeFileActions.append(new QAction("New Zip File"));
		mPasteFilesAction = new QAction(QIcon(":/ffx/res/image/paste-files.svg"), QObject::tr("Copy Files Here"));
		mRefreshAction = new QAction(QIcon(":/ffx/res/image/refresh.svg"), QObject::tr("Refresh"));
		mRefreshAction->setShortcut(QKeySequence("F5"));
		mRefreshAction->setShortcutContext(Qt::WindowShortcut);
		mMoveFilesAction = new QAction(QIcon(":/ffx/res/image/move-files.svg"), QObject::tr("Move Files Here"));
		mEnvelopeFilesAction = new QAction(QIcon(":/ffx/res/image/file-envelope.svg"), QObject::tr("Envelope Files By Folder"));
		mClearFolderAction = new QAction(QIcon(":/ffx/res/image/clear-folders.svg"), QObject::tr("Clear Folder"));
		mFixedToQuickPanelAction = new QAction(QIcon(":/ffx/res/image/pin.svg"), QObject::tr("Fix in Quick Panel"));
		mRenameAction = new QAction(QIcon(":/ffx/res/image/edit.svg"), QObject::tr("Rename"));
		mPropertyAction = new QAction(QIcon(":/ffx/res/image/file-prop.svg"), QObject::tr("Property"));
		mCopyFilePathAction = new QAction(QIcon(":/ffx/res/image/text-input.svg"), QObject::tr("Copy File Path"));
		mOpenCommandPromptAction = new QAction(QIcon(":/ffx/res/image/terminal.svg"), QObject::tr("Open in Command Prompt"));

		//mMainLayout->addWidget(mFileViewNavigator);
		QSplitter* splitter = new QSplitter(Qt::Horizontal);
		QWidget* rightWidget = new QWidget;
		QVBoxLayout* rightWidgetLayout = new QVBoxLayout;
		rightWidget->setLayout(rightWidgetLayout);
		rightWidgetLayout->setMargin(0);
		rightWidgetLayout->addWidget(mFileViewNavigator);
		rightWidgetLayout->addWidget(mFileListView);

		splitter->addWidget(mFileQuickView);
		splitter->addWidget(rightWidget);
		splitter->addWidget(mClipboardPanel);

		splitter->setStretchFactor(0, 2);
		splitter->setStretchFactor(1, 5);
		splitter->setStretchFactor(2, 4);
		mMainLayout->addWidget(splitter);

		mSetFileFilterShortcut = new QShortcut(QKeySequence("Ctrl+Shift+F"), this);
		mSetFileFilterShortcut->setContext(Qt::WindowShortcut);

		mFilterEdit = new QLineEdit;
		mFilterEdit->setPlaceholderText(QObject::tr("Set Filter"));
		mClearFilterAction = new QAction(QIcon(":/ffx/res/image/backspace.svg"), QObject::tr("Clear"));
		mFilterEdit->addAction(mClearFilterAction, QLineEdit::TrailingPosition);
		mClearFilterAction->setVisible(false);
		mFileViewNavigator->AddWidget(mFilterEdit);
		mRefreshFileListButton = new QToolButton;
		mRefreshFileListButton->setDefaultAction(mRefreshAction);
		//mRefreshFileListButton->setIcon(QIcon(":/ffx/res/image/refresh.svg"));
		mRefreshFileListButton->setFixedSize(QSize(32, 32));
		mRefreshFileListButton->setIconSize(QSize(16, 16));
		mFileViewNavigator->AddWidget(mRefreshFileListButton);
		mRefreshFileListButton->setDefaultAction(mRefreshAction);

		mSetFileListOrderButton = new QToolButton;
		mSetFileListOrderButton->setIcon(QIcon(":/ffx/res/image/sort.svg"));
		mSetFileListOrderButton->setText(QObject::tr("Order by"));
		mSetFileListOrderButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mSetFileListOrderButton->setFixedHeight(32);
		mSetFileListOrderButton->setIconSize(QSize(16, 16));
		mSetFileListOrderButton->setPopupMode(QToolButton::InstantPopup);
		QMenu* menu = new QMenu;
		mOrderByActionGroup = new QActionGroup(this);
		mOrderByActionGroup->addAction(new QAction(QObject::tr("Order by Name")))->setCheckable(true);
		mOrderByActionGroup->addAction(new QAction(QObject::tr("Order by Date")))->setCheckable(true);
		mOrderByActionGroup->addAction(new QAction(QObject::tr("Order by Size")))->setCheckable(true);
		mOrderByActionGroup->addAction(new QAction(QObject::tr("Order by Type")))->setCheckable(true);
		mOrderByActionGroup->actions()[0]->setChecked(true);

		mSortActionGroup = new QActionGroup(this);
		mSortActionGroup->addAction(new QAction(QObject::tr("Asc")))->setCheckable(true);
		mSortActionGroup->addAction(new QAction(QObject::tr("Desc")))->setCheckable(true);
		mSortActionGroup->actions()[0]->setChecked(true);
		for (QAction* action : mOrderByActionGroup->actions())
			menu->addAction(action);
		menu->addSeparator();
		for (QAction* action : mSortActionGroup->actions())
			menu->addAction(action);

		mSetFileListOrderButton->setMenu(menu);

		mFileViewNavigator->AddWidget(mSetFileListOrderButton);
		
		mMainLayout->setContentsMargins(5, 0, 0, 0);
		setLayout(mMainLayout);

		connect(mFixedToQuickPanelAction, &QAction::triggered, this, &FileMainView::OnFixedToQuickPanel);
		connect(mFileViewNavigator, &DefaultFileListViewNavigator::RootPathChanged, this, [=](const QString& path) {
			mFileListView->SetRootPath(path);
			emit CurrentPathChanged(path); // Transfer the signals for 
			});
		connect(mFileQuickView->QuickNaviPanelPtr(), &QuickNavigatePanel::RootPathChanged, this, &FileMainView::OnRootPathChanged);
		connect(mFileQuickView->FileTreeNaviPanelPtr(), &FileTreeNavigatePanel::RootPathChanged, this, &FileMainView::OnRootPathChanged);
		connect(mFileListView, &DefaultFileListView::FileDoubleClicked, this, &FileMainView::OnFileDoubleClicked);

		connect(mMakeDirAction, &QAction::triggered, mFileListView, &DefaultFileListView::MakeDirAndEdit);
		connect(mMakeFileAction, &QAction::triggered, mFileListView, [=]() { mFileListView->MakeFileAndEdit(""); });

		connect(mPasteFilesAction, &QAction::triggered, mFileListView, [=]() { mFileListView->CopyFiles(); });
		connect(mMoveFilesAction, &QAction::triggered, mFileListView, [=]() { mFileListView->MoveFiles(); });

		connect(mRefreshAction, &QAction::triggered, mFileListView, &DefaultFileListView::Refresh);

		connect(mFileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]() { emit SelectionChanged(mFileListView->SelectedFiles()); });
		
		connect(mEnvelopeFilesAction, &QAction::triggered, this, &FileMainView::OnEnvelopeFiles);
		connect(mClearFolderAction, &QAction::triggered, this, &FileMainView::OnClearFolder);
		connect(mRenameAction, &QAction::triggered, this, &FileMainView::OnRename);
		connect(mPropertyAction, &QAction::triggered, this, &FileMainView::OnFileProperty);
		connect(mCopyFilePathAction, &QAction::triggered, this, &FileMainView::OnCopyFilePath);
		connect(mOpenCommandPromptAction, &QAction::triggered, this, &FileMainView::OnOpenCommandPrompt);

		connect(mOrderByActionGroup, &QActionGroup::triggered, this, &FileMainView::OnSetOrderBy);
		connect(mSortActionGroup, &QActionGroup::triggered, this, &FileMainView::OnSetOrderBy);

		connect(mFilterEdit, &QLineEdit::returnPressed, this, &FileMainView::OnFileFilterChanged);

		connect(mSetFileFilterShortcut, &QShortcut::activated, this, &FileMainView::OnFocusFileFilterSet);
		connect(mFilterEdit, &QLineEdit::textChanged, this, &FileMainView::OnFileFilterEditTextChanged);
		connect(mClearFilterAction, &QAction::triggered, this, &FileMainView::OnClearFileFilter);		
		//connect(mRefreshFileListButton, &QToolButton::clicked, this, &DefaultFileListView::Refresh);
	}

	void FileMainView::RefreshFileListView() {
		mFileListView->Refresh();
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

	void FileMainView::OnEnvelopeFiles() {
		QStringList selectedFiles = mFileListView->SelectedFiles();
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(selectedFiles), std::make_shared<FileEnvelopeByDirHandler>());
	}

	void FileMainView::OnClearFolder() {
		QStringList selectedFiles = mFileListView->SelectedFiles();
		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(selectedFiles), std::make_shared<ClearFolderHandler>());
	}

	void FileMainView::OnRename() {
		QStringList selectedFiles = mFileListView->SelectedFiles();
		RenameDialog dialog(selectedFiles);
		dialog.exec();
	}

	void FileMainView::OnFileProperty() {
		QFileInfoList files = FileInfoList(mFileListView->SelectedFiles());
		if (files.isEmpty())
			files = FileInfoList(mFileListView->CurrentDir());
		FilePropertyDialog dialog(files);
		dialog.exec();
	}

	void FileMainView::OnCopyFilePath() {
		QFileInfoList files = FileInfoList(mFileListView->SelectedFiles());
		if (files.size() != 1)
			return;

		QClipboard* clipboard = QApplication::clipboard();
		clipboard->setText(files[0].absoluteFilePath());
	}

	void FileMainView::OnOpenCommandPrompt() {
		QString root = mFileListView->CurrentDir();
#ifdef Q_OS_WIN
		QString cmd = QString("/k cd /d \"%1\"").arg(root);
		ShellExecute(NULL, NULL, L"cmd", cmd.toStdWString().c_str(), NULL, SW_SHOWNORMAL);
#endif
	}

	void FileMainView::OnSetOrderBy() {
		QAction* order = mOrderByActionGroup->checkedAction();
		int column = mOrderByActionGroup->actions().indexOf(order);
		QAction* sort = mSortActionGroup->checkedAction();
		bool asc = mSortActionGroup->actions().indexOf(sort) == 0;
		mFileListView->SetSortBy((OrderBy)column, asc ? Qt::AscendingOrder : Qt::DescendingOrder);
	}

	void FileMainView::OnFileFilterChanged() {
		QString filter = mFilterEdit->text();
		if (filter.isEmpty())
			return;

		mFileListView->SetFilter(filter);
	}

	void FileMainView::OnFocusFileFilterSet() {
		mFilterEdit->setFocus();
	}

	void FileMainView::OnFileFilterEditTextChanged() {
		QString text = mFilterEdit->text();
		if (!text.isEmpty()) {
			mClearFilterAction->setVisible(true);
		} else {
			mClearFilterAction->setVisible(false);
			mFileListView->SetFilter("*");
		}
	}

	void FileMainView::OnClearFileFilter() {
		mFilterEdit->clear();
		mClearFilterAction->setVisible(false);
		mFileListView->SetFilter("*");
	}

	void FileMainView::Save(AppConfig* config) {
		QuickNavigatePanel* quickPanel = mFileQuickView->QuickNaviPanelPtr();
	
		config->WritePairItemArray(objectName(), quickPanel->Items());
		config->WriteItem(objectName(), "RootPath", RootPath());
		
		config->WriteItem(objectName(), "OrderBy", mFileListView->GetOrderBy());
		config->WriteItem(objectName(), "SortOrder", mFileListView->GetSortOrder());
	}

	void FileMainView::Restore(AppConfig* config) {
		FileQuickViewPtr()->QuickNaviPanelPtr()->AddItem(config->ReadPairItemArray(objectName()));
		
		int orderBy = config->ReadItem(objectName(), "OrderBy").toInt();
		int sortOrder = config->ReadItem(objectName(), "SortOrder").toInt();
		mOrderByActionGroup->actions()[orderBy]->setChecked(true);
		mSortActionGroup->actions()[sortOrder]->setChecked(true);
		mFileListView->SetSortBy((OrderBy)orderBy, (Qt::SortOrder)sortOrder);

		QString rootPath = config->ReadItem(objectName(), "RootPath").toString();
		if (rootPath.isEmpty()) {
			rootPath = QDir::currentPath();
		}
		Goto(rootPath);
	}
}

