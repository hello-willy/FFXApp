#pragma once
#include "FFXCommonFileListView.h"

#include <QWidget>

class QGridLayout;
class QLabel;
class QMenu;

namespace FFX {
	class ClipboardPanelHeader : public QWidget {
		Q_OBJECT
	public:
		ClipboardPanelHeader(QWidget* parent = nullptr);

	public:
		void AddWidget(QWidget* widget);
		void AddAction(QAction* action);
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

	class ClipboardPanel : public QWidget
	{
		Q_OBJECT

	public:
		ClipboardPanel(QWidget* parent = nullptr);
		~ClipboardPanel();

	public:
		ClipboardPanelHeader* Header();

	private:
		void SetupUi();

	private slots:
		void OnClipboardDataChanged();
		void OnClear();
		void OnRemoveSelection();

	private:
		ClipboardPanelHeader* mClipboardPanelHeader;
		QGridLayout* mMainLayout;
		CommonFileListView* mItemListView;
		QToolButton* mClearButton;
		QToolButton* mRemoveSelectionButton;
		QToolButton* mGotoParentDirButton;
		QAction* mRemoveSelectionAction;
	};
}

