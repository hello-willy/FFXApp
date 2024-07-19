#include "FFXFileFilterExpr.h"
#include "FFXString.h"

namespace FFX {
	LogicalOperator::LogicalOperator(const std::string& name, int priorIn, int priorOut)
		: mName(name)
		, mPriorIn(priorIn)
		, mPriorOut(priorOut)
	{}

	LogicalOperator::LogicalOperator(const LogicalOperator& other)
		: mName(other.mName)
		, mPriorIn(other.mPriorIn)
		, mPriorOut(other.mPriorOut)
	{}

	LogicalOperator& LogicalOperator::operator=(const LogicalOperator& other) {
		mName = other.mName;
		mPriorIn = other.mPriorIn;
		mPriorOut = other.mPriorOut;
		return *this;
	}

	bool LogicalOperator::operator == (const LogicalOperator& other) const {
		return mName == other.mName;
	}

	bool LogicalOperator::operator >= (const LogicalOperator& other) const {
		return mPriorOut >= other.mPriorIn;
	}

	FileFilterPtr LogicalAnd::Apply(std::stack<FileFilterPtr>& stack)
	{
		if (stack.size() < 2)
			throw std::exception("Expression pase failed!");
		FileFilterPtr f1 = stack.top();
		stack.pop();
		FileFilterPtr f2 = stack.top();
		stack.pop();
		return std::make_shared<AndFileFilter>(f1, f2);
	}

	FileFilterPtr LogicalOr::Apply(std::stack<FileFilterPtr>& stack)
	{
		if (stack.size() < 2)
			throw std::exception("Expression pase failed!");
		FileFilterPtr f1 = stack.top();
		stack.pop();
		FileFilterPtr f2 = stack.top();
		stack.pop();
		return std::make_shared<OrFileFilter>(f1, f2);
	}

	FileFilterPtr LogicalNot::Apply(std::stack<FileFilterPtr>& stack)
	{
		if (stack.size() < 1)
			throw std::exception("Expression pase failed!");
		FileFilterPtr f1 = stack.top();
		stack.pop();
		return std::make_shared<NotFileFilter>(f1);
	}

	std::map<std::string, LogicalOperatorPtr> FileFilterExpr::sAllLogicalOperator = {
		{"&", std::make_shared<LogicalAnd>()},
		{"|", std::make_shared<LogicalOr>()},
		{"!", std::make_shared<LogicalNot>()},
		{"(", std::make_shared<LogicalLeftParen>()},
		{")", std::make_shared<LogicalRightParen>()} };

	LogicalOperatorPtr FileFilterExpr::CreateLogicalOperator(const std::string& token)
	{
		std::map<std::string, LogicalOperatorPtr>::iterator it = sAllLogicalOperator.find(token);
		if (it == sAllLogicalOperator.end())
			return LogicalOperatorPtr();
		return LogicalOperatorPtr(it->second->Clone());
	}

	FileFilterExpr::FileFilterExpr(const std::string& exp, bool caseSenitive)
		: mExpression(exp)
		, mCaseSensitive(caseSenitive)
	{

	}

	FileFilterExpr::~FileFilterExpr()
	{}

	FileFilterPtr FileFilterExpr::Filter() {
		std::vector<std::string> tokens;
		if (!Tokenized(tokens))
			return FileFilterPtr();
		std::vector<std::string> backTokens;
		Convert(tokens, backTokens);

		std::stack<FileFilterPtr> result;
		for (const std::string& token : backTokens)	{
			LogicalOperatorPtr op = CreateLogicalOperator(token);
			if (op != nullptr) {
				FileFilterPtr f = op->Apply(result);
				result.push(f);
			} else {
				result.push(std::make_shared<RegExpFileFilter>(QString::fromStdString(token), QRegExp::Wildcard, mCaseSensitive));
			}
		}
		return result.top();
	}

	bool FileFilterExpr::Match(const std::string& value)
	{
		return Filter()->Accept(QFileInfo(QString::fromStdString(value)));
	}

	bool FileFilterExpr::Tokenized(std::vector<std::string>& variables)
	{
		std::string token;
		size_t size = mExpression.size();
		std::map<char, int> counter;
		std::vector<char> strop = { '!', '&', '|', '(', ')' };
		int vars = 0;
		for (size_t i = 0; i < size; i++) {
			if (std::find(strop.begin(), strop.end(), mExpression[i]) != strop.end()) {
				String::Trim(token);
				if (!token.empty()) {
					variables.push_back(token);
					token.clear();
					vars++;
				}

				variables.push_back(mExpression.substr(i, 1));
				counter[mExpression[i]]++;
				continue;
			}
			token += mExpression[i];
		}
		if (counter['('] != counter[')'])
			return false;

		String::Trim(token);
		if (!token.empty()) {
			variables.push_back(token);
			token.clear();
			vars++;
		}
		int binops = counter['&'] + counter['|'];
		if (binops + 1 != vars)
			return false;
		return true;
	}

	bool FileFilterExpr::Convert(const std::vector<std::string>& tokens, std::vector<std::string>& back) {
		if (tokens.empty()) {
			return true;
		}
		std::stack<LogicalOperatorPtr> ops;
		for (const std::string& token : tokens) {
			LogicalOperatorPtr op = CreateLogicalOperator(token);
			if (op != nullptr) {
				if (ops.empty() || *op >= *ops.top()) {
					ops.push(op);
				} else {
					while (!ops.empty()) {
						if (*ops.top() == *sAllLogicalOperator["("]) {
							ops.pop();
							break;
						}
						back.push_back(ops.top()->Name());
						ops.pop();
						if (*op >= *ops.top()) {
							ops.push(op);
							break;
						}
					}
				}
				continue;
			}
			back.push_back(token);
		}
		while (!ops.empty()) {
			back.push_back(ops.top()->Name());
			ops.pop();
		}
		return true;
	}
}