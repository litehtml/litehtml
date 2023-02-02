#include <sstream>
#include <fstream>
#include "../containers/test/test_container.h"
#include "../containers/test/Bitmap.h"
using namespace std;

void error(const char* msg) { puts(msg); exit(1); }

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

	position pos = bmp.find_picture();
	bmp.resize(min(pos.right() + 8, width), min(pos.bottom() + 8, height));

	return bmp;
}

void test(string filename)
{
	string html = readfile(filename);

	int width = 1600, height = 6000; // image will be cropped to contain only the "inked" part
//	int width = 595, height = 842;
//	int width = 1190, height = 6000;
	test_container container(width, height, ".");

	auto doc = document::createFromString(html.c_str(), &container);
	doc->render(width);
	Bitmap bmp = draw(doc, width, height);

	bmp.save(filename + ".png");
}

int main(int argc, char *argv[], char **envp) {
    if( argc != 2 )
        printf("usage html2bmp filename\n");
    else
        test(argv[1]);
}
