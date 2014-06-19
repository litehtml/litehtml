#include "globals.h"

size_t curlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream )
{
	Glib::RefPtr< Gio::MemoryInputStream >* s = (Glib::RefPtr< Gio::MemoryInputStream >*) stream;
	(*s)->add_data(ptr, size * nmemb);
	return size * nmemb;
}

Glib::RefPtr< Gio::InputStream > load_file(const litehtml::tstring& url)
{
	Glib::RefPtr< Gio::MemoryInputStream > stream = Gio::MemoryInputStream::create();

	CURL* curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,	curlWriteFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA,		&stream);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	return stream;
}

std::string urljoin(const std::string &base, const std::string &relative)
{
    UriParserStateA state;
    UriUriA uriOne;
    UriUriA uriTwo;

    state.uri = &uriOne;

    if (uriParseUriA(&state, base.c_str()) != URI_SUCCESS)
    {
        return "";
    }
    state.uri = &uriTwo;
    if (uriParseUriA(&state, relative.c_str()) != URI_SUCCESS)
    {
        uriFreeUriMembersA(&uriTwo);
        return "";
    }

    UriUriA result;
    if (uriAddBaseUriA(&result, &uriTwo, &uriOne) != URI_SUCCESS)
    {
        uriFreeUriMembersA(&result);
        return "";
    }
    uriFreeUriMembersA(&uriOne);
    uriFreeUriMembersA(&uriTwo);

    int charsRequired;
    uriToStringCharsRequiredA(&result, &charsRequired);
    charsRequired++;

    char *buf = (char*) malloc(charsRequired * sizeof(char)); if (uriToStringA(buf, &result, charsRequired, NULL) != URI_SUCCESS)
        return "";
    uriFreeUriMembersA(&result);

    std::string ret(buf);
    free(buf);

    return ret;
}

void load_text_file(const litehtml::tstring& url, litehtml::tstring& out)
{
	out.clear();
	Glib::RefPtr< Gio::InputStream > stream = load_file(url);
	gsize sz;
	char buff[1025];
	while( (sz = stream->read(buff, 1024)) > 0 )
	{
		buff[sz] = 0;
		out += buff;
	}
}
