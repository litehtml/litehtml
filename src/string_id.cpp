#include "html.h"
#include <mutex>
#include <assert.h>

namespace litehtml
{

static std::map<string, string_id> map;
static std::vector<string> array;
static std::mutex mutex;

static int init()
{
	string_vector names;
	split_string(initial_string_ids, names, ",");
	for (auto& name : names)
	{
		trim(name);
		assert(name[0] == '_' && name.back() == '_');
		name = name.substr(1, name.size() - 2);				// _border_color_ -> border_color
		std::replace(name.begin(), name.end(), '_', '-');	// border_color   -> border-color
		_id(name);  // this will create association _border_color_ <-> "border-color"
	}
	return 0;
}
static int dummy = init();

string_id _id(const string& str)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto it = map.find(str);
	if (it != map.end()) return it->second;
	// else: str not found, add it to the array and the map
	array.push_back(str);
	return map[str] = (string_id)(array.size() - 1);
}

string _s(string_id id)
{
	std::lock_guard<std::mutex> lock(mutex);
	// this may fail with "vector subscript out of range" if litehtml functions are called before main
	return array[id];
}

} // namespace litehtml