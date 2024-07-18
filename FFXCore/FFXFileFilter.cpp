#include "FFXFileFilter.h"


namespace FFX {
	bool AndFileFilter::Accept(const File& file) const {
		return mLeftFilter->Accept(file) && mRightFilter->Accept(file);
	}

	bool OrFileFilter::Accept(const File& file) const {
		return mLeftFilter->Accept(file) || mRightFilter->Accept(file);
	}

	bool NotFileFilter::Accept(const File& file) const {
		return !mOtherFilter->Accept(file);
	}

	bool IsFileFilter::Accept(const File& file) const {
		return file.IsFile();
	}

	bool IsDirFilter::Accept(const File& file) const {
		return !file.IsFile();
	}

	bool RegExpFileFilter::Accept(const File& file) const {
		if (!mRegExp.isValid())
			return true;
		return mRegExp.exactMatch(file.FileName());
	}
}