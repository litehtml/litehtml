#include "html.h"
#include "event_handler.h"
#include "element.h"
#include <assert.h>

namespace litehtml
{
	struct event_info
	{
		mouse_event_type m_type;
		bool m_bubbles, m_cancelable;
	};

	static const event_info event_info_table[] = {
		{ mouse_event_click, true, true },
		{ mouse_event_contextmenu, true, true },
		{ mouse_event_dblclick, true, true },
		{ mouse_event_mousedown, true, true },
		{ mouse_event_mouseenter, false, false },
		{ mouse_event_mouseleave, false, false },
		{ mouse_event_mousemove, true, true },
		{ mouse_event_mouseover, true, true },
		{ mouse_event_mouseout, true, true },
		{ mouse_event_mouseup, true, true }
	};

	static_assert( sizeof( event_info_table) / sizeof( event_info ) == mouse_event_invalid, "event_info_table out of sync with mouse_event_type enum" );
}


void litehtml::event_handler::handle_mouse_event( element & el, const mouse_event & event, bool & prevent_default )
{
	const event_info & info = event_info_table[ event.m_type ];
	assert( info.m_type == event.m_type );

	if ( info.m_bubbles )
	{
		element* current_el = &el;

		while( current_el )
		{
			event_response response;

			assert( current_el->get_pointer_events() != pointer_events_none );

			on_mouse_event( *current_el, response, event );
			prevent_default |= response.m_prevent_default && info.m_cancelable;

			if( response.m_stop_propagation )
			{
				break;
			}

			current_el = current_el->parent();
		}
	}
	else
	{
		event_response response;

		on_mouse_event( el, response, event );
		prevent_default |= response.m_prevent_default && info.m_cancelable;
	}
}
