#include <QApplication>
#include "SimpleView.h"

void Usage(const char *n)
{
  fprintf(stderr,"Usage: %s [-nofibrin] [-nofibrinsprings] [-noreplicate] [object_to_display]+\n",n);
}
 
int main( int argc, char** argv )
{
  // QT Stuff
  QApplication app( argc, argv );

  // Parse the non-Qt part of the command line.
  // Start with default values.
  bool  do_fibrin = true;
  bool	do_fibrin_springs = true;
  bool	do_fibrin_replicate = true;
  QVector<QString> object_names;
  unsigned i = 0, real_params = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-nofibrin")) {
	do_fibrin = false;
    } else if (!strcmp(argv[i], "-nofibrinsprings")) {
	do_fibrin_springs = false;
    } else if (!strcmp(argv[i], "-nofibrinreplicate")) {
	do_fibrin_replicate = false;
    } else if (argv[i][0] == '-') {
	Usage(argv[0]);
    } else switch (++real_params) {
      default:
	object_names.push_back(argv[i]);
    }
  }
  if (object_names.size() > 0) {
	// If we give it model names, we don't want the fibrin default.
	do_fibrin = false;
	do_fibrin_springs = false;
	do_fibrin_replicate = false;
  }

  SimpleView mySimpleView(do_fibrin, do_fibrin_springs, do_fibrin_replicate);
  mySimpleView.addObjects(object_names);
  mySimpleView.setWindowTitle("SketchBio");
  mySimpleView.show();
 
  return app.exec();
}
