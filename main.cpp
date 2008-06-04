#include <iostream>
#include <fstream>
#include <string>

#include <QProcess>
#include <QVariant>
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QDir>

typedef QMap<QString, QString> Config;
typedef QMap<QString, QString> ProcList;

QString userName()
{
   QProcess proc;
   proc.start("whoami");
   proc.waitForFinished(2000);
   return proc.readAllStandardOutput().trimmed();
}

bool loadFile(const QString &fileName, Config &config)
{
   using namespace std;
   ifstream f(fileName.toLatin1());
   if( f )
   {
      string sline;
      while( getline(f, sline) )
      {
         string line(QString::fromStdString(sline).trimmed().toStdString());
         if( line.empty() )
            continue;
         if( line[0] == '#' )
            continue;
         string::size_type p = line.find('=');
         if( p == string::npos )
            continue; // ill-formed
         QString key = QString::fromStdString(line.substr(0,p)).trimmed(),
                 val = QString::fromStdString(line.substr(p+1)).trimmed();
         config[key] = val;
      }
      return true;
   } else {
      qDebug() << "Couldn't open file " << fileName << ".";
      return false;
   }
}

void writeFile(const QString fileName, Config &config)
{
   std::ofstream file(fileName.toLatin1());
   for(Config::const_iterator i = config.begin(); i != config.end(); ++i)
   {
      QString line = QString("%1=%2\n").arg(i.key(), i.value());
      file.write(line.toLatin1(), line.length());
   }
}

int startProcess(const QString &cmd)
{
   QStringList args = cmd.split(" ", QString::SkipEmptyParts);
   if( args.count() <= 0 )
      return 0;
   QString app = args[0];
   args.erase(args.begin());
   qint64 pid = 0;
   qDebug() << "Starting app" << app << "with params" << args.join("\" \"");
   QProcess::startDetached(app, args, QString(), &pid);
   qDebug() << "pid: " << pid;
   return pid;
}

QString findWindowForPid(int pid)
{
   QProcess proc;
   QStringList args;
   args << "-p" << "-l";
   proc.start("wmctrl", args);
   proc.waitForFinished();
   QList<QByteArray> lines = proc.readAllStandardOutput().split('\n');
   QString spid = qvariant_cast<QString>(pid);
   for(QList<QByteArray>::const_iterator i = lines.begin(); i != lines.end(); ++i)
   {
      if( i->isEmpty() )
         continue;
      QStringList fields = QString(*i).split(' ', QString::SkipEmptyParts);
      if( fields.count() < 3 )
         continue;
      QString pidField = fields[2];
      qDebug() << pidField << spid;
      if(  pidField == spid )
         return QString(fields[0]);
   }
   return QString();
}

int main(int argc, char *argv[])
{
   if( argc != 2 )
   {
      std::cout
         << "xstartonce 0.0.2\n"
         << "Usage: xstartonce <hotkey-name>\n"
         << "You need to define hotkeys in ~/.xstartonce in the format \"name=command --params\".";
      return 1;
   }
   Config config;
   loadFile(QDir::home().absoluteFilePath(".xstartonce"), config);
   QString dbPath = QDir::temp().absoluteFilePath("xstartonce-db.%1").arg(userName());
   for(Config::const_iterator i = config.begin(); i != config.end(); ++i)
   {
      qDebug() << i.key();
      if( i.key() == argv[1] )
      {
         qDebug() << "Found key" << i.key();
         ProcList procs;
         if( loadFile(dbPath, procs) )
            for(ProcList::const_iterator j = procs.begin(); j != procs.end(); ++j)
            {
               if( j.key() == i.key() )
               {
                  QString winId = findWindowForPid(qvariant_cast<int>(j.value()));
                  if( !winId.isEmpty() ) 
                  {
                     QProcess proc;
                     QStringList args;
                     args << "-i" << "-a" << winId;
                     qDebug() << "Activating window" << winId;
                     proc.start("wmctrl", args);
                     proc.waitForFinished(2000);
                     return 0;
                  } else {
                     qDebug() << "Could not find window for pid" << j.value();
                     break;
                  }
               }
            }
         int i_pid = startProcess(i.value());
         if( i_pid > 0 )
         {
            QString pid = qvariant_cast<QString>(i_pid);
            procs[i.key()] = pid;
            writeFile(dbPath, procs);
         }
         return 0;
      }
   }
   qDebug() << "No such configuration " << argv[1] << ".";
   return 3;
}
