/*************************************************************************
 * $Id: CollectionTest.cxx,v 1.4 2000/03/14 11:12:55 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/14 11:12:55 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  11/27/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include "BufferedArray.hxx"
#include "Slist.hxx"
#include "Dlist.hxx"
#include "HashTable.hxx"

#define VERIFY_LIST_RESULT(X, Y)                \
    if(verify_list_result((X), (Y))) {          \
        return;                                 \
    }

__USING_NAMESPACE(SELFSOFT)

/**************************************************************************/
/* List Tests                                                             */
/**************************************************************************/

int verify_list_result(const AbstractList<char> &list, const char *s) {
    if((int) strlen(s) != list.size()) {
        cout << "Failed for " << s << " (inconsistent size: " << list.size() << endl;
    }

    Ptr<ListIterator<char> > iterator = list.getIterator();
    String str;
    while(iterator->isNextNotNull()) {
        str += *iterator->next();
    }

    if(str != s) {
        cout << "Failed for " << s << ": " << str << endl;
        return 1;
    }

    return 0;
}

void doAbstractListTest(AbstractList<char> &list) {
    try {
        list.prepend('a');
        VERIFY_LIST_RESULT(list, "a");
        list.prepend('b');
        VERIFY_LIST_RESULT(list, "ba");
        list.append('c');
        VERIFY_LIST_RESULT(list, "bac");
        list.insert('d', 2);
        VERIFY_LIST_RESULT(list, "badc");
        list.prepend("abc", 3);
        VERIFY_LIST_RESULT(list, "abcbadc");
        list.append("ba", 2);
        VERIFY_LIST_RESULT(list, "abcbadcba");
        list.insert("cc", 2, 2);
        VERIFY_LIST_RESULT(list, "abcccbadcba");

        list.removeFirst();
        VERIFY_LIST_RESULT(list, "bcccbadcba");
        list.removeLast();
        VERIFY_LIST_RESULT(list, "bcccbadcb");
        list.remove(4);
        VERIFY_LIST_RESULT(list, "bcccadcb");
        list.remove(1, 3);
        VERIFY_LIST_RESULT(list, "badcb");

        list.append(list);
        VERIFY_LIST_RESULT(list, "badcbbadcb");
        list.prepend(list);
        VERIFY_LIST_RESULT(list, "badcbbadcbbadcbbadcb");
        list.insert(list, 5);
        VERIFY_LIST_RESULT(list, "badcbbadcbbadcbbadcbbadcbbadcbbadcbbadcb");

        char c;
        if((c = list.get(7)) != 'd') {
            cout << "failed for get(7), result = " << c << endl;
            return;
        }

        if((c = list.get(18)) != 'c') {
            cout << "failed for get(18), result = " << c << endl;
            return;
        }

        list.set('z', 10);
        if((c = list.get(10)) != 'z') {
            cout << "failed for set('z', 10)then get(10), result = " << c << endl;
            return;
        }

        int idx;
        if((idx = list.firstIndexOf('z')) != 10) {
            cout << "failed for firstIndexOf('z'), result = " << idx << endl;
            return;
        }

        if((idx = list.lastIndexOf('a')) != 36) {
            cout << "failed for lastIndexOf('a'), result = " << idx << endl;
            return;
        }

        list.clear();
        VERIFY_LIST_RESULT(list, "");

    } catch(Exception e) {
        cout << "failed. Exception: " << e.getMessage() << endl;
    }

    cout << "passed" << endl;
}

void doArrayTest() {
    cout << "BufferedArray test ... " << flush;
    BufferedArray<char> array;
    doAbstractListTest(array);
}

void doSlistTest() {
    cout << "Slist test ... " << flush;
    Slist<char> slist;
    doAbstractListTest(slist);
}

void doDlistTest() {
    cout << "Dlist test ... " << flush;
    Dlist<char> dlist;
    doAbstractListTest(dlist);
}

void doHashTableTest() {
    cout << "HashTable test ... " << flush;

	int i;
    HashTable<int32, int32> htbl(hash_int32);

    htbl.put(1, 11);
    htbl.put(2, 22);
    htbl.put(3, 33);
    htbl.put(4, 44);
    htbl.put(5, 55);

    //htbl.setBucketSize(1);

    cout << htbl.size() << endl;
    cout << htbl.getBucketSize() << endl;

    for(i = 1; i <= 10; i++) {
        const int32 *v;
        if((v = htbl.get(i)) != NULL) {
            cout << "Found " << i << " = " << *v << endl;
        } else {
            cout << "Not found " << i << endl;
        }
    }

    htbl.remove(2);
    htbl.remove(4);

    Ptr<HashTableKeyIterator<int, int> > itK =
        (HashTableKeyIterator<int, int> *)htbl.getKeyIterator();
    while(itK->isNextNotNull()) {
        cout << *itK->next() << endl;
    }

    Ptr<HashTableValueIterator<int, int> > itV =
        (HashTableValueIterator<int, int> *) htbl.getValueIterator();
    while(itV->isNextNotNull()) {
        cout << *itV->next() << endl;
    }


    for(i = 1; i <= 5; i++) {
        const int32 *v;
        if((v = htbl.get(i)) != NULL) {
            cout << "Found " << i << " = " << *v << endl;
        } else {
            cout << "Not found " << i << endl;
        }
    }

    htbl.put(1, 111);
    htbl.put(2, 222);
    htbl.put(3, 333);
    htbl.put(4, 444);
    htbl.put(5, 555);
    for(i = 1; i <= 5; i++) {
        const int32 *v;
        if((v = htbl.get(i)) != NULL) {
            cout << "Found " << i << " = " << *v << endl;
        } else {
            cout << "Not found " << i << endl;
        }
    }

    itK =
        (HashTableKeyIterator<int, int> *) htbl.getKeyIterator();
    while(itK->isNextNotNull()) {
        cout << *itK->next() << endl;
    }

    itV =
        (HashTableValueIterator<int, int> *) htbl.getValueIterator();
    while(itV->isNextNotNull()) {
        cout << *itV->next() << endl;
    }

    cout << "passed" << endl;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
