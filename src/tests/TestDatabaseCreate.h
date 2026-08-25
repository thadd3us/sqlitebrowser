#ifndef TESTDATABASECREATE_H
#define TESTDATABASECREATE_H

#include <QObject>

/**
 * Covers "File > New Database", i.e. DBBrowserDB::create() followed by the
 * createTable() call MainWindow makes right afterwards. This mostly exists to
 * catch differences between SQLite and drop-in replacements such as doltlite.
 */
class TestDatabaseCreate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void createReturnsAnOpenDatabase();
    void createFailsLoudlyForUnwritableLocation();
};

#endif
