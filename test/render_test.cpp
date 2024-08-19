#include <filesystem>
#include <fstream>
#include <sstream>
#include "../containers/test/test_container.h"
using namespace std;
using namespace filesystem;

vector<string> find_htm_files(string dir);
bool test(string filename);

#if STANDALONE

#include <chrono>
using namespace chrono;

string test_dir = ".";
auto usage =
	"Usage: \n"
	"render_test [folder | html-file]+ \n"
	"render_test -d png-file png-file ";

#define RED(str)   "\33[91m" str "\33[0m"
#define GREEN(str) "\33[92m" str "\33[0m"
void error(string msg) { printf(RED("%s\n"), msg.c_str()); exit(1); }

int main(int argc, char* argv[])
{
	auto start = steady_clock::now();

	// Using a hack to enable ANSI escape sequences in a Windows console.
	// Whenif it stops working ENABLE_VIRTUAL_TERMINAL_PROCESSING can be used instead.
	system(" ");

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

	vector<string> files;
	for (int i = 1; i < argc; i++)
	{
		if (is_directory(argv[i]))
			files += find_htm_files(argv[i]);
		else if (exists(argv[i]))
			files.push_back(argv[i]);
		else
			printf(RED("%s not found\n"), argv[i]);
	}

	if (files.empty())
		error("No html files found");

	vector<string> failed_tests;
	for (auto file : files)
	{
		if (test(file))
			printf(GREEN("pass")" %s\n", file.c_str());
		else
			printf(RED("FAILURE %s\n"), file.c_str()),
			failed_tests.push_back(file);
	}

	if (failed_tests.empty())
		printf(GREEN("\nAll tests passed\n"));
	else
	{
		int count = (int)failed_tests.size();
		printf(RED("\n%d %s FAILED:\n"), count, count == 1 ? "test" : "tests");
		for (auto file : failed_tests)
			printf(RED("%s\n"), file.c_str());
	}

	auto stop = steady_clock::now();
	printf("\nTime: %d sec\n", (int)((stop - start).count() / 1'000'000'000));
}

#else

#include <gtest/gtest.h>

string test_dir = "../test/render"; // ctest is run from litehtml/build

using render_test = testing::TestWithParam<string>;

TEST_P(render_test, _)
{
	ASSERT_TRUE(test(GetParam()));
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
			if (name[0] != '-' && name != "support")
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

bool test(string filename)
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

	bool pass = bmp == Bitmap(filename + ".png");
	if (!pass)
		bmp.save(filename + "-FAILED.png");
	return pass;
}
