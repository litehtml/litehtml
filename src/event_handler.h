#pragma once
#include "object.h"

namespace litehtml
{
	enum mouse_event_type
	{
		event_click,
		event_contextmenu,
		event_dblclick,
		event_mousedown,
		event_mouseenter,
		event_mouseleave,
		event_mousemove,
		event_mouseover,
		event_mouseout,
		event_mouseup
	};

    struct mouse_event
    {
        mouse_event_type m_type;
        Point m_position;
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
	};
}