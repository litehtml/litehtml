#ifndef LITEHTML_CSS_CALC_H
#define LITEHTML_CSS_CALC_H

#include "litehtml/css_length.h"
#include "litehtml/css_tokenizer.h"
#include "litehtml/types.h"
#include <memory>
#include <utility>

namespace litehtml
{
	namespace expr
	{
		// Node base class
		class node
		{
		  public:
			node() {}
			virtual ~node() = default;
		};

		// Operation base class
		class node_op : public node
		{
			std::unique_ptr<node> m_left;
			std::unique_ptr<node> m_right;
		};

		// Value node
		class node_value : public node
		{
			float value = 0;
			css_units units = css_units_none;
		};

		// Function base class
		class node_func : public node
		{
			std::unique_ptr<node> m_root;
		};

		// Expression class
		class expression
		{
			std::unique_ptr<node> m_root;

		  public:
			expression();

			bool from_token(const css_token& token, int options);

		  private:

		};
	} // namespace expr

} // namespace litehtml

#endif // LITEHTML_CSS_CALC_H
