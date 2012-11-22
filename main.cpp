#include <QApplication>
#include "SimpleView.h"

void Usage(const char *n)
{
  fprintf(stderr,"Usage: %s [-nofibrin] [-nofibrinsprings] [-noreplicate]\n",n);
}
 
int main( int argc, char** argv )
{
  // QT Stuff
  QApplication app( argc, argv );

  // Parse the non-Qt part of the command line.
  // Start with default values.
  bool  do_fibrin = true;
  bool	do_fibrin_springs = true;
  bool	do_replicate = true;
  unsigned i = 0, real_params = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-nofibrin")) {
	do_fibrin = false;
    } else if (!strcmp(argv[i], "-nofibrinsprings")) {
	do_fibrin_springs = false;
    } else if (!strcmp(argv[i], "-noreplicate")) {
	do_replicate = false;
    } else switch (++real_params) {
	Usage(argv[0]);
	return -1;
    }
  }
 
  SimpleView mySimpleView(do_fibrin, do_fibrin_springs, do_replicate);
  mySimpleView.setWindowTitle("SketchBio");
  mySimpleView.show();
 
  return app.exec();
}
