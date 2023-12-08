#define _CRT_SECURE_NO_WARNINGS
#include <gtest/gtest.h>
#include <fstream>
#ifdef _WIN32
	#include "dirent.h"
#else
	#include <dirent.h>
#endif
#include "../containers/test/test_container.h"
#include "../containers/test/Bitmap.h"
using namespace std;

vector<string> find_htm_files();
void test(string filename);

const char* test_dir = "../test/render/"; // ctest is run from litehtml/build

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
using render_test = testing::TestWithParam<string>;

TEST_P(render_test, _)
{
	test(test_dir + GetParam());
}

INSTANTIATE_TEST_SUITE_P(, render_test, testing::ValuesIn(find_htm_files()));
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void error(const char* msg) { puts(msg); exit(1); }

vector<string> find_htm_files()
{
	DIR* dir = opendir(test_dir);
	if (!dir) error("Cannot read test directory");
	vector<string> ret;
	while (dirent* ent = readdir(dir))
	{
		if (ent->d_type != DT_REG) continue; // if not regular file
		string name = ent->d_name;
		if (name[0] != '-' && name.size() > 4 && name.substr(name.size() - 4) == ".htm")
			ret.push_back(name);
	}
	closedir(dir);
	sort(ret.begin(), ret.end());
	return ret;
}

string readfile(string filename)
{
	stringstream ss;
	ifstream(filename) >> ss.rdbuf();
	return ss.str();
}

Bitmap draw(document::ptr doc, int width, int height)
{
	Bitmap bmp(width, height);
	position clip(0, 0, width, height);

	doc->draw((uint_ptr)&bmp, 0, 0, &clip);

	bmp.resize(width, height);

	return bmp;
}

void test(string filename)
{
	string html = readfile(filename);

	int width = 800, height = 1600; // image will be cropped to content_width/content_height
	test_container container(width, height, test_dir);

	auto doc = document::createFromString(html.c_str(), &container);
	doc->render(width);
	Bitmap bmp = draw(doc, doc->content_width(), doc->content_height());

	Bitmap good(filename + ".png");
	if (bmp != good)
	{
		bmp.save(filename + "-FAILED.png");
		ASSERT_TRUE(false);
	}
}
