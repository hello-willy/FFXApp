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
#include <QShortcut>
#include <QMenu>
#include <QDebug>

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
        mHeaderLabel = new QLabel(QObject::tr("Quick Panel"));
        mHeaderLabel->setFixedHeight(32);
        mMainLayout = new QHBoxLayout;
        mMainLayout->setContentsMargins(0, 9, 0, 0);

        mMainLayout->addWidget(mHeaderLabel, 1);
        setLayout(mMainLayout);
    }

    void FileQuickViewHeader::paintEvent(QPaintEvent* event) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect().marginsAdded(QMargins(0, -9, 0, 0)), QColor("#EAEAEA"));
        //QPen borderPen(Qt::black);
        //borderPen.setWidth(1);
        //painter.setPen(borderPen);
        //painter.drawRect(rect());
    }

    QuickNavigatePanel::QuickNavigatePanel(QWidget* parent)
        : QWidget(parent) {
        SetupUi();
    }

    void QuickNavigatePanel::SetupUi() {
        mHeader = new FileQuickViewHeader;
        mMainLayout = new QVBoxLayout;
        mItemList = new QListWidget;
        mRemoveItemAction = new QAction(QIcon(":/ffx/res/image/unpin.svg"), "Cancel from quick panel");
        InitShortcuts();

        //! Init list widget
        //mItemList->setStyleSheet("QListView { border: transparent; }"); // set the list no border
        mItemList->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed); // Set edit mode.
        mItemList->setContextMenuPolicy(Qt::CustomContextMenu);

        mMainLayout->setContentsMargins(0, 0, 0, 0);
        mMainLayout->addWidget(mHeader);
        mMainLayout->addWidget(mItemList, 1);

        setLayout(mMainLayout);

        connect(mItemList, &QListWidget::currentItemChanged, this, &QuickNavigatePanel::OnCurrentItemChanged);

        connect(mItemList, &QListView::customContextMenuRequested, this, &QuickNavigatePanel::OnCustomContextMenuRequested);
        connect(mRemoveItemAction, &QAction::triggered, this, &QuickNavigatePanel::OnRemoveItem);
    }

    void QuickNavigatePanel::AddItem(const QString& dir, QString name) {
        QFileInfo fileInfo(dir);
        if (!fileInfo.exists(dir) || !fileInfo.isDir())
            return;

        QFileIconProvider fip;
        QIcon icon = fip.icon(dir);
        QString displayName = name;
        if (displayName.isEmpty())
            displayName = fileInfo.isRoot() ? fileInfo.absoluteFilePath() : fileInfo.fileName();
        QListWidgetItem* item = new QListWidgetItem(icon, displayName);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
        item->setData(Qt::UserRole, dir);
        mItemList->addItem(item);
    }

    void QuickNavigatePanel::AddItem(const QuickItemList& items) {
        for (const QuickItem& item : items) {
            AddItem(item.second.toString(), item.first);
        }
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
        return mMaxItems <= mItemList->count();
    }

    QuickItemList QuickNavigatePanel::Items() const {
        QuickItemList ret;
        int count = mItemList->count();
        for (int i = 0; i < count; i++) {
            QListWidgetItem* item = mItemList->item(i);
            if (item == nullptr)
                continue;
            ret << QuickItem(item->text(), item->data(Qt::UserRole));
        }
        return ret;
    }

    void QuickNavigatePanel::OnCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) {
        //! This slot will be triggered at system startup, SO ADD the previous == nullptr
        if (current == nullptr || previous == nullptr)
            return;
        QString path = current->data(Qt::UserRole).toString();
        if (!mFreezeSignal) {
            emit RootPathChanged(path);
        } else {
            qDebug() << "Freeze Signal";
        }
    }

    void QuickNavigatePanel::InitShortcuts() {
        for (int i = 0; i < mMaxItems; i++) {
            QShortcut* shortcut = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)), this);
            shortcut->setContext(Qt::ApplicationShortcut);
            connect(shortcut, &QShortcut::activated, this, [=]() {
                QListWidgetItem* item = mItemList->item(i);
                if (item == nullptr)
                    return;
                mItemList->setCurrentRow(i);
                });
        }
    }

    void QuickNavigatePanel::OnCustomContextMenuRequested(const QPoint& pos) {
        QMenu* menu = new QMenu(this);
        menu->addAction(mRemoveItemAction);
        menu->exec(QCursor::pos());
        delete menu;
    }

    void QuickNavigatePanel::OnRemoveItem() {
        int row = mItemList->currentRow();
        if (row >= 0) {
            delete mItemList->takeItem(row);
        }
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

    /*
    void FileQuickView::paintEvent(QPaintEvent* event) {
        
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen borderPen(Qt::black);
        borderPen.setWidth(1);
        painter.setPen(borderPen);
        painter.drawRect(rect());
        
    }
    */
    void FileQuickView::SetupUi() {
        mQuickNaviPanel = new QuickNavigatePanel;
        mFileTreeNavigatePanel = new FileTreeNavigatePanel;
        mMainLayout = new QVBoxLayout;

        mMainLayout->setContentsMargins(0, 0, 0, 0);
        mMainLayout->addWidget(mQuickNaviPanel, 2);
        //QFrame* line = new QFrame;
        //line->setFrameShape(QFrame::HLine);
        //line->setFrameShadow(QFrame::Sunken);
        //mMainLayout->addWidget(line);
        mMainLayout->addWidget(mFileTreeNavigatePanel, 5);
        //mMainLayout->setSpacing(9);
        setLayout(mMainLayout);

        //! Init item list, fill the drivers.
        //QFileInfoList drivers = QDir::drives();
        //for (const QFileInfo& driver : drivers) {
        //    mQuickNaviPanel->AddItem(driver.absoluteFilePath());
        //}
    }
}
