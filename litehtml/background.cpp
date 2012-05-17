#include "html.h"
#include "background.h"

litehtml::background::background(void)
{
	m_attachment	= background_attachment_scroll;
	m_repeat		= background_repeat_repeat;
	m_clip			= background_box_border;
	m_origin		= background_box_padding;
	m_color.alpha	= 0;
	m_color.red		= 0;
	m_color.green	= 0;
	m_color.blue	= 0;
}

litehtml::background::background( const background& val )
{
	m_image			= val.m_image;
	m_baseurl		= val.m_baseurl;
	m_color			= val.m_color;
	m_attachment	= val.m_attachment;
	m_position		= val.m_position;
	m_repeat		= val.m_repeat;
	m_clip			= val.m_clip;
	m_origin		= val.m_origin;
}

litehtml::background::~background(void)
{
}

void litehtml::background::operator=( const background& val )
{
	m_image			= val.m_image;
	m_baseurl		= val.m_baseurl;
	m_color			= val.m_color;
	m_attachment	= val.m_attachment;
	m_position		= val.m_position;
	m_repeat		= val.m_repeat;
	m_clip			= val.m_clip;
	m_origin		= val.m_origin;
}
