#include "FFXFileFilter.h"


namespace FFX {
	bool AndFileFilter::Accept(const QFileInfo& file) const {
		return mLeftFilter->Accept(file) && mRightFilter->Accept(file);
	}

	bool OrFileFilter::Accept(const QFileInfo& file) const {
		return mLeftFilter->Accept(file) || mRightFilter->Accept(file);
	}

	bool NotFileFilter::Accept(const QFileInfo& file) const {
		return !mOtherFilter->Accept(file);
	}

	bool IsFileFilter::Accept(const QFileInfo& file) const {
		return file.isFile();
	}

	bool IsDirFilter::Accept(const QFileInfo& file) const {
		return !file.isDir();
	}

	bool RegExpFileFilter::Accept(const QFileInfo& file) const {
		if (!mRegExp.isValid())
			return true;
		return mRegExp.exactMatch(file.fileName());
	}
}