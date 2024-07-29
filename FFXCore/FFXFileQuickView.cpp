#include "FFXFileQuickView.h"

#include <QStyleOptionToolButton>
#include <QApplication>
#include <QLabel>
#include <QHBoxLayout>
#include <QListWidget>
#include <QFileIconProvider>
#include <QMouseEvent>
#include <QPainter>
#include <QSizePolicy>

namespace FFX {

    FileQuickViewHeader::FileQuickViewHeader(QWidget* parent) 
        : QWidget(parent) {
        SetupUi();
    }

    void FileQuickViewHeader::AddAction(QAction* action) {
        QToolButton* bn = new QToolButton;
        bn->setDefaultAction(action);
        mMainLayout->addWidget(bn);
    }

    void FileQuickViewHeader::SetupUi() {
        mHeaderLabel = new QLabel;
        mMainLayout = new QHBoxLayout;
        mMainLayout->setMargin(0);

        mMainLayout->addWidget(mHeaderLabel, 1);
    }

    QuickNavigatePanel::QuickNavigatePanel(QWidget* parent)
        : QWidget(parent) {
        SetupUi();
    }

    void QuickNavigatePanel::SetupUi() {
        mHeader = new FileQuickViewHeader;
        mMainLayout = new QVBoxLayout;
        mItemList = new QListWidget;

        //! Init list widget
        mItemList->setStyleSheet("QListView { border: transparent; }"); // set the list no border

        mMainLayout->setContentsMargins(9, 0, 0, 0);
        mMainLayout->addWidget(mHeader);
        mMainLayout->addWidget(mItemList, 1);

        setLayout(mMainLayout);

        connect(mItemList, &QListWidget::currentItemChanged, this, &QuickNavigatePanel::OnCurrentItemChanged);
    }

    void QuickNavigatePanel::AddItem(const QString& dir) {
        QFileInfo fileInfo(dir);
        if (!fileInfo.exists(dir) || !fileInfo.isDir())
            return;

        QFileIconProvider fip;
        QIcon icon = fip.icon(dir);
        QListWidgetItem* item = new QListWidgetItem(icon, fileInfo.isRoot() ? fileInfo.absoluteFilePath() : fileInfo.fileName());
        item->setData(Qt::UserRole, dir);
        mItemList->addItem(item);
    }

    bool QuickNavigatePanel::IsDirFixed(const QString& dir) {
        int count = mItemList->count();
        for (int i = 0; i < count; i++) {
            QListWidgetItem* item = mItemList->item(i);
            QString path = item->data(Qt::UserRole).toString();
            if (dir == path)
                return true;
        }

        return false;
    }

    void QuickNavigatePanel::RemoveItem(const QString& dir) {
        int count = mItemList->count();
        for (int i = 0; i < count; i++) {
            QListWidgetItem* item = mItemList->item(i);
            QString path = item->data(Qt::UserRole).toString();
            if (dir == path) {
                delete mItemList->takeItem(i);
                break;
            }
        }
    }

    bool QuickNavigatePanel::IsFull() const {
        return 8 <= mItemList->count();
    }

    void QuickNavigatePanel::OnCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
        QString path = current->data(Qt::UserRole).toString();
        emit RootPathChanged(path);
    }

    FileTreeNavigatePanel::FileTreeNavigatePanel(QWidget* parent)
        : QTreeView(parent) {
        SetupUi();
    }

    void FileTreeNavigatePanel::mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            QModelIndex cur = selectionModel()->currentIndex();
            QModelIndex idx = this->indexAt(event->pos());
            if (idx == cur) {
                onCurrentChanged(idx, cur);
            }
        }
        QTreeView::mousePressEvent(event);
    }

    void FileTreeNavigatePanel::SetupUi() {
        mFileSystemModel = new FileTreeViewModel(this);
        mFileSystemModel->setRootPath("");
        mFileSystemModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
        setModel(mFileSystemModel);

        connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &FileTreeNavigatePanel::onCurrentChanged);

        setHeaderHidden(true);
    }

    void FileTreeNavigatePanel::onCurrentChanged(const QModelIndex& current, const QModelIndex& previous) {
        QFileInfo currentFileInfo = mFileSystemModel->fileInfo(current);
        emit RootPathChanged(currentFileInfo);
    }

	FileQuickView::FileQuickView(QWidget* parent)
		: QWidget(parent) {
        SetupUi();
    }

	FileQuickView::~FileQuickView()
	{}

    void FileQuickView::paintEvent(QPaintEvent* event) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen borderPen(Qt::black);
        borderPen.setWidth(1);
        painter.setPen(borderPen);
        painter.drawRect(rect());
    }

    void FileQuickView::SetupUi() {
        mQuickNaviPanel = new QuickNavigatePanel;
        mFileTreeNavigatePanel = new FileTreeNavigatePanel;
        mMainLayout = new QVBoxLayout;

        mMainLayout->setContentsMargins(5, 0, 5, 1);
        mMainLayout->addWidget(mQuickNaviPanel, 2);
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        mMainLayout->addWidget(line);
        mMainLayout->addWidget(mFileTreeNavigatePanel, 3);
        mMainLayout->setSpacing(9);
        setLayout(mMainLayout);

        //! Init item list, fill the drivers.
        QFileInfoList drivers = QDir::drives();
        for (const QFileInfo& driver : drivers) {
            mQuickNaviPanel->AddItem(driver.absoluteFilePath());
        }
    }
}
