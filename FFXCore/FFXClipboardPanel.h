#pragma once
#include "FFXCommonFileListView.h"

#include <QWidget>
class QGridLayout;
namespace FFX {
	class ClipboardPanel : public QWidget
	{
		Q_OBJECT

	public:
		ClipboardPanel(QWidget* parent = nullptr);
		~ClipboardPanel();

	private:
		void SetupUi();

	private slots:
		void OnClipboardDataChanged();
		void OnClear();
		void OnRemoveSelection();

	private:
		QGridLayout* mMainLayout;
		CommonFileListView* mItemListView;
		QToolButton* mClearButton;
		QAction* mRemoveSelection;
	};
}

