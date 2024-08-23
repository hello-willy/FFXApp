#pragma once

#include <QWidget>
#include <QFrame>
#include <QToolButton>
#include <QTreeView>
#include <QFileSystemModel>
#include <QPair>

class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QShortcut;

namespace FFX {
	typedef QPair<QString, QVariant> QuickItem;
	typedef QList<QuickItem> QuickItemList;

	class FileQuickViewHeader : public QWidget {
		Q_OBJECT
	public:
		FileQuickViewHeader(QWidget* parent = nullptr);

	public:
		void AddAction(QAction* action);

	private:
		void SetupUi();

	private:
		QLabel* mHeaderLabel;
		QList<QAction*> mActions;
		QHBoxLayout* mMainLayout;
	};

	class QuickNavigatePanel : public QWidget {
		Q_OBJECT
	public:
		QuickNavigatePanel(QWidget* parent = 0);

	public:
		void AddItem(const QString& dir, QString name = QString());
		void AddItem(const QuickItemList& items);
		bool IsDirFixed(const QString& dir);
		void RemoveItem(const QString& dir);
		bool IsFull() const;
		QuickItemList Items() const;

	private:
		void SetupUi();
		void InitShortcuts();

	private slots:
		void OnCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
		void OnCustomContextMenuRequested(const QPoint& pos);
		void OnRemoveItem();

	private:
		FileQuickViewHeader* mHeader;
		QListWidget* mItemList;
		QVBoxLayout* mMainLayout;
		QList<QShortcut*> mQuickNaviShortcuts;
		QAction* mRemoveItemAction;
		int mMaxItems = 9;

	Q_SIGNALS:
		void RootPathChanged(const QFileInfo& file);
	};

	class FileTreeViewModel : public QFileSystemModel
	{
		Q_OBJECT
	public:
		FileTreeViewModel(QObject* parent = nullptr) : QFileSystemModel(parent) {}
		//! for hide the other columns.
		int columnCount(const QModelIndex& parent) const override {
			return 1;
		}
	};

	class FileTreeNavigatePanel : public QTreeView {
		Q_OBJECT
	public:
		FileTreeNavigatePanel(QWidget* parent = nullptr);

	protected:
		void mousePressEvent(QMouseEvent* event) override;

	private:
		void SetupUi();

	private slots:
		void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

	private:
		FileTreeViewModel* mFileSystemModel;

	Q_SIGNALS:
		void RootPathChanged(const QFileInfo& file);
	};

	class FileQuickView : public QWidget {
		Q_OBJECT

	public:
		FileQuickView(QWidget* parent = nullptr);
		~FileQuickView();

	public:
		QuickNavigatePanel* QuickNaviPanelPtr() { return mQuickNaviPanel; }
		FileTreeNavigatePanel* FileTreeNaviPanelPtr() { return mFileTreeNavigatePanel; }

	protected:
		virtual void paintEvent(QPaintEvent* event) override;

	private:
		void SetupUi();

	private:
		QuickNavigatePanel* mQuickNaviPanel;
		FileTreeNavigatePanel* mFileTreeNavigatePanel;
		QVBoxLayout* mMainLayout;
	};

}
