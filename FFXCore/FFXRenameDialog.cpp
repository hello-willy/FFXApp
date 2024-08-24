#include "FFXRenameDialog.h"
#include "FFXMainWindow.h"
#include "FFXTaskPanel.h"

#include <QPainter>
#include <QFileIconProvider>
#include <QGridLayout>
#include <QPainterPath>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QTime>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>
#include <QButtonGroup>
#include <QAction>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

namespace FFX {
	RenameFileListViewModel::RenameFileListViewModel(QObject* parent)
		: QAbstractListModel(parent) {

	}

	int RenameFileListViewModel::rowCount(const QModelIndex& parent) const {
		return mData.size();
	}

	QVariant RenameFileListViewModel::data(const QModelIndex& index, int role) const {
		QVariant ret;
		int row = index.row();

		RenameData data = mData[row];
		switch (role) {
		case Qt::UserRole + 1:
			ret = data.file;
			break;
		case Qt::UserRole + 2:
			ret = data.newFileName;
			break;
		default:
			break;
		}
		return ret;
	}

	bool RenameFileListViewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
		bool ret = false;
		int row = index.row();

		if (row >= mData.size() || (!index.isValid())) {
			return false;
		}
		RenameData data = mData[row];

		switch (role) {
		case Qt::UserRole + 1:
			data.file = value.toString();
			ret = true;
			break;
		case Qt::UserRole + 2:
			data.newFileName = value.toString();
			ret = true;
			break;
		default:
			break;
		}
		mData.replace(row, data);
		return ret;
	}

	void RenameFileListViewModel::Append(const QString& file, const QString& newFileName) {
		beginResetModel();
		QString name(newFileName);
		if (name.isEmpty()) {
			name = QFileInfo(file).fileName();
		}
		mData.push_back(RenameData(file, name));
		endResetModel();
	}

	void RenameFileListViewModel::Clear() {
		beginResetModel();
		mData.clear();
		endResetModel();
	}

	void RenameFileListViewModel::RemoveRow(int row) {
		beginResetModel();
		mData.removeAt(row);
		endResetModel();
	}

	void RenameFileListViewModel::UpdateData(QFileInfoList newFiles) {
		beginResetModel();
		int size = mData.size();
		for (int i = 0; i < size; i++) {
			mData[i].newFileName = newFiles[i].fileName();
		}
		endResetModel();
	}

	void RenameFileListViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		if (!index.isValid())
			return;

		painter->setRenderHint(QPainter::Antialiasing, true);
		

		QRect rect = option.rect;

		QRect bk(rect);
		bk.adjust(0, 1, 0, -1);
		if (option.state.testFlag(QStyle::State_MouseOver)) {
			painter->fillRect(bk, QColor("#E5F3FF"));
		}
		if (option.state.testFlag(QStyle::State_Selected)) {
			painter->fillRect(bk, QColor("#CCE8FF"));
		}

		painter->save();
		painter->setPen(QColor(212, 212, 212));
		painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

		QString file = index.data(Qt::UserRole + 1).toString();
		QString newFileName = index.data(Qt::UserRole + 2).toString();
		QFileInfo fileInfo(file);

		QFileIconProvider fip;
		QIcon icon = fip.icon(file);
		QPixmap pixmap = icon.pixmap(QSize(50, 50));
		QRect iconRect(rect.left() + 5, rect.top() + 5, 50, 50);
		painter->drawPixmap(iconRect, pixmap);
		
		painter->restore();

		QFont font("Microsoft YaHei", 9);
		painter->setFont(font);
		
		int margin = 5;

		//! draw old file name
		QFontMetrics fm(font);
		int oldTextWidth = fm.horizontalAdvance(fileInfo.fileName());
		int effectWidth = rect.width() - iconRect.width() - 30 - margin * 5;
		int oldTextPlanWidth = effectWidth * 0.4;
		QRect oldNameRect(iconRect.right() + 5, rect.top() + 5, oldTextWidth, 30);
		painter->drawText(oldNameRect, Qt::AlignVCenter | Qt::AlignLeft, fileInfo.fileName());
		painter->save();

		//! draw angle right
		QIcon angleRight(":/ffx/res/image/angle-right.svg");
		QRect angleRightRect(oldNameRect.right() + 5, rect.top() + 5, 30, 30);
		painter->drawPixmap(angleRightRect, angleRight.pixmap(QSize(30,30)));

		//! draw new file name
		font.setBold(true);
		painter->setPen(QColor(192, 64, 64));
		QRect newNameRect(angleRightRect.right() + 5, rect.top() + 5, effectWidth - oldNameRect.width(), 30);
		painter->drawText(newNameRect, Qt::AlignVCenter | Qt::AlignLeft, newFileName);

		painter->restore();
		painter->setFont(QFont("Microsoft YaHei", 7));
		
		QRect pathNameRect(iconRect.right() + 5, oldNameRect.bottom() + 5, rect.width() - iconRect.width() - 10, 20);
		painter->drawText(pathNameRect, Qt::AlignVCenter | Qt::AlignLeft, fileInfo.absolutePath());
	}

	QSize RenameFileListViewItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
		Q_UNUSED(index)
		return QSize(option.rect.width(), mItemHeight);
	}
	
	RenameFileListView::RenameFileListView(QWidget* parent)
		: QListView(parent) {
		mItemDelegate = new RenameFileListViewItemDelegate;
		mDataModel = new RenameFileListViewModel;
		setModel(mDataModel);
		setItemDelegate(mItemDelegate);
	}
	
	void RenameFileListView::AddFile(const QString& file, const QString& newFileName) {
		mDataModel->Append(file, newFileName);
	}

	void RenameFileListView::Apply(QFileInfoList newFiles) {
		mDataModel->UpdateData(newFiles);
	}

	void RenameFileListView::Clear() {
		mDataModel->Clear();
	}

	ExprLineEdit::ExprLineEdit(QWidget* parent) 
		: QLineEdit(parent) {
		setStyleSheet("border: 1px solid #999999; background-color: transparent;");
	}

	void ExprLineEdit::focusInEvent(QFocusEvent* e) {
		QLineEdit::focusInEvent(e);
		emit focused();
	}

	ExprListItemWidget::ExprListItemWidget(QListWidgetItem* listWidgetItem, QWidget* parent) 
		: ExprListItemWidget(listWidgetItem, "", "", QRegExp::Wildcard, Qt::CaseInsensitive, false, parent) {

	}

	ExprListItemWidget::ExprListItemWidget(QListWidgetItem* listWidgetItem, const QString& pattern, const QString& stringFill, QRegExp::PatternSyntax syntax,
		Qt::CaseSensitivity caseSensitive, bool suffixInclude, QWidget* parent) 
		: mListWidgetItem(listWidgetItem) {
		mFileNameValidator = new QRegExpValidator(QRegExp("^[^/\\\\:*?\"<>|]+$"));
		mRuleActivedCheckBox = new QCheckBox;
		mRuleActivedCheckBox->setChecked(true);
		mPatternEdit = new ExprLineEdit;
		mPatternEdit->setPlaceholderText(QObject::tr("Input the pattern"));

		mPatternEdit->setText(pattern);
		mStringFillEdit = new ExprLineEdit;
		mStringFillEdit->setValidator(mFileNameValidator);
		mStringFillEdit->setPlaceholderText(QObject::tr("Input the string to replace"));
		mStringFillEdit->setText(stringFill);

		mCaseSensitiveCheckBox = new QCheckBox;
		mCaseSensitiveCheckBox->setText(QObject::tr("Case Sensitive"));
		mCaseSensitiveCheckBox->setChecked(caseSensitive == Qt::CaseSensitive);

		mSuffixIncludeCheckBox = new QCheckBox;
		mSuffixIncludeCheckBox->setText(QObject::tr("Suffix Include"));
		mSuffixIncludeCheckBox->setChecked(false);

		mSyntaxCombo = new QComboBox;
		mSyntaxCombo->addItem(QObject::tr("RegExp"), QRegExp::RegExp);
		mSyntaxCombo->addItem(QObject::tr("Wildcard"), QRegExp::Wildcard);
		mSyntaxCombo->addItem(QObject::tr("FixedString"), QRegExp::FixedString);

		mSyntaxCombo->setCurrentIndex((int)syntax);

		mMainLayout = new QHBoxLayout;
		mMainLayout->setContentsMargins(5, 10, 5, 10);

		mMainLayout->addWidget(mRuleActivedCheckBox);
		mMainLayout->addWidget(mSyntaxCombo);
		mMainLayout->addWidget(mPatternEdit, 1);
		mMainLayout->addWidget(mStringFillEdit, 1);
		mMainLayout->addWidget(mCaseSensitiveCheckBox);
		mMainLayout->addWidget(mSuffixIncludeCheckBox);

		setLayout(mMainLayout);

		connect(mPatternEdit, &ExprLineEdit::focused, this, &ExprListItemWidget::OnItemWidgetFocused);
		connect(mStringFillEdit, &ExprLineEdit::focused, this, &ExprListItemWidget::OnItemWidgetFocused);
		connect(mPatternEdit, &ExprLineEdit::textChanged, this, &ExprListItemWidget::OnItemChanged);
		connect(mStringFillEdit, &ExprLineEdit::textChanged, this, &ExprListItemWidget::OnItemChanged);

		//connect(mRuleActivedCheckBox, &QCheckBox::clicked, this, &ExprListItemWidget::OnItemChanged);
		connect(mCaseSensitiveCheckBox, &QCheckBox::clicked, this, &ExprListItemWidget::OnItemChanged);
		connect(mSuffixIncludeCheckBox, &QCheckBox::clicked, this, &ExprListItemWidget::OnItemChanged);
		connect(mRuleActivedCheckBox, &QCheckBox::stateChanged, this, &ExprListItemWidget::OnItemChanged);
		// NOTICE: currentIndexChanged signal has two implementations, so we must explicitly specify one.
		connect(mSyntaxCombo, (void(QComboBox::*)(int)) & QComboBox::currentIndexChanged, this, &ExprListItemWidget::OnItemChanged);
	}

	ExprListItemWidget::~ExprListItemWidget() {
		delete mFileNameValidator;
	}

	void ExprListItemWidget::SetChecked(bool checked) {
		mRuleActivedCheckBox->setChecked(checked);
	}

	bool ExprListItemWidget::IsChecked() {
		return mRuleActivedCheckBox->isChecked();
	}

	void ExprListItemWidget::SetCaseSensitivity(bool caseSensitivity) {
		mCaseSensitiveCheckBox->setChecked(caseSensitivity);
	}

	Qt::CaseSensitivity ExprListItemWidget::IsCaseSensitivity() const {
		return mCaseSensitiveCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
	}

	bool ExprListItemWidget::IsSuffixInclude() const{
		return mSuffixIncludeCheckBox->isChecked();
	}

	QString ExprListItemWidget::Pattern() const {
		return mPatternEdit->text();
	}

	QString ExprListItemWidget::StringToReplace() const {
		return mStringFillEdit->text();
	}

	QRegExp::PatternSyntax ExprListItemWidget::Syntax() const {
		return (QRegExp::PatternSyntax)mSyntaxCombo->currentData().toInt();
	}

	void ExprListItemWidget::SetEnable(bool enable) {
		mRuleActivedCheckBox->setEnabled(enable);
		mPatternEdit->setEnabled(enable);
		mStringFillEdit->setEnabled(enable);
		mCaseSensitiveCheckBox->setEnabled(enable);
	}

	void ExprListItemWidget::paintEvent(QPaintEvent* event) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);
		QPen pen(QColor("#999999"));
		painter.setPen(pen);
		painter.drawRect(0, 0, width(), height());
	}

	void ExprListItemWidget::OnItemWidgetFocused() {
		QListWidget* listWidget = mListWidgetItem->listWidget();
		if (listWidget != nullptr) {
			listWidget->setCurrentItem(mListWidgetItem);
		}
	}

	void ExprListItemWidget::OnItemChanged() {
		OnItemWidgetFocused();
		//! Activate itemChanged(QListWidget) sigal, the text do not be same.
		mListWidgetItem->setData(Qt::UserRole, QTime::currentTime().toString("hh:mm:ss.zzz"));
	}

	FileHandlerPtr ExprListItemWidget::MakeReplaceHandler() const {
		return std::make_shared<FileNameReplaceByExpHandler>(Pattern(), StringToReplace(), Syntax(), 
			mCaseSensitiveCheckBox->isChecked(), IsSuffixInclude());
	}

	ExprListWidget::ExprListWidget(QWidget* parent) 
		: QListWidget(parent) {

	}

	void ExprListWidget::MoveRowUp() {
		int row = currentRow();
		if (row <= 0 || row >= count())
			return;
		MoveRowTo(row, row - 1);
	}

	void ExprListWidget::MoveRowDown() {
		int row = currentRow();
		if (row < 0 || row >= count() - 1)
			return;
		MoveRowTo(row, row + 1);
	}

	void ExprListWidget::MoveRowTop() {
		int row = currentRow();
		if (row <= 0 || row >= count())
			return;
		MoveRowTo(row, 0);
	}

	void ExprListWidget::MoveRowBottom() {
		int row = currentRow();
		if (row < 0 || row >= count() - 1)
			return;
		int targetRow = count();
		MoveRowTo(row, targetRow);
	}

	void ExprListWidget::AddRule(const QString & pattern, const QString & after, QRegExp::PatternSyntax syntax,
		Qt::CaseSensitivity caseSensitive, bool suffixInclude) {
		QListWidgetItem* newItem = new QListWidgetItem();
		ExprListItemWidget* itemWidget = new ExprListItemWidget(newItem, pattern, after, syntax, caseSensitive, suffixInclude);
		newItem->setSizeHint(itemWidget->sizeHint());
		addItem(newItem);
		setItemWidget(newItem, itemWidget);
	}

	ExprData ExprListWidget::RuleOf(int row) {
		QListWidgetItem* listItem = item(row);
		if (listItem == nullptr)
			return ExprData();
		ExprListItemWidget* itemWidget = static_cast<ExprListItemWidget*>(this->itemWidget(listItem));
		if (!itemWidget->IsChecked())
			return ExprData();
		return ExprData(itemWidget->Pattern(), itemWidget->StringToReplace(), itemWidget->Syntax(), itemWidget->IsCaseSensitivity(), itemWidget->IsSuffixInclude());
	}

	void ExprListWidget::SetEnable(bool enable) {
		int itemCount = count();
		for (int i = 0; i < itemCount; i++)
		{
			QListWidgetItem* listItem = item(i);
			ExprListItemWidget* itemWidget = static_cast<ExprListItemWidget*>(this->itemWidget(listItem));
			if (itemWidget == nullptr)
				continue;
			itemWidget->SetEnable(enable);
		}
	}

	void ExprListWidget::MoveRowTo(int row, int toRow) {
		//! cache rule info first.
		QListWidgetItem* item = currentItem();
		ExprListItemWidget* srcItemWidget = static_cast<ExprListItemWidget*>(itemWidget(item));
		bool actived = srcItemWidget->IsChecked();
		Qt::CaseSensitivity caseSensitivity = srcItemWidget->IsCaseSensitivity();
		QString pattern = srcItemWidget->Pattern();
		QString stringToFill = srcItemWidget->StringToReplace();
		QRegExp::PatternSyntax syntax = srcItemWidget->Syntax();
		bool suffixInclude = srcItemWidget->IsSuffixInclude();
		blockSignals(true);
		QListWidgetItem* srcItem = takeItem(row);
		blockSignals(false);
		insertItem(toRow, srcItem);
		ExprListItemWidget* newItemWidget = new ExprListItemWidget(srcItem, pattern, stringToFill, syntax, caseSensitivity, suffixInclude);
		newItemWidget->SetChecked(actived);
		setItemWidget(srcItem, newItemWidget);
		setCurrentItem(srcItem);

		emit ExprChanged();
	}

	FileHandlerPtr ExprListWidget::MakeRenameHandler() const {
		FFX::FileHandlerPtr handler = std::make_shared<FFX::PipeFileHandler>();

		int itemCount = count();
		for (int i = 0; i < itemCount; i++)
		{
			QListWidgetItem* listItem = item(i);
			ExprListItemWidget* itemWidget = static_cast<ExprListItemWidget*>(this->itemWidget(listItem));
			if (itemWidget == nullptr)
				continue;
			if (!itemWidget->IsChecked())
				continue;
			std::dynamic_pointer_cast<FFX::PipeFileHandler>(handler)->Append(itemWidget->MakeReplaceHandler());
		}

		return handler;
	}

	RenameDialog::RenameDialog(QList<QString> files, QWidget* parent)
		: QDialog(parent) 
		, mFiles(files) {
		SetupUi();
		int count = files.size();
		for (int i = 0; i < count; i++) {
			mRenameFileListView->AddFile(files[i]);
		}
	}

	RenameDialog::~RenameDialog() {

	}

	void RenameDialog::SetupUi() {
		//layout()->setContentsMargins(20, 10, 20, 10);
		setWindowTitle(QObject::tr("File Rename Pro"));
		resize(QSize(1024, 768));
		setWindowFlags(Qt::WindowCloseButtonHint);

		mRenameFileListTitleLabel = new QLabel(QObject::tr("File List"));
		mLoadFromClipboardButton = new QToolButton;
		mLoadFromClipboardButton->setIcon(QIcon(":/ffx/res/image/clipboard.svg"));

		mRenameFileInfoLabel = new QLabel;
		mRenameFileListView = new RenameFileListView;
		mMainLayout = new QGridLayout;
		mMainLayout->setContentsMargins(9, 9, 9, 9);
		mTabWidget = new QTabWidget();
		mExprListWidget = new ExprListWidget;
		mSettingWidget = new QWidget;
		mSettingWidgetLayout = new QVBoxLayout(mSettingWidget);
		mSettingWidgetLayout->setMargin(9);
		mSettingWidgetLayout->setSpacing(5);
		mControlLayout = new QHBoxLayout;
		mControlLayout->setContentsMargins(3, -1, 3, -1);
		mControlLabel = new QLabel(QObject::tr("Expr List"));
		mRuleMoveTop = new QToolButton;
		mRuleMoveTop->setIcon(QIcon(":/ffx/res/image/angle-top.svg"));
		mRuleMoveUp = new QToolButton;
		mRuleMoveUp->setIcon(QIcon(":/ffx/res/image/angle-up.svg"));
		mRuleMoveDown = new QToolButton;
		mRuleMoveDown->setIcon(QIcon(":/ffx/res/image/angle-down.svg"));
		mRuleMoveBottom = new QToolButton;
		mRuleMoveBottom->setIcon(QIcon(":/ffx/res/image/angle-bottom.svg"));
		mControlLine = new QFrame;
		mControlLine->setFrameShape(QFrame::VLine);
		mControlLine->setFrameShadow(QFrame::Sunken);
		mAddRuleButton = new QToolButton;
		mAddRuleButton->setPopupMode(QToolButton::MenuButtonPopup);
		mAddExprAction = new QAction(QIcon(":/ffx/res/image/plus.svg"), "");
		mAddExprMenu = new QMenu;
		mAddRuleButton->setMenu(mAddExprMenu);
		mAddRuleButton->setDefaultAction(mAddExprAction);

		mAddRemoveDigitalExprAction = new QAction(QObject::tr("Remove Digitals Expr"));
		mAddRemoveSpectialCharExprAction = new QAction(QObject::tr("Remove Spectial Char"));
		mAddRemoveChineseCharExprAction = new QAction(QObject::tr("Remove Chinese Char"));
		mAddExprMenu->addAction(mAddRemoveDigitalExprAction);
		mAddExprMenu->addAction(mAddRemoveSpectialCharExprAction);
		mAddExprMenu->addAction(mAddRemoveChineseCharExprAction);

		mAddRuleButton->setIcon(QIcon(":/ffx/res/image/plus.svg"));
		mRemoveRuleButton = new QToolButton;
		mRemoveRuleButton->setIcon(QIcon(":/ffx/res/image/delete.svg"));
		mMiscLayout = new QGridLayout;
		mMiscLayout->setColumnStretch(1, 1);
		mMiscLayout->setColumnStretch(2, 1);
		mMiscLayout->setColumnStretch(3, 1);
		mMiscLayout->setColumnStretch(4, 1);
		mMiscLayout->setHorizontalSpacing(6);
		mMiscLayout->setVerticalSpacing(12);

		mFootLayout = new QHBoxLayout;
		mCaseTransformCheckBox = new QCheckBox(QObject::tr("Case Transofrm:"));
		mCaseTransformCheckBox->setChecked(false);

		mFileLowerCheckBox = new QCheckBox(QObject::tr("Lower case"));
		mFileUpperCheckBox = new QCheckBox(QObject::tr("Upper case"));
		mCaseButtonGroup = new QButtonGroup;
		mCaseButtonGroup->setExclusive(true);
		mCaseButtonGroup->addButton(mFileLowerCheckBox);
		mCaseButtonGroup->addButton(mFileUpperCheckBox);

		mDuplicatesCheckBox = new QCheckBox(QObject::tr("File Duplicated:"));
		mDuplicatesCheckBox->setChecked(true);
		mDupFormatEdit = new QLineEdit;

		mDupSuffixCheckBox = new QCheckBox(QObject::tr("After name"));
		mDupSuffixCheckBox->setChecked(true);
		mDupPrefixCheckBox = new QCheckBox(QObject::tr("Before name"));
		mDupButtonGroup = new QButtonGroup;
		mDupButtonGroup->setExclusive(true);
		mDupButtonGroup->addButton(mDupSuffixCheckBox);
		mDupButtonGroup->addButton(mDupPrefixCheckBox);
		mFirstEffectCheckBox = new QCheckBox(QObject::tr("Ignore first file"));
		mFirstEffectCheckBox->setChecked(true);
		mOkButton = new QToolButton;
		mOkButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mOkButton->setText(QObject::tr("Ok"));
		mOkButton->setIcon(QIcon(":/ffx/res/image/ok.svg"));
		mCancelButton = new QToolButton;
		mCancelButton->setText(QObject::tr("Cancel"));
		mCancelButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mCancelButton->setIcon(QIcon(":/ffx/res/image/cancel.svg"));

		mMiscLayout->addWidget(mCaseTransformCheckBox, 0, 0, 1, 1);
		mMiscLayout->addWidget(mFileLowerCheckBox, 0, 1, 1, 1);
		mMiscLayout->addWidget(mFileUpperCheckBox, 0, 2, 1, 1);
		mMiscLayout->addWidget(mDuplicatesCheckBox, 1, 0, 1, 1);
		mMiscLayout->addWidget(mDupFormatEdit, 1, 1, 1, 1);
		mMiscLayout->addWidget(mDupSuffixCheckBox, 1, 2, 1, 1);
		mMiscLayout->addWidget(mDupPrefixCheckBox, 1, 3, 1, 1);
		mMiscLayout->addWidget(mFirstEffectCheckBox, 1, 4, 1, 1);

		mFootLayout->addStretch();
		mFootLayout->addWidget(mOkButton);
		mFootLayout->addWidget(mCancelButton);
		mControlLayout->addWidget(mControlLabel, 1);
		mControlLayout->addWidget(mRuleMoveTop);
		mControlLayout->addWidget(mRuleMoveUp);
		mControlLayout->addWidget(mRuleMoveDown);
		mControlLayout->addWidget(mRuleMoveBottom);
		mControlLayout->addWidget(mControlLine);
		mControlLayout->addWidget(mAddRuleButton);
		mControlLayout->addWidget(mRemoveRuleButton);

		mSettingWidgetLayout->addLayout(mControlLayout);
		mSettingWidgetLayout->addWidget(mExprListWidget, 1);
		mSettingWidgetLayout->addLayout(mMiscLayout);

		mTabWidget->addTab(mSettingWidget, QIcon(":/ffx/res/image/settings.svg"), QObject::tr("Settings"));
		
		setLayout(mMainLayout);
		mMainLayout->addWidget(mRenameFileListTitleLabel, 0, 0, 1, 1);
		mMainLayout->addWidget(mLoadFromClipboardButton, 0, 1, 1, 1);
		//mMainLayout->addWidget(mRenameFileInfoLabel, 0, 1, 1, 1);
		mMainLayout->addWidget(mRenameFileListView, 1, 0, 1, 2);
		mMainLayout->addWidget(mTabWidget, 2, 0, 1, 2);
		mMainLayout->addLayout(mFootLayout, 3, 0, 1, 2);

		connect(mAddExprAction, &QAction::triggered, this, &RenameDialog::OnAddRuleButtonClicked);
		connect(mAddRemoveDigitalExprAction, &QAction::triggered, this, &RenameDialog::OnRemoveDigital);
		connect(mAddRemoveSpectialCharExprAction, &QAction::triggered, this, &RenameDialog::OnRemoveSpecitalChar);
		connect(mAddRemoveChineseCharExprAction, &QAction::triggered, this, &RenameDialog::OnRemoveChineseChar);
		connect(mRemoveRuleButton, &QToolButton::clicked, this, &RenameDialog::OnRemoveRuleButtonClicked);
		connect(mExprListWidget, &ExprListWidget::itemChanged, this, &RenameDialog::OnRuleChanged);
		connect(mExprListWidget, &ExprListWidget::ExprChanged, this, &RenameDialog::OnRuleChanged);
		connect(mCaseTransformCheckBox, &QCheckBox::stateChanged, this, &RenameDialog::OnRuleChanged);
		connect(mCaseButtonGroup, &QButtonGroup::idClicked, this, &RenameDialog::OnRuleChanged);
		connect(mDuplicatesCheckBox, &QCheckBox::stateChanged, this, &RenameDialog::OnRuleChanged);
		connect(mDupButtonGroup, &QButtonGroup::idClicked, this, &RenameDialog::OnRuleChanged);
		connect(mDupFormatEdit, &QLineEdit::textChanged, this, &RenameDialog::OnRuleChanged);
		connect(mFirstEffectCheckBox, &QCheckBox::stateChanged, this, &RenameDialog::OnRuleChanged);
		connect(mOkButton, &QToolButton::clicked, this, &RenameDialog::OnOkClicked);
		connect(mCancelButton, &QToolButton::clicked, this, &RenameDialog::OnCancelClicked);
		connect(mRuleMoveTop, &QToolButton::clicked, this, &RenameDialog::OnRuleMoveTop);
		connect(mRuleMoveUp, &QToolButton::clicked, this, &RenameDialog::OnRuleMoveUp);
		connect(mRuleMoveDown, &QToolButton::clicked, this, &RenameDialog::OnRuleMoveDown);
		connect(mRuleMoveBottom, &QToolButton::clicked, this, &RenameDialog::OnRuleMoveBottom);
		connect(mLoadFromClipboardButton, &QToolButton::clicked, this, &RenameDialog::OnLoadFileFromClipboard);
	}

	void RenameDialog::OnOkClicked(){
		// submit task
		FileHandlerPtr handler = std::make_shared<FFX::FileRenameHandler>(mExprListWidget->MakeRenameHandler());
		QString dupTempl = mDupFormatEdit->text();

		if (mCaseTransformCheckBox->isChecked()) {
			std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::CaseTransformHandler>(mFileUpperCheckBox->isChecked()));
		}
		if (mDuplicatesCheckBox->isChecked()) {
			std::dynamic_pointer_cast<FFX::FileRenameHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>(dupTempl, mFirstEffectCheckBox->isChecked(), mDupSuffixCheckBox->isChecked()));
		}

		MainWindow::Instance()->TaskPanelPtr()->Submit(FileInfoList(mFiles), handler);
		QDialog::accept();
	}

	void RenameDialog::OnCancelClicked() {
		QDialog::reject();
	}

	void RenameDialog::OnRuleChanged() {
		// Update all files
		FileHandlerPtr handler = mExprListWidget->MakeRenameHandler();
		QString dupTempl = mDupFormatEdit->text();

		if (mCaseTransformCheckBox->isChecked()) {
			std::dynamic_pointer_cast<FFX::PipeFileHandler>(handler)->Append(std::make_shared<FFX::CaseTransformHandler>(mFileUpperCheckBox->isChecked()));
		}
		if (mDuplicatesCheckBox->isChecked()) {
			std::dynamic_pointer_cast<FFX::PipeFileHandler>(handler)->Append(std::make_shared<FFX::FileDuplicateHandler>(dupTempl, mFirstEffectCheckBox->isChecked(), mDupSuffixCheckBox->isChecked()));
		}
		QFileInfoList result = handler->Handle(FileInfoList(mFiles));
		mRenameFileListView->Apply(result);
	}

	void RenameDialog::OnAddRuleButtonClicked() {
		mExprListWidget->AddRule("", "");
		int row = mExprListWidget->count();
		//if (row >= 6)
		//	mAddRuleButton->setEnabled(false);
		OnRuleChanged();
	}

	// [\一-\龥]: 匹配中文
	void RenameDialog::OnRemoveDigital() {
		mExprListWidget->AddRule("[\\d]", "", QRegExp::RegExp);
		int row = mExprListWidget->count();
		//if (row >= 6)
		//	mAddRuleButton->setEnabled(false);
		OnRuleChanged();
	}

	void RenameDialog::OnRemoveSpecitalChar() {
		mExprListWidget->AddRule("[\\(\\)\\[\\]\\{\\}\\+\\-\\_\\=@#$%&\\^!,'\\.\\s]", "", QRegExp::RegExp);
		int row = mExprListWidget->count();
		//if (row >= 6)
		//	mAddRuleButton->setEnabled(false);
		OnRuleChanged();
	}

	void RenameDialog::OnRemoveChineseChar() {
		mExprListWidget->AddRule("[\\一-\\龥]", "", QRegExp::RegExp);
		int row = mExprListWidget->count();
		//if (row >= 6)
		//	mAddRuleButton->setEnabled(false);
		OnRuleChanged();
	}

	void RenameDialog::OnRemoveRuleButtonClicked() {
		int row = mExprListWidget->currentRow();
		if (row < 0)
			return;
		delete mExprListWidget->takeItem(row);
		int rowCount = mExprListWidget->count();
		//if (rowCount < 6)
		//	mAddRuleButton->setEnabled(true);
		OnRuleChanged();
	}

	void RenameDialog::OnRuleMoveUp() {
		mExprListWidget->MoveRowUp();
	}

	void RenameDialog::OnRuleMoveDown() {
		mExprListWidget->MoveRowDown();
	}

	void RenameDialog::OnRuleMoveTop() {
		mExprListWidget->MoveRowTop();
	}

	void RenameDialog::OnRuleMoveBottom() {
		mExprListWidget->MoveRowBottom();
	}

	void RenameDialog::OnLoadFileFromClipboard() {
		mRenameFileListView->Clear();
		QClipboard* clipboard = QApplication::clipboard();
		const QMimeData* mimeData = clipboard->mimeData();
		QList<QUrl> curUrls = mimeData->urls();
		int size = curUrls.size();
		for (int i = 0; i < size; i++) {
			QString file = curUrls[i].toLocalFile();
			mRenameFileListView->AddFile(file);
		}
	}
}
