#pragma once
#include "FFXCommonFileListView.h"

#include <QWidget>

class QGridLayout;
class QLabel;
class QMenu;

namespace FFX {
	class FFXCORE_EXPORT ClipboardPanelHeader : public QWidget {
		Q_OBJECT
	public:
		ClipboardPanelHeader(QWidget* parent = nullptr);

	public:
		void AddWidget(QWidget* widget);
		void AddAction(QAction* action);
		QMenu* AddMenuAction(const QString& name);
		void RemoveMenu(const QMenu* menu);
		void AddSeperator();

	protected:
		virtual void paintEvent(QPaintEvent* event) override;

	private:
		void SetupUi();

	private:
		QLabel* mHeaderLabel;
		QToolButton* mOperatorButton;
		QMenu* mOperatorMenu;
		QHBoxLayout* mMainLayout;
	};

	class FFXCORE_EXPORT ClipboardPanel : public QWidget {
		Q_OBJECT

	public:
		ClipboardPanel(QWidget* parent = nullptr);
		~ClipboardPanel();

	public:
		ClipboardPanelHeader* Header();
		QStringList AllFiles();

	private:
		void SetupUi();

	private slots:
		void OnClipboardDataChanged();
		void OnClear();
		void OnTaskComplete(int taskId, bool success);
		void OnTaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message);
		void OnRemoveSelection();
		void OnCopyFileTo();
		void OnMoveFileTo();

	private:
		ClipboardPanelHeader* mClipboardPanelHeader;
		QGridLayout* mMainLayout;
		CommonFileListView* mItemListView;
		QToolButton* mClearButton;
		QToolButton* mRemoveSelectionButton;
		QToolButton* mGotoParentDirButton;
		QAction* mRemoveSelectionAction;
		QAction* mCopyFileToAction;
		QAction* mMoveFileToAction;
	};
}

