#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include "fonts.h"
#include "render2png.h"
#include "litehtml/html.h"

std::vector<std::string> find_htm_files(std::string dir);
bool test(std::string filename);

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

std::string test_dir = "../test/render"; // ctest is run from litehtml/build

using render_test = testing::TestWithParam<std::string>;

TEST_P(render_test, _)
{
	ASSERT_TRUE(test(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(, render_test, testing::ValuesIn(find_htm_files("")));

#endif // STANDALONE

std::vector<std::string> find_htm_files(std::string dir)
{
	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::directory_iterator(test_dir + "/" + dir))
	{
		std::string name = entry.path().filename().string();
		std::string path;
		if(dir.empty())
		{
			path = name;
		} else
		{
			path = dir + "/" + name;
		}
		if (entry.is_directory())
		{
			if (name[0] != '-' && name != "support")
			{
				auto tmp_files = find_htm_files(path);
				files.insert(files.end(), tmp_files.begin(), tmp_files.end());
			}
		} else
		{
			if (name[0] != '-' && litehtml::is_one_of(std::filesystem::path(name).extension(), ".htm", ".html"))
				files.push_back(path);
		}
	}
	std::sort(files.begin(), files.end());
	return files;
}

bool test(std::string filename)
{
	std::string html_path = test_dir + "/" + filename;
	std::string png_path = test_dir + "/" + filename + ".png";
	std::string failed_png_path = test_dir + "/" + filename + "-FAILED.png";

	auto font_options = prepare_fonts_for_testing();

	html2png::converter converter(800, 600, 96, "serif", font_options);
	auto pixbuf = converter.to_pixbuf(html_path);

	if(font_options)
	{
		cairo_font_options_destroy(font_options);
	}

	bool res = html2png::pngcmp(pixbuf, png_path) == html2png::png_diff_same;
	if(!res)
	{
		gdk_pixbuf_save(pixbuf, failed_png_path.c_str(), "png", nullptr, nullptr);
	}
	g_object_unref(pixbuf);
	return res;
}
