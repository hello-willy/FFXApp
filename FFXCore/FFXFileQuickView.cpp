#include "FFXFileQuickView.h"

#include <QStyleOptionToolButton>
#include <QApplication>
#include <QLabel>
#include <QHBoxLayout>

namespace FFX {
    const char* ActionLabelStyle =
        "QSint--ActionLabel[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #0033ff;"
        "text-align: left;"
        "font: 11px;"
        "}"

        "QSint--ActionLabel[class='action']:!enabled {"
        "color: #999999;"
        "}"

        "QSint--ActionLabel[class='action']:hover {"
        "color: #0099ff;"
        "text-decoration: underline;"
        "}"

        "QSint--ActionLabel[class='action']:focus {"
        "border: 1px dotted black;"
        "}"

        "QSint--ActionLabel[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
        "}"
        ;
    const char* ActionBoxStyle =
        "QSint--ActionBox {"
        "background-color: white;"
        "border: 1px solid white;"
        "border-radius: 3px;"
        "text-align: left;"
        "}"

        "QSint--ActionBox:hover {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
        "border: 1px solid #DAF2FC;"
        "}"


        "QSint--ActionBox QSint--ActionLabel[class='header'] {"
        "text-align: left;"
        "font: 14px;"
        "color: #006600;"
        "background-color: transparent;"
        "border: none;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='header']:hover {"
        "color: #00cc00;"
        "text-decoration: underline;"
        "}"


        "QSint--ActionBox QSint--ActionLabel[class='action'] {"
        "background-color: transparent;"
        "border: none;"
        "color: #0033ff;"
        "text-align: left;"
        "font: 11px;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:!enabled {"
        "color: #999999;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:hover {"
        "color: #0099ff;"
        "text-decoration: underline;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
        "}"

        ;
    ActionLabel::ActionLabel(QWidget* parent) 
        : QToolButton(parent) {
        Init();
    }

    void ActionLabel::Init() {
		setCursor(Qt::PointingHandCursor);
		setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		setStyleSheet(QString(ActionLabelStyle));
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		setFocusPolicy(Qt::StrongFocus);
	}

    QSize ActionLabel::sizeHint() const {
        ensurePolished();

        int w = 0, h = 0;

        QStyleOptionToolButton opt;
        initStyleOption(&opt);

        QString s(text());
        bool empty = s.isEmpty();
        if (empty)
            s = QString::fromLatin1("XXXX");
        QFontMetrics fm = fontMetrics();
        QSize sz = fm.size(Qt::TextShowMnemonic, s);
        if (!empty || !w)
            w += sz.width();
        if (!empty || !h)
            h = qMax(h, sz.height());
        opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height

        if (!icon().isNull()) {
            int ih = opt.iconSize.height();
            int iw = opt.iconSize.width() + 4;
            w += iw;
            h = qMax(h, ih);
        }

        if (menu())
            w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

        h += 4;
        w += 8;

        QSize sizeHint = (style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(w, h), this).
            expandedTo(QApplication::globalStrut()));

        return sizeHint;
    }

    QSize ActionLabel::minimumSizeHint() const {
        return sizeHint();
    }

    ActionBox::ActionBox(const QPixmap& icon, const QString& headerText, QWidget* parent) : QFrame(parent) {
        Init();
        mHeaderLabel->setText(headerText);
        SetIcon(icon);
    }
    
    void ActionBox::Init() {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        setStyleSheet(QString(ActionBoxStyle));

        QHBoxLayout* mainLayout = new QHBoxLayout(this);

        QVBoxLayout* iconLayout = new QVBoxLayout();
        mainLayout->addLayout(iconLayout);

        mIconLabel = new QLabel(this);
        iconLayout->addWidget(mIconLabel);
        iconLayout->addStretch();

        mMainLayout = new QVBoxLayout();
        mainLayout->addLayout(mMainLayout);

        mHeaderLabel = CreateItem("");
    }

    void ActionBox::SetIcon(const QPixmap& icon) {
        mIconLabel->setPixmap(icon);
        mIconLabel->setFixedSize(icon.size());
    }

    const QPixmap* ActionBox::Icon() const {
        return mIconLabel->pixmap(); 
    }

    ActionLabel* ActionBox::Header() const {
        return mHeaderLabel; 
    }

    ActionLabel* ActionBox::CreateItem(const QString& text, QAction* action) {
        if (!action)
            return 0;

        ActionLabel* act = new ActionLabel(this);
        act->setText(text);
        act->setStyleSheet("");

        QHBoxLayout* hbl = new QHBoxLayout();
        hbl->addWidget(act);
        hbl->addItem(new QSpacerItem(1, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        mMainLayout->addLayout(hbl);

        if(action != nullptr)
            act->setDefaultAction(action);
        return act;
    }

	FileQuickView::FileQuickView(QWidget* parent)
		: QWidget(parent) {
        // Init();
    }

	FileQuickView::~FileQuickView()
	{}

}
