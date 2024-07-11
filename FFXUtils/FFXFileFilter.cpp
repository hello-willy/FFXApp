#include "FFXFileFilter.h"


namespace FFX {
	bool AndFileFilter::Accept(const FileInfo& file) const {
		return mLeftFilter->Accept(file) && mRightFilter->Accept(file);
	}

	bool OrFileFilter::Accept(const FileInfo& file) const {
		return mLeftFilter->Accept(file) || mRightFilter->Accept(file);
	}

	bool NotFileFilter::Accept(const FileInfo& file) const {
		return !mOtherFilter->Accept(file);
	}

	bool IsFileFilter::Accept(const FileInfo& file) const {
		return file.IsFile();
	}

	bool IsDirFilter::Accept(const FileInfo& file) const {
		return !file.IsFile();
	}

	bool RegExpFileFilter::Accept(const FileInfo& file) const {
		if (!mRegExp.isValid())
			return true;
		return mRegExp.exactMatch(file.FileName());
	}
}