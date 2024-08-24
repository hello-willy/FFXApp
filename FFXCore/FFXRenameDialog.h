#pragma once
#include "FFXFileHandler.h"

#include <QDialog>
#include <QListView>
#include <QStyledItemDelegate>
#include <QListWidget>
#include <QLineEdit>

class QGridLayout;
class QCheckBox;
class QListWidgetItem;
class QHBoxLayout;
class QVBoxLayout;
class QComboBox;
class QTabWidget;
class QLabel;
class QButtonGroup;
class QMenu;

namespace FFX {
	struct RenameData {
		RenameData() = default;
		RenameData(const QString& file, const QString& newFileName) 
			: file(file)
			, newFileName(newFileName) {

		}
		QString file;
		QString newFileName;
	};

	class RenameFileListViewModel : public QAbstractListModel {
		Q_OBJECT
	public:
		explicit RenameFileListViewModel(QObject* parent = 0);
		friend class RenameFileListView;

	public:
		int rowCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;

	public:
		void Append(const QString& file, const QString& newFileName = QString());
		void Clear();
		void RemoveRow(int row);
		void UpdateData(QFileInfoList newFiles);

	private:
		QList<RenameData> mData;
	};

	class RenameFileListViewItemDelegate : public QStyledItemDelegate {
	public:
		RenameFileListViewItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	private:
		int mItemHeight = 65;

	};

	class RenameFileListView : public QListView {
		Q_OBJECT
	public:
		RenameFileListView(QWidget* parent = nullptr);
		
	public:
		void AddFile(const QString& file, const QString& newFileName = QString());
		void Apply(QFileInfoList newFiles);
		void Clear();

	private:
		RenameFileListViewItemDelegate* mItemDelegate;
		RenameFileListViewModel* mDataModel;
	};

	struct ExprData {
		ExprData() = default;
		ExprData(const QString& pattern, const QString& stringFill, QRegExp::PatternSyntax syntax = QRegExp::Wildcard,
			Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive, bool suffixInclude = false)
			: pattern(pattern)
			, after(stringFill)
			, syntax(syntax)
			, caseSensitive(caseSensitive)
			, suffixInclude(suffixInclude) {

		}

		QString pattern = "";
		QString after = "";
		int syntax = QRegExp::Wildcard;
		bool caseSensitive = true;
		bool suffixInclude = false;
	};

	class ExprLineEdit : public QLineEdit {
		Q_OBJECT
	public:
		ExprLineEdit(QWidget* parent = nullptr);

	protected:
		virtual void focusInEvent(QFocusEvent* e) override;

	Q_SIGNALS:
		void focused();
	};

	class ExprListItemWidget : public QWidget {
		Q_OBJECT
	public:
		ExprListItemWidget(QListWidgetItem* listWidgetItem, QWidget* parent = nullptr);
		ExprListItemWidget(QListWidgetItem* listWidgetItem, const QString& pattern, const QString& stringFill, QRegExp::PatternSyntax syntax = QRegExp::Wildcard,
			Qt::CaseSensitivity caseSensitive = Qt::CaseInsensitive, bool suffixInclude = false, QWidget* parent = nullptr);
		~ExprListItemWidget();
	public:
		void SetChecked(bool checked);
		bool IsChecked();
		void SetCaseSensitivity(bool caseSensitivity);
		Qt::CaseSensitivity IsCaseSensitivity() const;
		bool IsSuffixInclude() const;
		QString Pattern() const;
		QRegExp::PatternSyntax Syntax() const;
		QString StringToReplace() const;
		void SetEnable(bool enable);
		FileHandlerPtr MakeReplaceHandler() const;

	protected:
		void paintEvent(QPaintEvent* event);
	private slots:
		void OnItemWidgetFocused();
		void OnItemChanged();

	private:
		QCheckBox* mRuleActivedCheckBox;
		ExprLineEdit* mPatternEdit;
		ExprLineEdit* mStringFillEdit;
		QCheckBox* mCaseSensitiveCheckBox;
		QCheckBox* mSuffixIncludeCheckBox;
		QHBoxLayout* mMainLayout;
		QComboBox* mSyntaxCombo;
		QListWidgetItem* mListWidgetItem;
		QRegExpValidator* mFileNameValidator;
	};

	class ExprListWidget : public QListWidget
	{
		Q_OBJECT
	public:
		ExprListWidget(QWidget* parent = nullptr);
		~ExprListWidget() = default;

	public:
		void MoveRowUp();
		void MoveRowDown();
		void MoveRowTop();
		void MoveRowBottom();
		void AddRule(const QString& pattern, const QString& after, QRegExp::PatternSyntax syntax = QRegExp::Wildcard, 
			Qt::CaseSensitivity = Qt::CaseInsensitive, bool suffixInclude = false);
		ExprData RuleOf(int row);
		void SetEnable(bool enable);
		FileHandlerPtr MakeRenameHandler() const;

	private:
		void MoveRowTo(int row, int toRow);

	Q_SIGNALS:
		void ExprChanged();
	};

	class RenameDialog : public QDialog	{
		Q_OBJECT
	public:
		RenameDialog(QList<QString> files, QWidget* parent = nullptr);
		~RenameDialog();

	private:
		void SetupUi();

	private slots:
		void OnOkClicked();
		void OnCancelClicked();
		void OnRuleChanged();
		void OnAddRuleButtonClicked();
		void OnRemoveDigital();
		void OnRemoveSpecitalChar();
		void OnRemoveChineseChar();
		void OnRemoveRuleButtonClicked();
		void OnRuleMoveUp();
		void OnRuleMoveDown();
		void OnRuleMoveTop();
		void OnRuleMoveBottom();
		void OnLoadFileFromClipboard();

	private:
		QStringList mFiles;

		QGridLayout* mMainLayout;
		QLabel* mRenameFileListTitleLabel;
		QToolButton* mLoadFromClipboardButton;
		QLabel* mRenameFileInfoLabel;
		RenameFileListView* mRenameFileListView;
		QTabWidget* mTabWidget;

		QWidget* mSettingWidget;
		QVBoxLayout* mSettingWidgetLayout;
		QHBoxLayout* mControlLayout;
		QLabel* mControlLabel;
		QToolButton* mRuleMoveTop;
		QToolButton* mRuleMoveUp;
		QToolButton* mRuleMoveDown;
		QToolButton* mRuleMoveBottom;
		QFrame* mControlLine;
		QToolButton* mAddRuleButton;
		QMenu* mAddExprMenu;
		QAction* mAddExprAction;
		QAction* mAddRemoveDigitalExprAction;
		QAction* mAddRemoveSpectialCharExprAction;
		QAction* mAddRemoveChineseCharExprAction;

		QToolButton* mRemoveRuleButton;

		ExprListWidget* mExprListWidget;
		// Misc settings
		QGridLayout* mMiscLayout;
		QCheckBox* mCaseTransformCheckBox;
		QCheckBox* mFileLowerCheckBox;
		QCheckBox* mFileUpperCheckBox;
		QButtonGroup* mCaseButtonGroup;

		QCheckBox* mDuplicatesCheckBox;
		QLineEdit* mDupFormatEdit;
		QCheckBox* mDupSuffixCheckBox;
		QCheckBox* mDupPrefixCheckBox;
		QButtonGroup* mDupButtonGroup;
		QCheckBox* mFirstEffectCheckBox;
		// Foot buttons
		QHBoxLayout* mFootLayout;
		QToolButton* mOkButton;
		QToolButton* mCancelButton;
	};

}
