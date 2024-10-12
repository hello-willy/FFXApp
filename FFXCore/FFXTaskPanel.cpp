#include "FFXTaskPanel.h"
#include "FFXString.h"

#include <QGridLayout>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QFrame>
#include <QProgressBar>
#include <QHeaderView>
#include <QDateTime>

namespace FFX {
	int TaskIdGenerator::Id() {
		return mAutoincreamentId++;
	}

	TaskProgressBar::TaskProgressBar(int height, QWidget* parent)
		: QWidget(parent) {
		mProgressBar = new QProgressBar;
		mProgressBar->setAlignment(Qt::AlignCenter);
		mLayout = new QVBoxLayout;
		mLayout->setMargin(0);
		mLayout->setSpacing(0);
		mLayout->addWidget(mProgressBar, 1);
		//mLayout->setAlignment(mProgressBar, Qt::AlignCenter);
		setLayout(mLayout);
		mProgressBar->setFixedHeight(height);
	}

	void TaskProgressBar::setMinimum(int minimum) {
		mProgressBar->setMinimum(minimum);
	}

	void TaskProgressBar::setMaximum(int maximum) {
		mProgressBar->setMaximum(maximum);
	}

	void TaskProgressBar::setValue(int value) {
		mProgressBar->setValue(value);
	}

	QMap<QString, int> HEADER = { {"ID", 0}, {"NAME", 1}, {"START", 2}, {"PROG", 5}, {"COST", 3}, {"STATE", 4}, {"MSG", 6} };
	TaskPanel::TaskPanel(QWidget* parent)
		: QWidget(parent) {
		SetupUi();
	}

	TaskPanel::~TaskPanel()	{
		// clear the task
	}

	int TaskPanel::Submit(const QFileInfoList& files, FileHandlerPtr handler, bool showInPanel) {
		int newTaskId = mTaskIdGenerator.Id();
		Task* newTask = new Task(newTaskId, files, handler);
		connect(newTask, &Task::TaskComplete, this, &TaskPanel::OnTaskComplete);
		connect(newTask, &Task::TaskProgressChanged, this, &TaskPanel::OnTaskProgressChanged);
		connect(newTask, &Task::TaskFileHandled, this, &TaskPanel::OnTaskFileHandled);
		connect(newTask, &Task::TaskStateChanged, this, &TaskPanel::OnTaskStateChanged);

		if (showInPanel) {
			int row = 0;
			mTaskTable->insertRow(row);

			QTableWidgetItem* idRow = new QTableWidgetItem(QString::number(newTaskId));
			idRow->setData(Qt::UserRole, newTaskId);
			mTaskTable->setItem(row, HEADER["ID"], idRow);

			QTableWidgetItem* nameRow = new QTableWidgetItem(handler->DisplayName());
			mTaskTable->setItem(row, HEADER["NAME"], nameRow);

			QTableWidgetItem* timeRow = new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
			mTaskTable->setItem(row, HEADER["START"], timeRow);

			QTableWidgetItem* progressRow = new QTableWidgetItem("");
			mTaskTable->setItem(row, HEADER["PROG"], progressRow);
			mTaskTable->setCellWidget(row, HEADER["PROG"], new TaskProgressBar(30));

			QTableWidgetItem* costTimeRow = new QTableWidgetItem("-");
			mTaskTable->setItem(row, HEADER["COST"], costTimeRow);

			QTableWidgetItem* stateRow = new QTableWidgetItem(Task::StateText(newTask->Status()));
			mTaskTable->setItem(row, HEADER["STATE"], stateRow);

			QTableWidgetItem* msgRow = new QTableWidgetItem("");
			mTaskTable->setItem(row, HEADER["MSG"], msgRow);
		}
		
		QMutexLocker locker(&mTaskMapLocker);
		mTaskMap.insert(newTaskId, TaskPtr(newTask));
		mWorkerGroup->start(newTask);

		emit TaskSubmit(newTaskId);
		return newTaskId;
	}

	void TaskPanel::Cancel(int taskId) {
		QMutexLocker locker(&mTaskMapLocker);
		TaskPtr task = mTaskMap.value(taskId);
		if (task != nullptr)
			task->Cancel();
	}

	void TaskPanel::SetupUi() {
		resize(600, 400);

		mMainGridLayout = new QGridLayout;
		mMainGridLayout->setSpacing(6);
		mMainGridLayout->setContentsMargins(6, 6, 6, 6);
		setLayout(mMainGridLayout);

		mRemoveTaskButton = new QToolButton;
		mRemoveTaskButton->setText(QObject::tr("&Delete"));
		mRemoveTaskButton->setIcon(QIcon(":/ffx/res/image/delete.svg"));
		mRemoveTaskButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

		mMainGridLayout->addWidget(mRemoveTaskButton, 0, 6, 1, 1);

		mFilterLabel = new QLabel;
		mFilterLabel->setText(QObject::tr("Filter:"));
		mMainGridLayout->addWidget(mFilterLabel, 0, 0, 1, 1);

		mStateFilterCombo = new QComboBox;
		mMainGridLayout->addWidget(mStateFilterCombo, 0, 2, 1, 1);

		mTaskSearchEdit = new QLineEdit;
		mTaskSearchEdit->setPlaceholderText(QObject::tr("Please enter filtering keywords"));
		mMainGridLayout->addWidget(mTaskSearchEdit, 0, 1, 1, 1);

		mNewTaskButton = new QToolButton;
		mNewTaskButton->setText(QObject::tr("&New Task"));
		mNewTaskButton->setIcon(QIcon(":/ffx/res/image/plus.svg"));
		mNewTaskButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mNewTaskButton->setVisible(false); // TODO: set visible first.
		mMainGridLayout->addWidget(mNewTaskButton, 0, 4, 1, 1);

		mCancelTaskButton = new QToolButton;
		mCancelTaskButton->setText(QObject::tr("&Cancel"));
		mCancelTaskButton->setIcon(QIcon(":/ffx/res/image/cancel.svg"));
		mCancelTaskButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		mMainGridLayout->addWidget(mCancelTaskButton, 0, 5, 1, 1);

		mTaskTable = new QTableWidget;
		mMainGridLayout->addWidget(mTaskTable, 1, 0, 1, 7);
		mSeperator = new QFrame;
		mSeperator->setFrameShape(QFrame::VLine);
		mSeperator->setFrameShadow(QFrame::Sunken);
		mMainGridLayout->addWidget(mSeperator, 0, 3, 1, 1);

		mTaskTable->verticalHeader()->setHidden(true);
		mTaskTable->horizontalHeader()->setHighlightSections(false);
		mTaskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		mTaskTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

		// [0]id, [1]name, [2]start time, [3]progress, [4]cost, [5]state
		mTaskTable->setColumnCount(HEADER.size());
		mTaskTable->horizontalHeader()->sectionResizeMode(QHeaderView::Stretch);
		mTaskTable->horizontalHeader()->setStretchLastSection(true);
		// init the horizontal header
		QTableWidgetItem* idHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/property.svg"), QObject::tr("Task ID"));
		idHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["ID"], QHeaderView::Interactive);
		mTaskTable->setColumnWidth(HEADER["ID"], 120);
		mTaskTable->setHorizontalHeaderItem(HEADER["ID"], idHeader);

		QTableWidgetItem* nameHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/task.svg"), QObject::tr("Name"));
		nameHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["NAME"], QHeaderView::Interactive);
		mTaskTable->setColumnWidth(HEADER["NAME"], 400);
		mTaskTable->setHorizontalHeaderItem(HEADER["NAME"], nameHeader);

		QTableWidgetItem* startTimeHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/time-start.svg"), QObject::tr("Time Start"));
		startTimeHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["START"], QHeaderView::Interactive);
		mTaskTable->setColumnWidth(HEADER["START"], 240);
		mTaskTable->setHorizontalHeaderItem(HEADER["START"], startTimeHeader);

		QTableWidgetItem* progressHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/progress.svg"), QObject::tr("Progress"));
		progressHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["PROG"], QHeaderView::Interactive);
		mTaskTable->setColumnWidth(HEADER["PROG"], 350);
		mTaskTable->setHorizontalHeaderItem(HEADER["PROG"], progressHeader);

		QTableWidgetItem* costHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/time-cost.svg"), QObject::tr("Time Cost"));
		costHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["COST"], QHeaderView::Interactive);
		mTaskTable->setColumnWidth(HEADER["COST"], 120);
		mTaskTable->setHorizontalHeaderItem(HEADER["COST"], costHeader);

		QTableWidgetItem* stateHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/category.svg"), QObject::tr("Status"));
		stateHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["STATE"], QHeaderView::Interactive);
		mTaskTable->setColumnWidth(HEADER["STATE"], 100);
		mTaskTable->setHorizontalHeaderItem(HEADER["STATE"], stateHeader);

		QTableWidgetItem* msgHeader = new QTableWidgetItem(QIcon(":/cfm/resource/images/message.svg"), QObject::tr("Message"));
		msgHeader->setTextAlignment(Qt::AlignLeft);
		mTaskTable->horizontalHeader()->setSectionResizeMode(HEADER["MSG"], QHeaderView::Interactive);
		mTaskTable->setHorizontalHeaderItem(HEADER["MSG"], msgHeader);
		connect(mTaskTable, &QTableWidget::itemSelectionChanged, this, &TaskPanel::OnTaskTableItemSelectionChanged);
		connect(mTaskTable, &QTableWidget::itemChanged, this, &TaskPanel::OnTaskTableItemChanged);

		mCancelTaskButton->setEnabled(false);
		mRemoveTaskButton->setEnabled(false);

		mStateFilterCombo->setFixedWidth(mStateFilterCombo->sizeHint().width() * 1.2);
		mStateFilterCombo->addItem(QObject::tr("All"), -1);
		mStateFilterCombo->addItem(QObject::tr("Queued"), 0);
		mStateFilterCombo->addItem(QObject::tr("Holded"), 1);
		mStateFilterCombo->addItem(QObject::tr("Running"), 2);
		mStateFilterCombo->addItem(QObject::tr("Succeeded"), 3);
		mStateFilterCombo->addItem(QObject::tr("Failed"), 4);

		connect(mStateFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { UpdateTaskTable(); });
		connect(mTaskSearchEdit, &QLineEdit::textChanged, this, [this]() { UpdateTaskTable(); });
		connect(mCancelTaskButton, &QToolButton::clicked, this, &TaskPanel::OnCancelTaskButtonClicked);
		connect(mRemoveTaskButton, &QToolButton::clicked, this, &TaskPanel::OnRemoveTaskButtonClicked);
	}

	int TaskPanel::RowOf(int taskId) {
		int rowCount = mTaskTable->rowCount();
		for (int i = 0; i < rowCount; i++)
		{
			QTableWidgetItem* item = mTaskTable->item(i, HEADER["ID"]);
			if (item->data(Qt::UserRole).toInt() == taskId)
				return i;
		}
		return -1;
	}

	void TaskPanel::OnTaskTableItemSelectionChanged() {
		QList<QTableWidgetItem*> items = mTaskTable->selectedItems();
		if (items.isEmpty()) {
			mCancelTaskButton->setEnabled(false);
			mRemoveTaskButton->setEnabled(false);
			return;
		}
		bool hasRunningTask = false;
		bool hasFinishedTask = false;
		for (QTableWidgetItem* item : items) {
			if (item == nullptr)
				continue;
			QTableWidgetItem* stateItem = mTaskTable->item(item->row(), HEADER["STATE"]);
			if (stateItem == nullptr)
				continue;
			int state = stateItem->data(Qt::UserRole).toInt();
			if (state == (int)(Task::State::Succeeded) || state == (int)(Task::State::Failed))
				hasFinishedTask = true;
			if (state == (int)(Task::State::Running))
				hasRunningTask = true;
			if (hasFinishedTask && hasRunningTask)
				break;
		}
		mCancelTaskButton->setEnabled(hasRunningTask);
		mRemoveTaskButton->setEnabled(hasFinishedTask);
	}

	void TaskPanel::OnTaskTableItemChanged() {
		//! Update ui
	}

	void TaskPanel::OnCancelTaskButtonClicked() {
		QList<QTableWidgetItem*> items = mTaskTable->selectedItems();
		if (items.isEmpty())
			return;
		for (QTableWidgetItem* item : items)
		{
			QTableWidgetItem* stateItem = mTaskTable->item(item->row(), HEADER["STATE"]);
			if (stateItem == nullptr)
				continue;
			int state = stateItem->data(Qt::UserRole).toInt();
			if (state != (int)(Task::State::Running))
				continue;
			QTableWidgetItem* idItem = mTaskTable->item(item->row(), HEADER["ID"]);
			if (idItem == nullptr)
				continue;
			int taskId = idItem->data(Qt::UserRole).toInt();
			Cancel(taskId);
		}
	}

	void TaskPanel::OnRemoveTaskButtonClicked() {
		QList<QTableWidgetItem*> items = mTaskTable->selectedItems();
		if (items.isEmpty())
			return;
		QList<int> toRemovedRows;
		for (QTableWidgetItem* item : items) {
			QTableWidgetItem* stateItem = mTaskTable->item(item->row(), HEADER["STATE"]);
			if (stateItem == nullptr)
				continue;
			int state = stateItem->data(Qt::UserRole).toInt();
			if (!(state == (int)(Task::State::Succeeded) || state == (int)(Task::State::Failed)))
				continue;
			toRemovedRows << item->row();
		}
		if (toRemovedRows.isEmpty())
			return;

		std::sort(toRemovedRows.begin(), toRemovedRows.end());
		for (int i = toRemovedRows.size() - 1; i >= 0; i--) {
			mTaskTable->removeRow(toRemovedRows[i]);
			QTableWidgetItem* stateItem = mTaskTable->item(toRemovedRows[i], HEADER["ID"]);
			if (stateItem == nullptr)
				continue;
			int taskId = stateItem->data(Qt::UserRole).toInt();
			RemoveTaskFromCache(taskId);
		}
	}

	int TaskPanel::RunningTaskCount() const {
		int r = 0;
		QMap<int, TaskPtr>::const_iterator it = mTaskMap.begin();
		for (; it != mTaskMap.end(); it++) {
			if (it.value()->Status() == Task::State::Running || it.value()->Status() == Task::State::Queued) {
				r++;
			}
		}
		return r;
	}

	void TaskPanel::OnTaskStateChanged(int taskId, int oldState, int state) {
		int row = RowOf(taskId);
		if (row < 0)
			return;
		QTableWidgetItem* item = mTaskTable->item(row, HEADER["STATE"]);
		if (item == nullptr)
			return;
		item->setText(Task::StateText(static_cast<Task::State>(state)));
		item->setData(Qt::UserRole, state);
	}

	void TaskPanel::OnTaskComplete(int taskId, bool success, const QString& msg, qint64 timeCost) {
		// transfer the task complete signals.
		emit TaskComplete(taskId, success);

		int row = RowOf(taskId);
		if (row < 0)
			return;
		QTableWidgetItem* stateItem = mTaskTable->item(row, HEADER["STATE"]);
		if (stateItem == nullptr)
			return;
		stateItem->setText(success ? Task::StateText(Task::State::Succeeded) : Task::StateText(Task::State::Failed));

		QTableWidgetItem* timeCostItem = mTaskTable->item(row, HEADER["COST"]);
		if (timeCostItem == nullptr)
			return;
		timeCostItem->setText(String::TimeHint(timeCost));
		
		// remove task from cache
		// QMutexLocker locker(&mTaskMapLocker);
		// mTaskMap.remove(taskId);

		QTableWidgetItem* msgItem = mTaskTable->item(row, HEADER["MSG"]);
		if (msgItem == nullptr)
			return;
		msgItem->setText(msg);

		TaskProgressBar* pb = static_cast<TaskProgressBar*>(mTaskTable->cellWidget(row, HEADER["PROG"]));
		if (pb != nullptr) {
			pb->setMinimum(0);
			pb->setMaximum(100);
			pb->setValue(100);
		}
	}

	void TaskPanel::OnTaskProgressChanged(int taskId, const QString& message, int pos) {
		emit TaskProgressChanged(taskId, message, pos);

		int row = RowOf(taskId);
		if (row < 0)
			return;
		QTableWidgetItem* item = mTaskTable->item(row, HEADER["PROG"]);
		if (item == nullptr)
			return;
		TaskProgressBar* pb = static_cast<TaskProgressBar*>(mTaskTable->cellWidget(row, HEADER["PROG"]));
		if (pb != nullptr) {
			if (pos < 0) {
				pb->setMinimum(0);
				pb->setMaximum(0);
			} else {
				pb->setMinimum(0);
				pb->setMaximum(100);
				pb->setValue(pos);
			}
		}
		QTableWidgetItem* msgItem = mTaskTable->item(row, HEADER["MSG"]);
		if (msgItem == nullptr)
			return;
		msgItem->setText(message);
	}

	void TaskPanel::OnTaskFileHandled(int taskId, const QFileInfo& fileInput, const QFileInfo& fileOutput, bool success, const QString& message) {
		emit TaskFileHandled(taskId, fileInput, fileOutput, success, message);
	}

	void TaskPanel::UpdateTaskTable() {
		int type = mStateFilterCombo->currentData().toInt();
		int rowCount = mTaskTable->rowCount();
		QString pattern = mTaskSearchEdit->text();
		for (int i = 0; i < rowCount; i++) {
			if (type == -1) {
				mTaskTable->setRowHidden(i, false);
				continue;
			}
			bool stateFlag = true;
			bool nameFlag = true;
			QTableWidgetItem* stateRow = mTaskTable->item(i, HEADER["STATE"]);
			if (stateRow != nullptr) {
				int state = stateRow->data(Qt::UserRole).toInt();
				stateFlag = (state == type);
			}
			QTableWidgetItem* nameRow = mTaskTable->item(i, HEADER["NAME"]);
			if (nameRow != nullptr) {
				QString name = nameRow->text();
				nameFlag = name.contains(QRegExp(pattern, Qt::CaseSensitive, QRegExp::FixedString));
			}

			mTaskTable->setRowHidden(i, !(nameFlag && stateFlag));
		}
	}

	void TaskPanel::RemoveTaskFromCache(int taskId) {
		QMutexLocker locker(&mTaskMapLocker);
		mTaskMap.remove(taskId);
	}
}

