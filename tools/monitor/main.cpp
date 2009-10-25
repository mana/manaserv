/**
 * A monitor application that runs a set of servers.
 * (C) 2009  Thorbjørn Lindeijer
 *
 * When a server crashes, a gdb process is spawned to create a backtrace,
 * which is emailed when the CRASH_REPORT_RECEIVER environment variable
 * is set. The crashed server is restarted.
 */

#include <QtCore>

#include <signal.h>
#include <sys/socket.h>

/**
 * A thread that keeps a server running. It restarts the server when it quits
 * for some external reason, and sends out backtraces when appropriate.
 */
class ServerThread : public QThread
{
public:
    ServerThread(const QString &executable, QObject *parent = 0)
        : QThread(parent)
        , mProcess(0)
        , mExecutable(executable)
        , mRunning(false)
    {}

    void runServer();
    void stopServer();

protected:
    void run();

private:
    void startProcess();
    void maybeSendBacktrace();

    QProcess *mProcess;
    QString mExecutable;
    QTime mLastCrash;
    bool mRunning;
};

void ServerThread::runServer()
{
    Q_ASSERT(!mRunning);
    if (!QFile::exists(mExecutable)) {
        qDebug() << mExecutable << "not found";
        return;
    }
    mRunning = true;
    start();
}

void ServerThread::run()
{
    mProcess = new QProcess;
    startProcess();

    while (mRunning) {
        mProcess->waitForFinished(1000);

        if (mRunning && mProcess->state() == QProcess::NotRunning) {
            /* The process stopped without being requested via the monitor.
             * Check for crash and send report when appropriate, then restart
             * the process.
             */
            qDebug() << mExecutable << "terminated unexpectedly";
            maybeSendBacktrace();

            qDebug() << "Restarting" << mExecutable << "in 3 seconds...";
            sleep(3);
            startProcess();
        }

        if (!mRunning && mProcess->state() == QProcess::Running) {
            // Need to shut down the process
            qDebug() << "Terminating" << mExecutable;
            mProcess->terminate();
            if (!mProcess->waitForFinished(3000)) {
                qDebug() << mExecutable << "didn't terminate within 3 seconds,"
                        " killing";
                mProcess->kill();
            }
            break;
        }
    }

    delete mProcess;
    mProcess = 0;
}

void ServerThread::startProcess()
{
    mProcess->start(mExecutable);

    // Give the process 3 seconds to start up
    if (!mProcess->waitForStarted(3000)) {
        qDebug() << "Failed to start" << mExecutable;
        mRunning = false;
    } else {
        qDebug() << mExecutable << "started";
    }
}

void ServerThread::maybeSendBacktrace()
{
    QFile coreFile("core");

    if (!coreFile.exists()) {
        qDebug() << "No core dump found";
        return;
    }

    char *receiver = getenv("CRASH_REPORT_RECEIVER");
    if (!receiver) {
        qDebug() << "CRASH_REPORT_RECEIVER environment variable not set";
        return;
    }

    QProcess gdb;
    gdb.start(QString("gdb %1 core -q -x print-backtrace.gdb")
            .arg(mExecutable));

    if (!gdb.waitForStarted()) {
        qDebug() << "Failed to launch gdb";
        coreFile.remove();
        return;
    }

    if (!gdb.waitForFinished()) {
        qDebug() << "gdb process is not finishing, killing";
        gdb.kill();
        coreFile.remove();
        return;
    }

    coreFile.remove();

    const QByteArray gdbOutput = gdb.readAllStandardOutput();
    qDebug() << "gdb output:\n" << gdbOutput.constData();

    QTime current = QTime::currentTime();
    if (!mLastCrash.isNull() && mLastCrash.secsTo(current) < 60 * 10) {
        qDebug() << "Sent a crash report less than 10 minutes ago, "
            "dropping this one";
        return;
    }

    mLastCrash = current;

    QProcess mail;
    mail.start(QString("mail -s \"Crash report for %1\" %2")
            .arg(mExecutable, QString::fromLocal8Bit(receiver)));

    if (!mail.waitForStarted()) {
        qDebug() << "Failed to launch mail";
        return;
    }

    mail.write(gdbOutput);
    mail.closeWriteChannel();

    if (mail.waitForFinished()) {
        qDebug() << "Crash report sent to" << receiver;
    } else {
        qDebug() << "mail process is not finishing, killing";
        mail.kill();
    }
}


void ServerThread::stopServer()
{
    mRunning = false;
    wait();
}

class ServerMonitor : public QObject
{
    Q_OBJECT

public:
    ServerMonitor(const QStringList &serverExecutables);

    void start()
    {
        foreach (ServerThread *server, mServers)
            server->runServer();
    }

    static void setupUnixSignalHandlers();
    static void termSignalHandler(int);

private slots:
    void aboutToQuit()
    {
        // Stop servers in reverse order
        for (int i = mServers.count() - 1; i >= 0; --i)
            mServers[i]->stopServer();
    }

    void handleSigTerm();

private:
    QList<ServerThread*> mServers;
    QSocketNotifier *snTerm;

    static int sigtermFd[2];
};


/* What follows is a bit of jumping through hoops in order to perform Qt stuff
 * in response to UNIX signals. Based on documentation at:
 * http://doc.trolltech.com/4.5/unix-signals.html
 */

int ServerMonitor::sigtermFd[2];

ServerMonitor::ServerMonitor(const QStringList &serverExecutables)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");

    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)),
            QCoreApplication::instance(), SLOT(quit()));

    foreach (const QString &executable, serverExecutables)
        mServers.append(new ServerThread(executable, this));
}

void ServerMonitor::setupUnixSignalHandlers()
{
    struct sigaction term;
    term.sa_handler = ServerMonitor::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &term, 0))
        qFatal("Could not set TERM signal handler");
}

void ServerMonitor::termSignalHandler(int)
{
    char tmp = 1;
    ::write(sigtermFd[0], &tmp, sizeof(tmp));
}

void ServerMonitor::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    QCoreApplication::quit();

    snTerm->setEnabled(true);
}


int main(int argc, char *argv[])
{
    ServerMonitor::setupUnixSignalHandlers();

    QCoreApplication app(argc, argv);

    QStringList arguments = app.arguments();
    QStringList serverExecutables;

    if (!arguments.count() > 1) {
        arguments.removeFirst();
        serverExecutables = arguments;
    } else {
        serverExecutables
                << "src/manaserv-account"
                << "src/manaserv-game";
    }

    app.arguments();

    ServerMonitor monitor(serverExecutables);
    monitor.start();

    QObject::connect(&app, SIGNAL(aboutToQuit()),
                     &monitor, SLOT(aboutToQuit()));

    return app.exec();
}

#include "main.moc"
