#include <filesystem>
#include <fstream>
#include <sstream>
#if STANDALONE
	#define ASSERT_TRUE(x)
#else
	#include <gtest/gtest.h>
#endif
#include "../containers/test/test_container.h"
using namespace std;
using namespace filesystem;

vector<string> find_htm_files(string dir);
void test(string filename);
void error(string msg) { puts(msg.c_str()); exit(1); }

#if STANDALONE

#include <chrono>
using namespace chrono;
string test_dir = ".";
vector<string> failed_tests;
auto usage =
	"Usage: \n"
	"render_test [folder | html-file]+ \n"
	"render_test -d png-file png-file ";

int main(int argc, char* argv[])
{
	auto start = steady_clock::now();

	vector<string> files;
	if (argc == 1) error(usage);
	else if (string(argv[1]) == "-d")
	{
		if (argc != 4) error(usage);
		string pngfile1 = argv[2];
		string pngfile2 = argv[3];
		if (!exists(pngfile1)) error(pngfile1 + " not found");
		if (!exists(pngfile2)) error(pngfile2 + " not found");
		printf("max_color_diff = %d", max_color_diff(pngfile1, pngfile2));
		return 0;
	}
	else
	{
		for (int i = 1; i < argc; i++)
		{
			if (is_directory(argv[i]))
				files += find_htm_files(argv[i]);
			else if (exists(argv[i]))
				files.push_back(argv[i]);
			else
				printf("%s not found\n", argv[i]);
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
	for (auto entry : directory_iterator(dir))
	{
		string name = entry.path().filename().string();
		if (entry.is_directory())
		{
			if (name[0] != '-')
			{
				files += find_htm_files(dir + "/" + name);
			}
		} else
		{
			if (name[0] != '-' && is_one_of(path(name).extension(), ".htm", ".html"))
				files.push_back(dir + "/" + name);
		}
	}
	sort(files);
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

	string base_path = path(filename).parent_path().string();
	if (base_path == "") base_path = test_dir;

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
