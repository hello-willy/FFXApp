#pragma once
#include "FFXFileListView.h"

#include <QAbstractListModel>


namespace FFX {
	class CommonFileListViewModel : public QAbstractListModel {
		Q_OBJECT
	public:
		explicit CommonFileListViewModel(QObject* parent = 0);
		friend class CommonFileListView;

	public:
		int rowCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;

	public:
		void Append(const QString& file);
		void Append(const QStringList& files);
		void Clear();
		void RemoveRow(int row);
		QModelIndex IndexOf(const QString& file);
		int Count();
		QStringList AllItems() const;

	private:
		QStringList mListFileLoaded;
	};

	class CommonFileListViewItemDelegate : public QStyledItemDelegate {
	public:
		CommonFileListViewItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	};

	class CommonFileListView : public DefaultFileListView {
		Q_OBJECT

	public:
		CommonFileListView(QWidget* parent = nullptr);
		~CommonFileListView();

	public:
		virtual QStringList AllFiles();
		virtual QStringList SelectedFiles() override;
		virtual QString CurrentDir() override;
		virtual QModelIndex IndexOf(const QString& file) override;

	public:
		QStringList AllRow();
		void RemoveRow(int row);
		void AddItem(const QString& file);
		void AddItems(const QStringList& files);
		void RemoveAll();
		void AddAction(const QString& name, QAction* action);
		QAction* Action(const QString& name);
		int Count();
	Q_SIGNALS:
		void itemChanged();

	private slots:
		virtual void OnItemDoubleClicked(const QModelIndex& index) {}
		virtual void OnCustomContextMenuRequested(const QPoint& pos) override;
		void OnGotoParentDir();

	private:
		CommonFileListViewModel* mViewModel;
		CommonFileListViewItemDelegate* mItemDelegate;
		QMap<QString, QAction*> mActionMap;
		QAction* mGotoFileParentDirAction;
	};
}

