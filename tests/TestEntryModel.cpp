/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "TestEntryModel.h"

#include <QSignalSpy>
#include <QTest>

#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "gui/DatabaseIcons.h"
#include "gui/IconModels.h"
#include "gui/SortFilterHideProxyModel.h"
#include "gui/entry/AutoTypeAssociationsModel.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryModel.h"
#include "modeltest.h"

QTEST_GUILESS_MAIN(TestEntryModel)

void TestEntryModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
    QVERIFY(Crypto::init());
}

void TestEntryModel::test()
{
    Group* group1 = new Group();
    Group* group2 = new Group();

    Entry* entry1 = new Entry();
    entry1->setGroup(group1);
    entry1->setTitle("testTitle1");

    Entry* entry2 = new Entry();
    entry2->setGroup(group1);
    entry2->setTitle("testTitle2");

    EntryModel* model = new EntryModel(this);

    QSignalSpy spyAboutToBeMoved(model, SIGNAL(rowsAboutToBeMoved(QModelIndex, int, int, QModelIndex, int)));
    QSignalSpy spyMoved(model, SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)));

    ModelTest* modelTest = new ModelTest(model, this);

    model->setGroup(group1);

    QCOMPARE(model->rowCount(), 2);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    entry1->setTitle("changed");
    QCOMPARE(spyDataChanged.count(), 1);

    QModelIndex index1 = model->index(0, 1);
    QModelIndex index2 = model->index(1, 1);

    QCOMPARE(model->data(index1).toString(), entry1->title());
    QCOMPARE(model->data(index2).toString(), entry2->title());

    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(QModelIndex, int, int)));

    Entry* entry3 = new Entry();
    entry3->setGroup(group1);

    QCOMPARE(spyAboutToBeMoved.count(), 0);
    QCOMPARE(spyMoved.count(), 0);

    entry1->moveDown();
    QCOMPARE(spyAboutToBeMoved.count(), 1);
    QCOMPARE(spyMoved.count(), 1);

    entry1->moveDown();
    QCOMPARE(spyAboutToBeMoved.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    entry1->moveDown();
    QCOMPARE(spyAboutToBeMoved.count(), 2);
    QCOMPARE(spyMoved.count(), 2);

    entry3->moveUp();
    QCOMPARE(spyAboutToBeMoved.count(), 3);
    QCOMPARE(spyMoved.count(), 3);

    entry3->moveUp();
    QCOMPARE(spyAboutToBeMoved.count(), 3);
    QCOMPARE(spyMoved.count(), 3);

    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 0);
    QCOMPARE(spyRemoved.count(), 0);

    entry2->setGroup(group2);

    QCOMPARE(spyAboutToAdd.count(), 1);
    QCOMPARE(spyAdded.count(), 1);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    QSignalSpy spyReset(model, SIGNAL(modelReset()));
    model->setGroup(group2);
    QCOMPARE(spyReset.count(), 1);

    delete group1;
    delete group2;

    delete modelTest;
    delete model;
}

void TestEntryModel::testAttachmentsModel()
{
    EntryAttachments* entryAttachments = new EntryAttachments(this);

    EntryAttachmentsModel* model = new EntryAttachmentsModel(this);
    ModelTest* modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);
    model->setEntryAttachments(entryAttachments);
    QCOMPARE(model->rowCount(), 0);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(QModelIndex, int, int)));

    entryAttachments->set("first", QByteArray("123"));

    entryAttachments->set("2nd", QByteArray("456"));
    entryAttachments->set("2nd", QByteArray("7890"));

    const int firstRow = 0;
    QCOMPARE(model->data(model->index(firstRow, EntryAttachmentsModel::NameColumn)).toString(), QString("2nd"));
    QCOMPARE(model->data(model->index(firstRow, EntryAttachmentsModel::SizeColumn), Qt::EditRole).toInt(), 4);

    entryAttachments->remove("first");

    QCOMPARE(spyDataChanged.count(), 1);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    QSignalSpy spyReset(model, SIGNAL(modelReset()));
    entryAttachments->clear();
    model->setEntryAttachments(0);
    QCOMPARE(spyReset.count(), 2);
    QCOMPARE(model->rowCount(), 0);

    delete modelTest;
    delete model;
    delete entryAttachments;
}

void TestEntryModel::testAttributesModel()
{
    EntryAttributes* entryAttributes = new EntryAttributes(this);

    EntryAttributesModel* model = new EntryAttributesModel(this);
    ModelTest* modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);
    model->setEntryAttributes(entryAttributes);
    QCOMPARE(model->rowCount(), 0);

    QSignalSpy spyDataChanged(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));
    QSignalSpy spyAboutToAdd(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
    QSignalSpy spyAdded(model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy spyAboutToRemove(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
    QSignalSpy spyRemoved(model, SIGNAL(rowsRemoved(QModelIndex, int, int)));

    entryAttributes->set("first", "123");

    entryAttributes->set("2nd", "456");
    entryAttributes->set("2nd", "789");

    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("2nd"));

    entryAttributes->remove("first");

    // make sure these don't generate messages
    entryAttributes->set("Title", "test");
    entryAttributes->set("Notes", "test");

    QCOMPARE(spyDataChanged.count(), 1);
    QCOMPARE(spyAboutToAdd.count(), 2);
    QCOMPARE(spyAdded.count(), 2);
    QCOMPARE(spyAboutToRemove.count(), 1);
    QCOMPARE(spyRemoved.count(), 1);

    // test attribute protection
    QString value = entryAttributes->value("2nd");
    entryAttributes->set("2nd", value, true);
    QVERIFY(entryAttributes->isProtected("2nd"));
    QCOMPARE(entryAttributes->value("2nd"), value);

    QSignalSpy spyReset(model, SIGNAL(modelReset()));
    entryAttributes->clear();
    model->setEntryAttributes(0);
    QCOMPARE(spyReset.count(), 2);
    QCOMPARE(model->rowCount(), 0);

    delete modelTest;
    delete model;
}

void TestEntryModel::testDefaultIconModel()
{
    DefaultIconModel* model = new DefaultIconModel(this);
    ModelTest* modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), databaseIcons()->count());

    delete modelTest;
    delete model;
}

void TestEntryModel::testCustomIconModel()
{
    CustomIconModel* model = new CustomIconModel(this);
    ModelTest* modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);

    QHash<QUuid, QPixmap> icons;
    QList<QUuid> iconsOrder;

    QUuid iconUuid = QUuid::fromRfc4122(QByteArray(16, '2'));
    icons.insert(iconUuid, QPixmap());
    iconsOrder << iconUuid;

    QUuid iconUuid2 = QUuid::fromRfc4122(QByteArray(16, '1'));
    icons.insert(iconUuid2, QPixmap());
    iconsOrder << iconUuid2;

    model->setIcons(icons, iconsOrder);
    QCOMPARE(model->uuidFromIndex(model->index(0, 0)), iconUuid);
    QCOMPARE(model->uuidFromIndex(model->index(1, 0)), iconUuid2);

    delete modelTest;
    delete model;
}

void TestEntryModel::testAutoTypeAssociationsModel()
{
    AutoTypeAssociationsModel* model = new AutoTypeAssociationsModel(this);
    ModelTest* modelTest = new ModelTest(model, this);

    QCOMPARE(model->rowCount(), 0);

    AutoTypeAssociations* associations = new AutoTypeAssociations(this);
    model->setAutoTypeAssociations(associations);

    QCOMPARE(model->rowCount(), 0);

    AutoTypeAssociations::Association assoc;
    assoc.window = "1";
    assoc.sequence = "2";
    associations->add(assoc);

    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("1"));
    QCOMPARE(model->data(model->index(0, 1)).toString(), QString("2"));

    assoc.window = "3";
    assoc.sequence = "4";
    associations->update(0, assoc);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QString("3"));
    QCOMPARE(model->data(model->index(0, 1)).toString(), QString("4"));

    associations->add(assoc);
    associations->remove(0);
    QCOMPARE(model->rowCount(), 1);

    delete modelTest;
    delete model;
    delete associations;
}

void TestEntryModel::testProxyModel()
{
    EntryModel* modelSource = new EntryModel(this);
    SortFilterHideProxyModel* modelProxy = new SortFilterHideProxyModel(this);
    modelProxy->setSourceModel(modelSource);

    ModelTest* modelTest = new ModelTest(modelProxy, this);

    Database* db = new Database();
    Entry* entry = new Entry();
    entry->setTitle("Test Title");
    entry->setGroup(db->rootGroup());

    modelSource->setGroup(db->rootGroup());

    /**
     * @author Fonic <https://github.com/fonic>
     * Update comparison value of modelProxy->columnCount() to account for
     * additional columns 'Password', 'Notes', 'Expires', 'Created', 'Modified',
     * 'Accessed', 'Paperclip', 'Attachments', and TOTP
     */
    QSignalSpy spyColumnRemove(modelProxy, SIGNAL(columnsAboutToBeRemoved(QModelIndex, int, int)));
    modelProxy->hideColumn(0, true);
    QCOMPARE(modelProxy->columnCount(), 14);
    QVERIFY(!spyColumnRemove.isEmpty());

    int oldSpyColumnRemoveSize = spyColumnRemove.size();
    modelProxy->hideColumn(0, true);
    QCOMPARE(spyColumnRemove.size(), oldSpyColumnRemoveSize);

    modelProxy->hideColumn(100, true);
    QCOMPARE(spyColumnRemove.size(), oldSpyColumnRemoveSize);

    QList<Entry*> entryList;
    entryList << entry;
    modelSource->setEntries(entryList);

    /**
     * @author Fonic <https://github.com/fonic>
     * Update comparison value of modelProxy->columnCount() to account for
     * additional columns 'Password', 'Notes', 'Expires', 'Created', 'Modified',
     * 'Accessed', 'Paperclip', 'Attachments', and TOTP
     */
    QSignalSpy spyColumnInsert(modelProxy, SIGNAL(columnsAboutToBeInserted(QModelIndex, int, int)));
    modelProxy->hideColumn(0, false);
    QCOMPARE(modelProxy->columnCount(), 15);
    QVERIFY(!spyColumnInsert.isEmpty());

    int oldSpyColumnInsertSize = spyColumnInsert.size();
    modelProxy->hideColumn(0, false);
    QCOMPARE(spyColumnInsert.size(), oldSpyColumnInsertSize);

    delete modelTest;
    delete modelProxy;
    delete modelSource;
    delete db;
}

void TestEntryModel::testDatabaseDelete()
{
    EntryModel* model = new EntryModel(this);
    ModelTest* modelTest = new ModelTest(model, this);

    Database* db1 = new Database();
    Group* group1 = new Group();
    group1->setParent(db1->rootGroup());

    Entry* entry1 = new Entry();
    entry1->setGroup(group1);

    Database* db2 = new Database();
    Entry* entry2 = new Entry();
    entry2->setGroup(db2->rootGroup());

    model->setEntries(QList<Entry*>() << entry1 << entry2);

    QCOMPARE(model->rowCount(), 2);

    delete db1;
    QCOMPARE(model->rowCount(), 1);

    delete entry2;
    QCOMPARE(model->rowCount(), 0);

    delete db2;
    delete modelTest;
    delete model;
}
