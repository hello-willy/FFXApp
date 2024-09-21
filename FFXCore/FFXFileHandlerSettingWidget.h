#pragma once
#include "FFXFileHandler.h"

#include <QWidget>

class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QToolButton;
class QGridLayout;
class QScrollArea;
class QCheckBox;

namespace FFX {
	class Argument;

	class ArgumentEditor : public QWidget {
		Q_OBJECT
	public:
		ArgumentEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value() = 0;
		virtual void Focus() = 0;

	protected:
		const Argument& mArgument;
		int mItemHeight = 37;
	};

	class NormalEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		NormalEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QLineEdit* mLineEdit;
	};

	class BoolEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		BoolEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QCheckBox* mCheckBox;
	};

	class DateEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		DateEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QDateEdit* mDateEdit;
	};

	class OptionEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		OptionEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QComboBox* mComboBox;
	};

	class NormalRangeEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		NormalRangeEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QLineEdit* mLineEdit1;
		QLineEdit* mLineEdit2;
	};

	class DateRangeEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		DateRangeEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QDateEdit* mDateEdit1;
		QDateEdit* mDateEdit2;
	};

	class RectEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		RectEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QLineEdit* mLineEditMinX;
		QLineEdit* mLineEditMaxX;
		QLineEdit* mLineEditMinY;
		QLineEdit* mLineEditMaxY;
	};

	class SizeEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		SizeEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private:
		QHBoxLayout* mMainLayout;
		QLineEdit* mLineEdit1;
		QLineEdit* mLineEdit2;
	};

	class FileEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		FileEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private slots:
		void OnBrowseFile();

	private:
		QHBoxLayout* mMainLayout;
		QLineEdit* mPathEdit;
		QToolButton* mBrowseButton;
	};

	class DirEditor : public ArgumentEditor {
		Q_OBJECT
	public:
		DirEditor(const Argument& arg, QWidget* parent = nullptr);
	public:
		virtual QVariant Value();
		virtual void Focus();
	private slots:
		void OnBrowseFile();

	private:
		QHBoxLayout* mMainLayout;
		QLineEdit* mPathEdit;
		QToolButton* mBrowseButton;
	};

	class FFXCORE_EXPORT ArgumentCollectorListWidget : public QWidget {
		Q_OBJECT
	public:
		ArgumentCollectorListWidget(QWidget* parent = nullptr);

	public:
		void Add(const Argument& arg);
		void GetArgumentMap(ArgumentMap& argmap) const;
		void FocusAt(const QString& arg);

	protected:
		virtual void paintEvent(QPaintEvent* event) override;

	private:
		ArgumentEditor* CreateEditor(const Argument& arg);

	private:
		void SetupUi();

	private:
		QGridLayout* mMainLayout;
		QMap<QString, ArgumentEditor*> mArgumentWidgetMap;
	};

}
