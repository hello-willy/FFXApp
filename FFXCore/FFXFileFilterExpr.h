#pragma once

#include "FFXFileFilter.h"

#include <stack>
#include <map>

namespace FFX {
	class LogicalOperator {
	public:
		LogicalOperator() = default;
		LogicalOperator(const std::string & name, int priorIn, int priorOut);
		LogicalOperator(const LogicalOperator & other);

	public:
		LogicalOperator& operator=(const LogicalOperator & other);
		bool operator == (const LogicalOperator & other) const;
		bool operator >= (const LogicalOperator & other) const;

	public:
		bool IsValid() { return !mName.empty(); }
		std::string Name() const { return mName; }
		int PriorIn() const { return mPriorIn; }
		int PriorOut() const { return mPriorOut; }

	public:
		virtual FileFilterPtr Apply(std::stack<FileFilterPtr>&stack) { return FileFilterPtr(); }
		virtual LogicalOperator* Clone() = 0;
	private:
		std::string mName = "";
		int mPriorIn = 0;
		int mPriorOut = 0;
	};
	typedef std::shared_ptr<LogicalOperator> LogicalOperatorPtr;

	class LogicalAnd : public LogicalOperator
	{
	public:
		LogicalAnd() : LogicalOperator("&", 1, 1) {}
	public:
		virtual FileFilterPtr Apply(std::stack<FileFilterPtr>& stack);
		virtual LogicalOperator* Clone() { return new LogicalAnd(*this); }
	};

	class LogicalOr : public LogicalOperator
	{
	public:
		LogicalOr() : LogicalOperator("|", 1, 1) {}
	public:
		virtual FileFilterPtr Apply(std::stack<FileFilterPtr>& stack);
		virtual LogicalOperator* Clone() { return new LogicalOr(*this); }
	};

	class LogicalNot : public LogicalOperator
	{
	public:
		LogicalNot() : LogicalOperator("!", 2, 2) {}
	public:
		virtual FileFilterPtr Apply(std::stack<FileFilterPtr>& stack);
		virtual LogicalOperator* Clone() { return new LogicalNot(*this); }
	};

	class LogicalLeftParen : public LogicalOperator
	{
	public:
		LogicalLeftParen() : LogicalOperator("(", 0, 5) {}
	public:
		virtual LogicalOperator* Clone() { return new LogicalLeftParen(*this); }
	};

	class LogicalRightParen : public LogicalOperator
	{
	public:
		LogicalRightParen() : LogicalOperator(")", 0, -1) {}
	public:
		virtual LogicalOperator* Clone() { return new LogicalRightParen(*this); }
	};


	class FileFilterExpr
	{
	public:
		static FFXCORE_EXPORT std::map<std::string, LogicalOperatorPtr> sAllLogicalOperator;
		static FFXCORE_EXPORT LogicalOperatorPtr CreateLogicalOperator(const std::string& token);

	public:
		FFXCORE_EXPORT FileFilterExpr(const std::string& exp, bool caseSenitive = true);
		FFXCORE_EXPORT ~FileFilterExpr();

	public:
		FFXCORE_EXPORT bool Match(const std::string& value);
		FFXCORE_EXPORT FileFilterPtr Filter();
	private:
		bool Tokenized(std::vector<std::string>& variables);
		bool Convert(const std::vector<std::string>& tokens, std::vector<std::string>& back);
	private:
		std::string mExpression;
		bool mCaseSensitive = true;
	};
}

