#include "FFXCommonFileListView.h"
#include <QPainter>
#include <QFileIconProvider>
#include <QMenu>

namespace FFX {
	/************************************************************************************************************************
	 * Class：	CommonFileListViewModel
	 * Purpose:	The model for storing search results
	 *
	/************************************************************************************************************************/
	CommonFileListViewModel::CommonFileListViewModel(QObject* parent)
		: QAbstractListModel(parent) {

	}

	int CommonFileListViewModel::rowCount(const QModelIndex& parent) const {
		return mListFileLoaded.size();
	}

	QVariant CommonFileListViewModel::data(const QModelIndex& index, int role) const {
		if (role == Qt::DisplayRole)
			return mListFileLoaded[index.row()];
		return QVariant();
	}

	void CommonFileListViewModel::Append(const QString& file) {
		beginResetModel();
		mListFileLoaded << file;
		endResetModel();
	}

	void CommonFileListViewModel::Clear() {
		beginResetModel();
		mListFileLoaded.clear();
		endResetModel();
	}

	QModelIndex CommonFileListViewModel::IndexOf(const QString& file) {
		int row = mListFileLoaded.indexOf(file);
		return QAbstractListModel::index(row);
	}

	void CommonFileListViewModel::RemoveRow(int row) {
		beginResetModel();
		mListFileLoaded.removeAt(row);
		endResetModel();
	}

	void CommonFileListViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
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

	QSize CommonFileListViewItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
		Q_UNUSED(index)
		return QSize(option.rect.width(), 80); // Set the item height fixed.
	}

	CommonFileListView::CommonFileListView(QWidget* parent)
		: DefaultFileListView(parent) {
		mViewModel = new CommonFileListViewModel;
		setModel(mViewModel);
		mItemDelegate = new CommonFileListViewItemDelegate(this);
		setItemDelegate(mItemDelegate);
		setContextMenuPolicy(Qt::CustomContextMenu);

		//connect(this, &QListView::customContextMenuRequested, this, &CommonFileListView::OnCustomContextMenuRequested);
	}

	CommonFileListView::~CommonFileListView()
	{}

	QStringList CommonFileListView::SelectedFiles() {
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

	QString CommonFileListView::CurrentDir() {
		return QString();
	}

	QModelIndex CommonFileListView::IndexOf(const QString& file) {
		return mViewModel->IndexOf(file);
	}

	QStringList CommonFileListView::AllRow() {
		return mViewModel->mListFileLoaded;
	}

	void CommonFileListView::RemoveRow(int row) {
		mViewModel->RemoveRow(row);
	}

	void CommonFileListView::AddItem(const QString& file) {
		mViewModel->Append(file);
	}

	void CommonFileListView::RemoveAll() {
		mViewModel->Clear();
	}

	void CommonFileListView::AddAction(const QString& name, QAction* action) {
		mActionMap[name] = action;
	}

	QAction* CommonFileListView::Action(const QString& name) {
		return mActionMap[name];
	}

	void CommonFileListView::OnCustomContextMenuRequested(const QPoint& pos) {
		QMenu* menu = new QMenu(this);
		menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
		menu->setAttribute(Qt::WA_TranslucentBackground);
		menu->setMinimumWidth(200);

		QMap<QString, QAction*>::iterator it = mActionMap.begin();
		for (; it != mActionMap.end(); it++) {
			menu->addAction(it.value());
		}
		menu->exec(QCursor::pos());
		delete menu;
	}
}
