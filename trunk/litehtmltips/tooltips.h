#pragma once
#include <litehtml.h>
#include <cairo_container.h>

namespace litehtml
{
	enum tips_style
	{
		tips_style_square,
		tips_style_rounded,
		tips_style_baloon
	};

	enum tool_options
	{
		tool_opt_align_top		= 0x0000,
		tool_opt_align_bottom	= 0x0001,
		tool_opt_align_left		= 0x0002,
		tool_opt_align_right	= 0x0003,
		tool_opt_align_mask		= 0x000F,

		tool_opt_ask_text		= 0x0010
	};

	class tooltips_callback
	{
	public:
		virtual CTxDIB* ttcb_get_image(unsigned int id, LPCWSTR url) = 0;
		virtual void ttcb_get_text(unsigned int id, std::wstring& text) = 0;
	};

	struct tip_layout
	{
		int			x;
		int			y;
		int			width;
		int			height;
		int			content_x;
		int			content_y;
		int			content_height;
		int			content_width;
		int			anchor_x;
		int			anchor_y;
		UINT		align;
		tips_style	style;
	};

	struct tool
	{
		typedef std::map<unsigned int, tool>	map;

		std::wstring	text;
		HWND			hWnd;
		RECT			rc_tool;
		UINT			options;

		tool()
		{
			hWnd	= 0;
			options = 0;
			ZeroMemory(&rc_tool, sizeof(rc_tool));
		}

		tool(const tool& val)
		{
			text	= val.text;
			hWnd	= val.hWnd;
			rc_tool	= val.rc_tool;
			options	= val.options;
		}

		tool(const wchar_t* txt, HWND wnd, LPCRECT rc, UINT opt)
		{
			if(txt)
			{
				text	= txt;
			}
			hWnd	= wnd;
			rc_tool	= *rc;
			options	= opt;
		}

		void operator=(const tool& val)
		{
			text	= val.text;
			hWnd	= val.hWnd;
			rc_tool	= val.rc_tool;
			options	= val.options;
		}
	};

	struct tooltips_bg_cache
	{
		simpledib::dib		m_dib;
		cairo_surface_t*	m_surface;
		tip_layout			m_layout;
		COLORREF			m_clr_border;
		
		tooltips_bg_cache()
		{
			ZeroMemory(&m_layout, sizeof(m_layout));
			m_surface		= NULL;
			m_clr_border	= 0;
		}
		~tooltips_bg_cache()
		{
			clear();
		}

		bool need_redraw(tip_layout* layout);
		void draw(cairo_t* cr, tip_layout* layout, HWND hWnd, BYTE alpha);
		void fastbluralpha(LPRGBQUAD pixels, int width, int height, int radius);

		void clear()
		{
			if(m_surface)
			{
				cairo_surface_destroy(m_surface);
				m_surface = NULL;
			}
			m_dib.clear();
		}
	};

	class tooltips : public cairo_container
	{
		simpledib::dib				m_dib;
		cairo_t*					m_cr;
		cairo_surface_t*			m_surface;
		tooltips_bg_cache			m_bg_cache;

		HINSTANCE					m_hInst;
		HWND						m_hWnd;
		HWND						m_hWndParent;
		tooltips_callback*			m_callback;
		tool::map					m_tools;
		litehtml::document::ptr		m_html;
		litehtml::context*			m_html_context;
		int							m_max_width;
		int							m_show_time;
		int							m_hide_time;
		tips_style					m_style;
		POINT						m_mouse_pos;
		unsigned int				m_over_tool;
		unsigned int				m_show_tool;
		unsigned int				m_last_shown_tool;
		unsigned int				m_cached_tool;
		std::wstring				m_def_font_name;
		int							m_def_font_size;
		bool						m_disabled;
		BYTE						m_alpha;
		tip_layout					m_layout;
		int							m_top;
		bool						m_mouse_hover_on;
	public:
		tooltips(HINSTANCE hInst, litehtml::context* html_context);
		virtual ~tooltips(void);

		void add_tool(unsigned int id, const wchar_t* text, HWND ctl, LPCRECT rc_tool, UINT options);
		void clear();
		void create(HWND parent);
		void show(unsigned int id, int top = 0, bool is_update = false, bool re_render = false);

		void hide();
		void set_style(tips_style style)			{ m_style = style;		}
		void disable(bool val);
		void set_max_width(int width)				{ m_max_width = width;	}
		void set_show_time(int t)					{ m_show_time = t;		}
		void set_hide_time(int t)					{ m_hide_time = t;		}
		void set_callback(tooltips_callback* cb)	{ m_callback = cb;		}
		void set_alpha(int alpha)					{ m_alpha = alpha;		}
		void update(unsigned int id, bool re_render);
		void set_def_font(const wchar_t* font_name, int font_size);

		static void	rounded_rect( cairo_t* cr, int x, int y, int width, int height, int radius, int line_width );
		static void	baloon( cairo_t* cr, int x, int y, int width, int height, int ax, int ay, UINT align, int radius, int line_width );

	private:
		// cairo_container members
		virtual void		make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
		virtual CTxDIB*		get_image(LPCWSTR url, bool redraw_on_ready);

		// litehtml::document_container members
		virtual	void		set_caption(const wchar_t* caption);
		virtual	void		set_base_url(const wchar_t* base_url);
		virtual	void		link(litehtml::document* doc, litehtml::element::ptr el);
		virtual void		import_css(std::wstring& text, const std::wstring& url, std::wstring& baseurl, const litehtml::string_vector& media);
		virtual	void		on_anchor_click(const wchar_t* url, litehtml::element::ptr el);
		virtual	void		set_cursor(const wchar_t* cursor);
		virtual void		get_client_rect(litehtml::position& client);
		virtual int			get_default_font_size();
		virtual const wchar_t*	get_default_font_name();

		virtual LRESULT		OnMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		void				registerClass(HINSTANCE hInstance);
		int					tip_width();
		int					tip_height();
		void				content_point(LPPOINT pt);
		void				draw_background(tip_layout* layout);
		void				stop_timers();
		unsigned int		find_tool(int x, int y);
		void				calc_layout(tool* t, tip_layout* layout);
		void				calc_position(UINT align, LPRECT rc_tool, tip_layout* layout, bool second = false);
		void				GetDesktopRect(RECT* rcDsk, HWND hWnd);
		void				init_def_font();
		void				create_dib(int width, int height);
		void				draw_window(BOOL clr = FALSE);
		BOOL				scroll(int dx);
		BOOL				can_scroll();
		void				start_hover_tracking(bool start);
	};
}
