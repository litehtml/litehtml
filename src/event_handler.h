#pragma once
#include "object.h"

namespace litehtml
{
	enum mouse_event_type
	{
		mouse_event_click,
		mouse_event_contextmenu,
		mouse_event_dblclick,
		mouse_event_mousedown,
		mouse_event_mouseenter,
		mouse_event_mouseleave,
		mouse_event_mousemove,
		mouse_event_mouseover,
		mouse_event_mouseout,
		mouse_event_mouseup,
		mouse_event_invalid
	};

	enum browser_event_type
	{
		browser_event_load,
		browser_event_unload,
		browser_event_invalid
	};

	struct mouse_event
	{
		mouse_event_type m_type;
		point m_position;
	};

	struct event_response
	{
		event_response() : m_prevent_default( false ), m_stop_propagation ( false ){}
		bool	m_prevent_default;
		bool	m_stop_propagation;
	};

	class event_handler : public object
	{
	public:
		typedef litehtml::object_ptr<litehtml::event_handler>		ptr;

		virtual void 		on_mouse_event( element & el, event_response & response, const mouse_event & event ) = 0;
		virtual void 		on_browser_event( element & el, event_response & response, const browser_event_type event ) = 0;
	};
}
