#define _CRT_SECURE_NO_WARNINGS
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#if STANDALONE
	#define ASSERT_TRUE(x)
#else
	#include <gtest/gtest.h>
#endif
#ifdef _WIN32
	#define NOMINMAX
	#include "dirent.h"
#else
	#include <dirent.h>
#endif
#include "../containers/test/test_container.h"
using namespace std;

vector<string> find_htm_files(string dir);
void test(string filename);
void error(const char* msg) { puts(msg); exit(1); }

#if STANDALONE

#include <chrono>
using namespace chrono;
string test_dir = ".";
vector<string> failed_tests;

int main(int argc, char* argv[])
{
	auto start = steady_clock::now();

	vector<string> files;
	if (argc == 1)
		files = find_htm_files(test_dir);
	else if (argc == 2 && string(argv[1]) == "-h")
		error("Usage: render_test [folder | html-file]*");
	else
	{
		for (int i = 1; i < argc; i++)
		{
			struct stat st;
			if (stat(argv[i], &st) != 0)
			{
				printf("%s not found\n", argv[i]);
				continue;
			}
			if (S_ISDIR(st.st_mode))
				files += find_htm_files(argv[i]);
			else
				files.push_back(argv[i]);
		}
	}

	if (files.empty())
		error("No html files found");

	for (auto file : files)
		test(file);

	if (failed_tests.empty())
		puts("\nAll tests passed");
	else
	{
		auto count = failed_tests.size();
		printf("\n%Id %s FAILED:\n", count, count == 1 ? "test" : "tests");
		for (auto file : failed_tests)
			puts(file.c_str());
	}

	auto stop = steady_clock::now();
	printf("\nTime: %Id sec", (stop - start).count() / 1'000'000'000);
}

#else

string test_dir = "../test/render"; // ctest is run from litehtml/build

using render_test = testing::TestWithParam<string>;

TEST_P(render_test, _)
{
	test(GetParam());
}

INSTANTIATE_TEST_SUITE_P(, render_test, testing::ValuesIn(find_htm_files(test_dir)));

#endif // STANDALONE

vector<string> find_htm_files(string dir)
{
	vector<string> files;
	DIR* _dir = opendir(dir.c_str());
	if (!_dir) error(dir.c_str());
	while (dirent* ent = readdir(_dir))
	{
		string name = ent->d_name;
		if (ent->d_type == DT_DIR)
		{
			if (name != "." && name != ".." && name[0] != '-')
			{
				files += find_htm_files(dir + "/" + name);
			}
		} else if (ent->d_type == DT_REG)
		{
			if (name[0] != '-' && name.size() > 4 &&
				(name.substr(name.size() - 4) == ".htm" || name.substr(name.size() - 5) == ".html"))
				files.push_back(dir + "/" + name);
		}
	}
	closedir(_dir);
	sort(files.begin(), files.end());
	return files;
}

string readfile(string filename)
{
	stringstream ss;
	ifstream(filename, ios::binary) >> ss.rdbuf();
	return ss.str();
}

Bitmap draw(document::ptr doc, int width, int height)
{
	canvas canvas(width, height, rgba(1,1,1,1));
	rect clip(0, 0, width, height);

	doc->draw((uint_ptr)&canvas, 0, 0, &clip);

	return Bitmap(canvas);
}

void test(string filename)
{
	string html = readfile(filename);

	auto last_slash_pos = filename.find_last_of("\\/");
	string base_path;
	if(last_slash_pos != string::npos)
	{
		base_path = filename.substr(0, last_slash_pos);
	} else
	{
		base_path = test_dir;
	}
	// image size will be {content_width, content_height} (calculated after layout)
	// height is nonzero to get finite aspect-ratio media feature
	int width = 800, height = 1;
	test_container container(width, height, base_path);

	auto doc = document::createFromString(html, &container);
	doc->render(width);
	Bitmap bmp = draw(doc, doc->content_width(), doc->content_height());

	Bitmap good(filename + ".png");
	bool failed = bmp != good;
	if (failed)
	{
		bmp.save(filename + "-FAILED.png");
		ASSERT_TRUE(false);
	}
#if STANDALONE
	auto result = "pass";
	if (failed)
	{
		result = "FAILURE";
		failed_tests.push_back(filename);
	}
	printf("%s %s\n", result, filename.c_str());
#endif
}
