#include "TestDatabaseCreate.h"

#include "Settings.h"
#include "sql/ObjectIdentifier.h"
#include "sql/sqlitetypes.h"
#include "sqlitedb.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

// QCoreApplication is enough here and keeps the test runnable on headless CI machines. None of the code paths
// below pop up a dialog, they all fail through DBBrowserDB::lastError().
QTEST_GUILESS_MAIN(TestDatabaseCreate)

void TestDatabaseCreate::initTestCase()
{
    // Don't touch the settings of the user running the test suite
    Settings::setUserSettingsFile(QDir(QDir::tempPath()).filePath("sqlb-test-database-create.ini"));
}

void TestDatabaseCreate::createReturnsAnOpenDatabase()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("new.db");

    DBBrowserDB db;
    QVERIFY2(db.create(path), qPrintable(db.lastError()));
    QVERIFY(db.isOpen());
    QVERIFY(!db.readOnly());
    QVERIFY(QFile::exists(path));

    // This is what MainWindow::fileNew() does immediately after create()
    sqlb::FieldVector fields;
    fields.emplace_back("zzz", "INTEGER");
    QVERIFY2(db.createTable(sqlb::ObjectIdentifier("main", "hello"), fields), qPrintable(db.lastError()));

    QVERIFY2(db.executeSQL("INSERT INTO hello(zzz) VALUES(42);"), qPrintable(db.lastError()));
    QVERIFY2(db.releaseAllSavepoints(), qPrintable(db.lastError()));
    QVERIFY(!db.getDirty());
    QVERIFY(db.close());

    // The file we just wrote has to be readable through the regular open path
    DBBrowserDB reopened;
    QVERIFY2(reopened.open(path), qPrintable(reopened.lastError()));
    QCOMPARE(reopened.querySingleValueFromDb("SELECT zzz FROM hello;", false), QByteArray("42"));
    QVERIFY(reopened.close());
}

void TestDatabaseCreate::createFailsLoudlyForUnwritableLocation()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QFile::setPermissions(dir.path(), QFile::ReadOwner | QFile::ExeOwner));

    DBBrowserDB db;
    const bool created = db.create(dir.filePath("new.db"));

    // Restore the permissions before asserting so QTemporaryDir can clean up
    QFile::setPermissions(dir.path(), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    QVERIFY(!created);
    QVERIFY(!db.isOpen());
    QVERIFY(!db.lastError().isEmpty());

    // The message has to be the one the database engine gave us for the failed write. "Invalid file format" is
    // what the encryption probe reports once create() has already given up on the file, and it explains nothing.
    QVERIFY2(!db.lastError().contains("Invalid file format"), qPrintable(db.lastError()));
}
