#pragma once
#include "types.h"
#include "attributes.h"
#include "css_length.h"
#include "css_position.h"
#include "web_color.h"

namespace litehtml
{
	class background
	{
	public:
		std::wstring			m_image;
		std::wstring			m_baseurl;
		web_color				m_color;
		background_attachment	m_attachment;
		css_position		m_position;
		background_repeat		m_repeat;
		background_box			m_clip;
		background_box			m_origin;

	public:
		background(void);
		background(const background& val);
		~background(void);

		void operator=(const background& val);
	};

}