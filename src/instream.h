#pragma once

namespace litehtml 
{
	struct instream 
	{
		virtual ucode_t get_char() = 0;
		virtual void	ucode_to_chars(ucode_t wch, tstring& dst) = 0;
	};

	class str_instream: public instream
	{
	private:
		const tchar_t* m_str;

	public:
		str_instream(const tchar_t* src);

		virtual ucode_t get_char();
		virtual void ucode_to_chars(ucode_t wch, tstring& dst);
	};

	class utf8_instream: public instream
	{
	private:
		const byte* m_str;

	public:
		utf8_instream(const byte* src);

		virtual ucode_t get_char();
		virtual void ucode_to_chars(ucode_t code, tstring& dst);

	private:
		ucode_t getb()
		{
			if(!(*m_str)) return 0;
			return *m_str++;
		}
		ucode_t get_next_utf8(ucode_t val)
		{
			return (val & 0x3f);
		}
	};

}