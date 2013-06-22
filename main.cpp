#include <QApplication>
#include <QFileDialog>
#include <QDebug>

#include "SimpleView.h"

void Usage(const char *n)
{
  fprintf(stderr,"Usage: %s [-show_example] [object_to_display]+\n",n);
}
 
int main( int argc, char** argv )
{
  // QT Stuff
  QApplication app( argc, argv );

  // set required fields to use QSettings API (these specify the location of the settings)
  app.setApplicationName("Sketchbio");
  app.setOrganizationName("UNC Computer Science");
  app.setOrganizationDomain("sketchbio.org");

  // Parse the non-Qt part of the command line.
  // Start with default values.
  bool  do_example = false;
  QVector<QString> object_names;
  int i = 0, real_params = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-show_example")) {
    do_example = true;
    } else if (argv[i][0] == '-' || !strcmp(argv[i], "-h")) {
    Usage(argv[0]);
    exit(0);
    } else switch (++real_params) {
      case 1:
      case 2:
      default:
	object_names.push_back(argv[i]);
    }
  }
  if (object_names.size() > 0) {
	// If we give it model names, we don't want the fibrin default.
    do_example = false;
  }
  QString projDir = QFileDialog::getExistingDirectory((QWidget *)NULL,
                               QString("Select Project Folder"),QString("./"));
  int retVal = 0;
  if (projDir.length() != 0) {

    SimpleView mySimpleView(projDir,do_example);
    mySimpleView.addObjects(object_names);
    mySimpleView.setWindowTitle("SketchBio");
    mySimpleView.show();
 
    retVal = app.exec();
  }
  return retVal;
}
