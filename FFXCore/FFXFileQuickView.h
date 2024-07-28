#pragma once

#include <QWidget>
#include <QFrame>
#include <QToolButton>

class QVBoxLayout;
class QLabel;

namespace FFX {
	
	class ActionLabel : public QToolButton {
		Q_OBJECT

	public:
		/** Constructor.
		  */
		explicit ActionLabel(QWidget* parent = 0);

		/** Constructor. Creates ActionLabel from the action.
		  \since 0.2
		  */
		explicit ActionLabel(QAction* action, QWidget* parent = 0);

		virtual ~ActionLabel() {}

		virtual QSize sizeHint() const override;
		virtual QSize minimumSizeHint() const override;

	protected:
		void Init();
	};

	class ActionBox : public QFrame {
	public:
		ActionBox(const QPixmap& icon, const QString& headerText, QWidget* parent = 0);
		void SetIcon(const QPixmap& icon);
		inline const QPixmap* Icon() const;
		inline ActionLabel* Header() const;
		ActionLabel* CreateItem(const QString& text, QAction* action = nullptr);

	private:
		void Init();

	private:
		QVBoxLayout* mMainLayout;
		QLabel* mIconLabel;
		ActionLabel* mHeaderLabel;
	};

	class FileQuickView : public QWidget {
		Q_OBJECT

	public:
		FileQuickView(QWidget* parent = nullptr);
		~FileQuickView();
	};

}
