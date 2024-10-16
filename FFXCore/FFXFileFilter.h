#pragma once
#include "FFXCore.h"

#include <QFileInfo>

#include <memory> // for std::shared_ptr


namespace FFX {
	class FileFilter {
	public:
		virtual bool Accept(const QFileInfo& file) const = 0;
	};
	typedef std::shared_ptr<FileFilter> FileFilterPtr;

	class EmptyFilter : public FileFilter {
	public:
		virtual bool Accept(const QFileInfo& file) const;
	};

	class ComposeFileFilter : public FileFilter
	{
	public:
		ComposeFileFilter(FileFilterPtr left, FileFilterPtr right)
			: mLeftFilter(left)
			, mRightFilter(right) {}

		FileFilterPtr LeftFilter() {
			return mLeftFilter;
		}
		FileFilterPtr RightFilter() {
			return mRightFilter;
		}
	protected:
		FileFilterPtr mLeftFilter;
		FileFilterPtr mRightFilter;
	};

	class FFXCORE_EXPORT AndFileFilter : public ComposeFileFilter
	{
	public:
		AndFileFilter(FileFilterPtr left, FileFilterPtr right)
			: ComposeFileFilter(left, right) {}
	public:
		virtual bool Accept(const QFileInfo& file) const override;
	};

	class FFXCORE_EXPORT OrFileFilter : public ComposeFileFilter
	{
	public:
		OrFileFilter(FileFilterPtr left, FileFilterPtr right)
			: ComposeFileFilter(left, right) {}

	public:
		virtual bool Accept(const QFileInfo& file) const override;
	};

	class FFXCORE_EXPORT NotFileFilter : public FileFilter
	{
	public:
		NotFileFilter(FileFilterPtr otherFilter)
			: mOtherFilter(otherFilter) {}
	public:
		virtual bool Accept(const QFileInfo& file) const override;
	private:
		FileFilterPtr mOtherFilter;
	};

	class FFXCORE_EXPORT OnlyFileFilter : public FileFilter
	{
	public:
		virtual bool Accept(const QFileInfo& file) const override;
	};

	class FFXCORE_EXPORT OnlyDirFilter : public FileFilter
	{
	public:
		virtual bool Accept(const QFileInfo& file) const override;
	};

	class FFXCORE_EXPORT RegExpFileFilter : public FileFilter
	{
	public:
		RegExpFileFilter(const QString& pattern, QRegExp::PatternSyntax syntax = QRegExp::Wildcard, bool caseSenitive = true)
			: mRegExp(pattern, caseSenitive ? Qt::CaseSensitive : Qt::CaseInsensitive, syntax) {}
		
	public:
		virtual bool Accept(const QFileInfo& file) const override;

	private:
		QRegExp mRegExp;
	};
}


