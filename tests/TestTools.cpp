/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestTools.h"

#include "core/Clock.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestTools)

namespace
{
    QString createDecimal(QString wholes, QString fractions, QString unit)
    {
        return wholes + QLocale().decimalPoint() + fractions + " " + unit;
    }
} // namespace

void TestTools::testHumanReadableFileSize()
{
    constexpr auto kibibyte = 1024u;
    using namespace Tools;

    QCOMPARE(createDecimal("1", "00", "B"), humanReadableFileSize(1));
    QCOMPARE(createDecimal("1", "00", "KiB"), humanReadableFileSize(kibibyte));
    QCOMPARE(createDecimal("1", "00", "MiB"), humanReadableFileSize(kibibyte * kibibyte));
    QCOMPARE(createDecimal("1", "00", "GiB"), humanReadableFileSize(kibibyte * kibibyte * kibibyte));

    QCOMPARE(QString("100 B"), humanReadableFileSize(100, 0));
    QCOMPARE(createDecimal("1", "10", "KiB"), humanReadableFileSize(kibibyte + 100));
    QCOMPARE(createDecimal("1", "001", "KiB"), humanReadableFileSize(kibibyte + 1, 3));
    QCOMPARE(createDecimal("15", "00", "KiB"), humanReadableFileSize(kibibyte * 15));
}

void TestTools::testIsHex()
{
    QVERIFY(Tools::isHex("0123456789abcdefABCDEF"));
    QVERIFY(!Tools::isHex(QByteArray("0xnothex")));
}

void TestTools::testIsBase64()
{
    QVERIFY(Tools::isBase64(QByteArray("1234")));
    QVERIFY(Tools::isBase64(QByteArray("123=")));
    QVERIFY(Tools::isBase64(QByteArray("12==")));
    QVERIFY(Tools::isBase64(QByteArray("abcd9876MN==")));
    QVERIFY(Tools::isBase64(QByteArray("abcd9876DEFGhijkMNO=")));
    QVERIFY(Tools::isBase64(QByteArray("abcd987/DEFGh+jk/NO=")));
    QVERIFY(!Tools::isBase64(QByteArray("abcd123==")));
    QVERIFY(!Tools::isBase64(QByteArray("abc_")));
    QVERIFY(!Tools::isBase64(QByteArray("123")));
}

void TestTools::testEnvSubstitute()
{
    QProcessEnvironment environment;

#if defined(Q_OS_WIN)
    environment.insert("HOMEDRIVE", "C:");
    environment.insert("HOMEPATH", "\\Users\\User");
    environment.insert("USERPROFILE", "C:\\Users\\User");

    QCOMPARE(Tools::envSubstitute("%HOMEDRIVE%%HOMEPATH%\\.ssh\\id_rsa", environment),
             QString("C:\\Users\\User\\.ssh\\id_rsa"));
    QCOMPARE(Tools::envSubstitute("start%EMPTY%%EMPTY%%%HOMEDRIVE%%end", environment), QString("start%C:%end"));
    QCOMPARE(Tools::envSubstitute("%USERPROFILE%\\.ssh\\id_rsa", environment),
             QString("C:\\Users\\User\\.ssh\\id_rsa"));
    QCOMPARE(Tools::envSubstitute("~\\.ssh\\id_rsa", environment), QString("C:\\Users\\User\\.ssh\\id_rsa"));
#else
    environment.insert("HOME", QString("/home/user"));
    environment.insert("USER", QString("user"));

    QCOMPARE(Tools::envSubstitute("~/.ssh/id_rsa", environment), QString("/home/user/.ssh/id_rsa"));
    QCOMPARE(Tools::envSubstitute("$HOME/.ssh/id_rsa", environment), QString("/home/user/.ssh/id_rsa"));
    QCOMPARE(Tools::envSubstitute("start/$EMPTY$$EMPTY$HOME/end", environment), QString("start/$/home/user/end"));
#endif
}

void TestTools::testValidUuid()
{
    auto validUuid = Tools::uuidToHex(QUuid::createUuid());
    auto nonRfc4122Uuid = "1234567890abcdef1234567890abcdef";
    auto emptyUuid = QString();
    auto shortUuid = validUuid.left(10);
    auto longUuid = validUuid + "baddata";
    auto nonHexUuid = Tools::uuidToHex(QUuid::createUuid()).replace(0, 1, 'p');

    QVERIFY(Tools::isValidUuid(validUuid));
    /* Before https://github.com/keepassxreboot/keepassxc/pull/1770/, entry
     * UUIDs are simply random 16-byte strings. Such older entries should be
     * accepted as well. */
    QVERIFY(Tools::isValidUuid(nonRfc4122Uuid));
    QVERIFY(!Tools::isValidUuid(emptyUuid));
    QVERIFY(!Tools::isValidUuid(shortUuid));
    QVERIFY(!Tools::isValidUuid(longUuid));
    QVERIFY(!Tools::isValidUuid(nonHexUuid));
}

void TestTools::testBackupFilePatternSubstitution_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("dbFilePath");
    QTest::addColumn<QString>("expectedSubstitution");

    static const auto DEFAULT_DB_FILE_NAME = QStringLiteral("KeePassXC");
    static const auto DEFAULT_DB_FILE_PATH = QStringLiteral("/tmp/") + DEFAULT_DB_FILE_NAME + QStringLiteral(".kdbx");
    static const auto NOW = Clock::currentDateTime();
    auto DEFAULT_FORMATTED_TIME = NOW.toString("dd_MM_yyyy_hh-mm-ss");

    QTest::newRow("Null pattern") << QString() << DEFAULT_DB_FILE_PATH << QString();
    QTest::newRow("Empty pattern") << QString("") << DEFAULT_DB_FILE_PATH << QString("");
    QTest::newRow("Null database path") << "valid_pattern" << QString() << QString();
    QTest::newRow("Empty database path") << "valid_pattern" << QString("") << QString();
    QTest::newRow("Unclosed/invalid pattern") << "{DB_FILENAME" << DEFAULT_DB_FILE_PATH << "{DB_FILENAME";
    QTest::newRow("Unknown pattern") << "{NO_MATCH}" << DEFAULT_DB_FILE_PATH << "{NO_MATCH}";
    QTest::newRow("Do not replace escaped patterns (filename)")
        << "\\{DB_FILENAME\\}" << DEFAULT_DB_FILE_PATH << "{DB_FILENAME}";
    QTest::newRow("Do not replace escaped patterns (time)")
        << "\\{TIME:dd.MM.yyyy\\}" << DEFAULT_DB_FILE_PATH << "{TIME:dd.MM.yyyy}";
    QTest::newRow("Multiple patterns should be replaced")
        << "{DB_FILENAME} {TIME} {DB_FILENAME}" << DEFAULT_DB_FILE_PATH
        << DEFAULT_DB_FILE_NAME + QStringLiteral(" ") + DEFAULT_FORMATTED_TIME + QStringLiteral(" ")
               + DEFAULT_DB_FILE_NAME;
    QTest::newRow("Default time pattern") << "{TIME}" << DEFAULT_DB_FILE_PATH << DEFAULT_FORMATTED_TIME;
    QTest::newRow("Default time pattern (empty formatter)")
        << "{TIME:}" << DEFAULT_DB_FILE_PATH << DEFAULT_FORMATTED_TIME;
    QTest::newRow("Custom time pattern") << "{TIME:dd-ss}" << DEFAULT_DB_FILE_PATH << NOW.toString("dd-ss");
    QTest::newRow("Invalid custom time pattern") << "{TIME:dd/-ss}" << DEFAULT_DB_FILE_PATH << NOW.toString("dd/-ss");
    QTest::newRow("Recursive substitution") << "{TIME:'{TIME}'}" << DEFAULT_DB_FILE_PATH << DEFAULT_FORMATTED_TIME;
    QTest::newRow("{DB_FILENAME} substitution")
        << "some {DB_FILENAME} thing" << DEFAULT_DB_FILE_PATH
        << QStringLiteral("some ") + DEFAULT_DB_FILE_NAME + QStringLiteral(" thing");
    QTest::newRow("{DB_FILENAME} substitution with multiple extensions") << "some {DB_FILENAME} thing"
                                                                         << "/tmp/KeePassXC.kdbx.ext"
                                                                         << "some KeePassXC.kdbx thing";
    // Not relevant right now, added test anyway
    QTest::newRow("There should be no substitution loops") << "{DB_FILENAME}"
                                                           << "{TIME:'{DB_FILENAME}'}.ext"
                                                           << "{DB_FILENAME}";
}

void TestTools::testBackupFilePatternSubstitution()
{
    QFETCH(QString, pattern);
    QFETCH(QString, dbFilePath);
    QFETCH(QString, expectedSubstitution);

    QCOMPARE(Tools::substituteBackupFilePath(pattern, dbFilePath), expectedSubstitution);
}
