#include <QCoreApplication>
#include <QSettings>


int main(int argc, char *argv[])
{
  // QT Stuff
  QCoreApplication app( argc, argv );

  // set required fields to use QSettings API (these specify the location of the settings)
  app.setApplicationName("Sketchbio");
  app.setOrganizationName("UNC Computer Science");
  app.setOrganizationDomain("sketchbio.org");

  QSettings settings;
  settings.clear();

  return 0;
 
}
