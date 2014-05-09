#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QDebug>

#include "SimpleView.h"

void Usage(const char *n)
{
    fprintf(stderr,"Usage: %s [-show_example] [-f deviceFile]\n",n);
}
 
int main( int argc, char** argv )
{
  // QT Stuff
  QApplication app( argc, argv );

  // set required fields to use QSettings API (these specify the location of the settings)
  app.setApplicationName("Sketchbio");
  app.setOrganizationName("UNC Computer Science");
  app.setOrganizationDomain("sketchbio.org");

  // fix current directory for Visual Studio/XCode and other IDES that put executables in
  // folders based on build type.  This makes the default location of the device config files
  // correct.
  QString path = app.applicationDirPath();
  if (path.endsWith("/Release") || path.endsWith("/Debug") || path.endsWith("RelWithDebInfo")) {
	  QDir::setCurrent(path.mid(0,path.lastIndexOf("/")));
  }

  // Parse the non-Qt part of the command line.
  // Start with default values.
  bool  do_example = false;
  const char *deviceFile = "devices/razer_hydra.xml";
  QVector<QString> object_names;
  int i = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-show_example")) {
    do_example = true;
    } else if (!strcmp(argv[i], "-f")) {
        if (i+1 < argc) {
            deviceFile = argv[i+1];
        } else {
            fprintf(stderr, "No device filename specified with -f\n");
            Usage(argv[0]);
            exit(1);
        }
    } else if (argv[i][0] == '-' || !strcmp(argv[i], "-h")) {
    Usage(argv[0]);
    exit(0);
    } else {
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

    SimpleView mySimpleView(projDir,do_example,deviceFile);
    mySimpleView.setWindowTitle("SketchBio");
    mySimpleView.show();
 
    retVal = app.exec();
  }
  return retVal;
}
