#include <iostream>
#include <string>

#include <QProcess>
#include <QVector>
#include <QDebug>

/*QString readField(QString &str)
{
   std::string s(str.toStdString());
   std::string::size_type p = s.find_first_not_of(" ");
   if( p != std::string::npos )
   {
      str.
   }
}*/

QString getBaseName(const QString &file)
{
   QStringList args;
   args << file;
   QProcess proc;
   proc.start("basename",args);
   proc.waitForFinished(10000);
   return proc.readAllStandardOutput().trimmed();
}

struct ProcInfo
{
   QString windowId;
   int procId;
};

void startProcess(int argc, char *argv[])
{
   QStringList args;
   for(int i = 4; i < argc; ++i)
      args << argv[i];
   qDebug() << "starting " << argv[1] << args.join("' '");
   QProcess::startDetached(argv[1], args);
}

int main(int argc, char *argv[])
{
   if( argc < 2 )
   {
      std::cout
         << "xstartonce 0.0.1\n"
         << "Usage: xstartonce <application> [process-name]";
   }
   QProcess proc;
   QStringList args;
   qDebug() << argc;
   if( argc == 4 )
   {
      args << "-a" << argv[3];
      proc.start("wmctrl", args);
      proc.waitForFinished();
      if( proc.exitCode() != 0 )
         startProcess(argc, argv);
      return 0;
   }
   QString baseName;
   if( argc == 3 )
      baseName = getBaseName(argv[2]);
   else
      baseName = getBaseName(argv[1]);
   qDebug() << "basename:" << baseName;
   args.clear();
   args << "-l" << "-p";
   proc.start("wmctrl",args);
   proc.waitForFinished(10000);
   QString out = proc.readAllStandardOutput();
   QStringList lines = out.split("\n");
   QVector<ProcInfo> procs;
   for(QStringList::const_iterator i = lines.begin(); i != lines.end(); ++i)
   {
      if( i->isEmpty() )
         continue;
      QStringList fields = i->split(" ", QString::SkipEmptyParts);
      if( fields.count() < 5 )
      {
         qDebug() << "The line" << *i << "from 'wmctrl -l -p' is ill-formed";
         continue;
      }
      ProcInfo inf;
      inf.windowId = fields[0];
      inf.procId   = fields[2].toInt();
      procs.push_back(inf);
   }
   args.clear();
   args << "ax";
   proc.start("ps", args);
   proc.waitForFinished(10000);
   out = proc.readAllStandardOutput();
   lines = out.split("\n");
   for(QStringList::const_iterator i = lines.begin(); i != lines.end(); ++i)
   {
      if( i->isEmpty() )
         continue;
      QStringList fields = i->split(" ", QString::SkipEmptyParts);
      if( fields.count() < 4 )
      {
         qDebug() << "The line " << *i << "from 'ps ax' is ill-formed";
         continue;
      }
      int procId = fields[0].toInt();
      for(QVector<ProcInfo>::iterator j = procs.begin(); j != procs.end(); ++j)
      {
         if( j->procId == procId )
         {
            QString procBaseName = getBaseName(fields[4]);
            qDebug() << "Found matching process ID:" << procBaseName;
            if( procBaseName == baseName )
            {
               args.clear();
               args << "-i" << "-a" << j->windowId;
               proc.start("wmctrl",args);
               proc.waitForFinished(10000);
               return 0;
            }
         }
      }
   }
   startProcess(argc, argv);
}

