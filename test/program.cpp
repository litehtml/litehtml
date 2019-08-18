#include "litehtml.h"

extern const litehtml::tchar_t master_css[] = 
{
#include "master.css.inc"
,0
};

void contextTest();
void cssTest();
void documentTest();
void layoutGlobalTest();
void mediaQueryTest();
void webColorTest();

#if _HASPAUSE
#define mainPause(fmt) { printf(fmt"\n"); char c; scanf("%c", &c); }
#else
#define mainPause(fmt) { printf(fmt"\n"); }
#endif

int main(int argc, char **argv) {
	int testId = argv[1] ? atoi(argv[1]) : 1;
	// Launch test
	switch (testId) {
	case 0: mainPause("Press any key to continue."); break;
	case 1: contextTest(); break;
	case 2: cssTest(); break;
	case 3: documentTest(); break;
	case 4: layoutGlobalTest(); break;
	case 5: mediaQueryTest(); break;
	case 6: webColorTest(); break;
	default: mainPause("Unknown test."); break;
	}
	return 0;
}