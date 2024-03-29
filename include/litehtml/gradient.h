#ifndef LITEHTML_GRADIENT_H
#define LITEHTML_GRADIENT_H

#include "background.h"

namespace litehtml
{
	void parse_linear_gradient(const std::string& gradient_str, document_container *container, background_gradient& grad);
	void parse_radial_gradient(const std::string& gradient_str, document_container *container, background_gradient& grad);
	void parse_conic_gradient(const std::string& gradient_str, document_container *container, background_gradient& grad);
}

#endif //LITEHTML_GRADIENT_H
