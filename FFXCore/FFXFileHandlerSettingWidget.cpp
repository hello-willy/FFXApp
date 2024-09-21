#include "FFXFileHandlerSettingWidget.h"


#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QRegExpValidator>
#include <QDateEdit>
#include <QToolButton>
#include <QFileDialog>
#include <QGridLayout>
#include <QScrollArea>
#include <QPainter>
#include <QCheckBox>

namespace FFX {
	ArgumentEditor::ArgumentEditor(const Argument& arg, QWidget* parent)
		: QWidget(parent)
		, mArgument(arg) {

	}

	NormalEditor::NormalEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mLineEdit = new QLineEdit;
		mLineEdit->setFixedHeight(mItemHeight);
		mMainLayout->addWidget(mLineEdit, 1);
		mLineEdit->setText(arg.StringValue());
		setLayout(mMainLayout);

		QVariantList limits = arg.Limit();
		if (!limits.isEmpty() && !limits[0].isNull()) {
			QValidator* validator = new QRegExpValidator(QRegExp(limits[0].toString()));
			mLineEdit->setValidator(validator);
		}
	}

	QVariant NormalEditor::Value() {
		return mLineEdit->text();
	}

	BoolEditor::BoolEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mCheckBox = new QCheckBox;
		mCheckBox->setFixedHeight(mItemHeight);
		mMainLayout->addWidget(mCheckBox);
		mMainLayout->addStretch(1);
		setLayout(mMainLayout);
		mCheckBox->setChecked(arg.BoolValue());
	}

	QVariant BoolEditor::Value() {
		return mCheckBox->isChecked();
	}

	DateEditor::DateEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mDateEdit = new QDateEdit;
		mDateEdit->setFixedHeight(mItemHeight);
		mMainLayout->addWidget(mDateEdit);
		setLayout(mMainLayout);
	}

	QVariant DateEditor::Value() {
		return mDateEdit->date();
	}

	OptionEditor::OptionEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mComboBox = new QComboBox;
		mComboBox->setFixedHeight(mItemHeight);
		mMainLayout->addWidget(mComboBox);
		setLayout(mMainLayout);

		QVariantList limits = arg.Limit();
		for (QVariant v : limits) {
			mComboBox->addItem(v.toString(), v);
		}
	}

	QVariant OptionEditor::Value() {
		return mComboBox->currentData(Qt::UserRole);
	}

	NormalRangeEditor::NormalRangeEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mLineEdit1 = new QLineEdit;
		mLineEdit1->setFixedHeight(mItemHeight);
		mLineEdit2 = new QLineEdit;
		mLineEdit2->setFixedHeight(mItemHeight);
		mMainLayout->addWidget(mLineEdit1);
		mMainLayout->addWidget(mLineEdit2);
		setLayout(mMainLayout);

		QVariantList limits = arg.Limit();
		if (limits.size() == 2) {
			mLineEdit1->setValidator(new QRegExpValidator(QRegExp(limits[0].toString())));
			mLineEdit2->setValidator(new QRegExpValidator(QRegExp(limits[1].toString())));
		}
	}

	QVariant NormalRangeEditor::Value() {
		QVariantList ret;
		ret << mLineEdit1->text() << mLineEdit2->text();
		return ret;
	}

	DateRangeEditor::DateRangeEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mDateEdit1 = new QDateEdit;
		mDateEdit1->setFixedHeight(mItemHeight);
		mDateEdit2 = new QDateEdit;
		mDateEdit2->setFixedHeight(mItemHeight);
		mMainLayout->addWidget(mDateEdit1);
		mMainLayout->addWidget(mDateEdit2);
		setLayout(mMainLayout);
	}

	QVariant DateRangeEditor::Value() {
		QVariantList ret;
		ret << mDateEdit1->date() << mDateEdit2->date();
		return ret;
	}

	RectEditor::RectEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		QRect rect = arg.RectValue();
		mLineEditMinX = new QLineEdit;
		mLineEditMaxX = new QLineEdit;
		mLineEditMinY = new QLineEdit;
		mLineEditMaxY = new QLineEdit;

		mLineEditMinX->setFixedHeight(mItemHeight);
		mLineEditMinX->setText(QString("%1").arg(rect.left()));
		mLineEditMaxX->setFixedHeight(mItemHeight);
		mLineEditMaxX->setText(QString("%1").arg(rect.right()));
		mLineEditMinY->setFixedHeight(mItemHeight);
		mLineEditMinY->setText(QString("%1").arg(rect.top()));
		mLineEditMaxY->setFixedHeight(mItemHeight);
		mLineEditMaxY->setText(QString("%1").arg(rect.bottom()));

		mMainLayout->addWidget(mLineEditMinX);
		mMainLayout->addWidget(mLineEditMaxX);
		mMainLayout->addWidget(mLineEditMinY);
		mMainLayout->addWidget(mLineEditMaxY);
		setLayout(mMainLayout);
	}

	QVariant RectEditor::Value() {
		int minx = mLineEditMinX->text().toInt();
		int maxx = mLineEditMaxX->text().toInt();
		int miny = mLineEditMinY->text().toInt();
		int maxy = mLineEditMaxY->text().toInt();
		return QRect(minx, miny, maxx - minx, maxy - miny);
	}

	SizeEditor::SizeEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		QSize size = arg.SizeValue();
		mLineEdit1 = new QLineEdit;
		mLineEdit1->setFixedHeight(mItemHeight);
		mLineEdit1->setText(QString("%1").arg(size.width()));
		mLineEdit2 = new QLineEdit;
		mLineEdit2->setFixedHeight(mItemHeight);
		mLineEdit2->setText(QString("%1").arg(size.height()));
		mMainLayout->addWidget(mLineEdit1);
		mMainLayout->addWidget(mLineEdit2);
		setLayout(mMainLayout);
	}

	QVariant SizeEditor::Value() {
		int w = mLineEdit1->text().toInt();
		int h = mLineEdit2->text().toInt();
		return QSize(w, h);
	}

	FileEditor::FileEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mPathEdit = new QLineEdit;
		mPathEdit->setFixedHeight(mItemHeight);
		mPathEdit->setText(arg.StringValue());
		mBrowseButton = new QToolButton;
		mBrowseButton->setIcon(QIcon(":/ffx/res/image/file-txt.svg"));
		mMainLayout->addWidget(mPathEdit);
		mMainLayout->addWidget(mBrowseButton);
		setLayout(mMainLayout);
		connect(mBrowseButton, &QToolButton::clicked, this, &FileEditor::OnBrowseFile);
	}

	QVariant FileEditor::Value() {
		return mPathEdit->text();
	}

	void FileEditor::OnBrowseFile() {
		QString filter("All Files (*)");
		QVariantList limits = mArgument.Limit();
		if (!limits.isEmpty()) {
			filter = limits[0].toString();
		}
		QString file;
		if (mArgument.GetType() == Argument::File)
			file = QFileDialog::getOpenFileName(nullptr, QObject::tr("Select File"), QDir::homePath(), filter);
		else if (mArgument.GetType() == Argument::SaveFile)
			file = QFileDialog::getSaveFileName(nullptr, QObject::tr("Save File"), QDir::homePath(), filter);
		
		mPathEdit->setText(file);
	}

	DirEditor::DirEditor(const Argument& arg, QWidget* parent)
		: ArgumentEditor(arg, parent) {
		mMainLayout = new QHBoxLayout;
		mMainLayout->setMargin(0);
		mPathEdit = new QLineEdit;
		mBrowseButton = new QToolButton;
		mBrowseButton->setIcon(QIcon(":/ffx/res/image/folder-open.svg"));
		QSize size = mBrowseButton->sizeHint();
		mPathEdit->setFixedHeight(size.height());
		mPathEdit->setText(arg.StringValue());
		mMainLayout->addWidget(mPathEdit);
		mMainLayout->addWidget(mBrowseButton);
		setLayout(mMainLayout);
		connect(mBrowseButton, &QToolButton::clicked, this, &DirEditor::OnBrowseFile);
	}

	QVariant DirEditor::Value() {
		return mPathEdit->text();
	}

	void DirEditor::OnBrowseFile() {
		QString dir = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Select output directory"), QDir::homePath());
		mPathEdit->setText(dir);
	}

	ArgumentCollectorListWidget::ArgumentCollectorListWidget(QWidget* parent)
		: QWidget(parent) {
		SetupUi();
	}

	void ArgumentCollectorListWidget::paintEvent(QPaintEvent* event) {
		/*
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		QPen borderPen(Qt::black);
		borderPen.setWidth(1);
		painter.setPen(borderPen);
		painter.drawRect(rect());
		*/
	}

	void ArgumentCollectorListWidget::Add(const Argument& arg) {
		QLabel* title = new QLabel(QString("%1%2").arg(arg.DisplayName()).arg(arg.IsRequired() ? "*" : ""));
		ArgumentEditor* editor = CreateEditor(arg);
		mMainLayout->addWidget(title, mArgumentWidgetMap.size(), 0, 1, 1);
		title->setToolTip(arg.Description());
		mMainLayout->addWidget(editor, mArgumentWidgetMap.size(), 1, 1, 1);
		mArgumentWidgetMap[arg.Name()] = editor;
	}

	void ArgumentCollectorListWidget::SetupUi() {
		mMainLayout = new QGridLayout;
		mMainLayout->setMargin(9);
		mMainLayout->setAlignment(Qt::AlignTop);
		setLayout(mMainLayout);
	}

	ArgumentEditor* ArgumentCollectorListWidget::CreateEditor(const Argument& arg) {
		switch (arg.GetType()) {
		case Argument::Normal:
			return new NormalEditor(arg);
		case Argument::Bool:
			return new BoolEditor(arg);
		case Argument::Option:
			return new OptionEditor(arg);
		case Argument::Date:
			return new DateEditor(arg);
		case Argument::NormalRange:
			return new NormalRangeEditor(arg);
		case Argument::DateRange:
			return new DateRangeEditor(arg);
		case Argument::Rect:
			return new RectEditor(arg);
		case Argument::Size:
			return new SizeEditor(arg);
		case Argument::File:
		case Argument::SaveFile:
			return new FileEditor(arg);
		case Argument::Dir:
			return new DirEditor(arg);
		}
		return new NormalEditor(arg);
	}

	void ArgumentCollectorListWidget::GetArgumentMap(ArgumentMap& argmap) const {
		QMap<QString, ArgumentEditor*>::const_iterator it = mArgumentWidgetMap.begin();
		for (; it != mArgumentWidgetMap.end(); it++) {
			argmap[it.key()].SetValue(it.value()->Value());
		}
	}
}
