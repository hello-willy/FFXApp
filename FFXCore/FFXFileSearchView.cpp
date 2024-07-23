#include "FFXFileSearchView.h"
#include "FFXMainWindow.h"

#include <QPainter>
#include <QFileIconProvider>
#include <QAction>
#include <QGridLayout>
#include <QLineEdit>
#include <QToolButton>

namespace FFX {
	/************************************************************************************************************************
	 * Class：	SearchFileListViewModel
	 * Purpose:	The model for storing search results
	 *
	/************************************************************************************************************************/
	SearchFileListViewModel::SearchFileListViewModel(QObject* parent) : QAbstractListModel(parent) {

	}

	int SearchFileListViewModel::rowCount(const QModelIndex& parent) const {
		return mListFileLoaded.size();
	}

	QVariant SearchFileListViewModel::data(const QModelIndex& index, int role) const {
		if (role == Qt::DisplayRole)
			return mListFileLoaded[index.row()];
		return QVariant();
	}

	void SearchFileListViewModel::Append(const QString& file) {
		beginResetModel();
		mListFileLoaded << file;
		endResetModel();
	}

	void SearchFileListViewModel::Clear() {
		beginResetModel();
		mListFileLoaded.clear();
		endResetModel();
	}

	QModelIndex SearchFileListViewModel::IndexOf(const QString& file) {
		int row = mListFileLoaded.indexOf(file);
		return QAbstractListModel::index(row);
	}

	/************************************************************************************************************************
	 * Class：	SearchFileListViewItemDelegate
	 * Purpose:	Delegate used to draw search result list item
	 *
	/************************************************************************************************************************/
	void SearchFileListViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		if (!index.isValid())
			return;
		QString file = index.data().toString();
		painter->setRenderHint(QPainter::Antialiasing, true);
		painter->save();

		QRect rect = option.rect;

		if (option.state.testFlag(QStyle::State_MouseOver)) {
			painter->fillRect(rect, QColor("#E5F3FF"));
		}
		if (option.state.testFlag(QStyle::State_Selected)) {
			painter->fillRect(rect, QColor("#CCE8FF"));
		}
		QFileIconProvider fip;
		QIcon icon = fip.icon(file);
		QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(32, 32)));
		QRect iconRect(rect.left() + 5, rect.top() + 5, 32, 32);
		painter->drawPixmap(iconRect, pixmap);

		QFileInfo fi(file);
		QFont font("Microsoft YaHei", 9);
		painter->setFont(font);
		QRect fileNameRect(iconRect.right() + 5, rect.top() + 5, rect.width() - iconRect.width() - 5, 30);
		painter->drawText(fileNameRect, Qt::AlignVCenter | Qt::AlignLeft, fi.fileName());
		painter->setFont(QFont("Microsoft YaHei", 6));
		QRect pathNameRect(iconRect.right() + 5, fileNameRect.bottom() + 5, rect.width(), 30);
		painter->drawText(pathNameRect, Qt::AlignVCenter | Qt::AlignLeft, fi.absolutePath());
	}

	QSize SearchFileListViewItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
		Q_UNUSED(index)
		return QSize(option.rect.width(), 80); // Set the item height fixed.
	}

	/************************************************************************************************************************
	 * Class：	SearchFileListView
	 * Purpose:	List control that stores file search results
	 *
	/***********************************************************************************************************************/
	SearchFileListView::SearchFileListView(QWidget* parent) : DefaultFileListView(parent) {
		mFileSearchModel = new SearchFileListViewModel;
		setModel(mFileSearchModel);
		mFileSearchItemDelegate = new SearchFileListViewItemDelegate(this);
		setItemDelegate(mFileSearchItemDelegate);

		mOpenFileFolderAction = new QAction(QIcon(":/cfm/resource/images/folder-parent.svg"), QObject::tr("Open directory hold this file"));
		connect(mOpenFileFolderAction, &QAction::triggered, this, &SearchFileListView::OnOpenFileFolder);

		connect(this, &QListView::customContextMenuRequested, this, &SearchFileListView::OnCustomContextMenuRequested);
	}

	QStringList SearchFileListView::SelectedFiles() {
		QList<QString> files;
		QItemSelectionModel* m = selectionModel();
		QModelIndexList selection = m->selectedIndexes();
		for (const QModelIndex& index : selection) {
			if (index.column() != 0)
				continue;
			QString file = index.data().toString();
			files << file;
		}
		return files;
	}

	QString SearchFileListView::CurrentDir() {
		return QString();
	}

	QModelIndex SearchFileListView::IndexOf(const QString& file) {
		return mFileSearchModel->IndexOf(file);
	}

	void SearchFileListView::AddItem(const QString& file) {
		mFileSearchModel->Append(file);
	}

	void SearchFileListView::RemoveAll() {
		mFileSearchModel->Clear();
	}

	void SearchFileListView::OnOpenFileFolder() {
		QModelIndex idx = currentIndex();
		if (!idx.isValid())
			return;
		QFileInfo file(idx.data().toString());
		MainWindow::Instance()->FileMainViewPtr()->Goto(file.absolutePath());
	}

	void SearchFileListView::OnCustomContextMenuRequested(const QPoint& pos) {

	}

	/************************************************************************************************************************
	 * Class：	FileSearchView
	 * Purpose:	File search view
	 *
	/***********************************************************************************************************************/
	FileSearchView::FileSearchView(QWidget *parent)
		: QWidget(parent) {
		SetupUi();
	}

	FileSearchView::~FileSearchView()
	{}

	void FileSearchView::SetupUi() {
		mSearchFileListView = new SearchFileListView;
		mSearchEdit = new QLineEdit;
		mSearchAction = new QAction(QIcon(":/ffx/res/image/search.svg"), "", mSearchFileListView);
		mSearchEdit->addAction(mSearchAction, QLineEdit::TrailingPosition);
		mSearchFileOnlyButton = new QToolButton;
		mSearchFileOnlyButton->setIcon(QIcon(":/ffx/res/image/search-file-only.svg"));
		mSearchCaseButton = new QToolButton;
		mSearchCaseButton->setIcon(QIcon(":/ffx/res/image/search-case-sen.svg"));
		mMainLayout = new QGridLayout;
		mMainLayout->addWidget(mSearchEdit, 0, 0, 1, 1);
		mMainLayout->addWidget(mSearchFileOnlyButton, 0, 1, 1, 1);
		mMainLayout->addWidget(mSearchCaseButton, 0, 2, 1, 1);
		mMainLayout->addWidget(mSearchFileListView, 1, 0, 1, 3);
		mMainLayout->setRowStretch(0, 1);
		mMainLayout->setColumnStretch(1, 1);
		mMainLayout->setMargin(0);
		setLayout(mMainLayout);
	}
}
