//--------------------------------------------------------------------------------------
//  LenMus Library
//  Copyright (c) 2010 LenMus project
//
//  This program is free software; you can redistribute it and/or modify it under the 
//  terms of the GNU General Public License as published by the Free Software Foundation,
//  either version 3 of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT ANY 
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
//  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License along
//  with this library; if not, see <http://www.gnu.org/licenses/> or write to the
//  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
//  MA  02111-1307,  USA.
//
//  For any comment, suggestion or feature request, please contact the manager of
//  the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------

#ifdef _LM_DEBUG_

#include <UnitTest++.h>
#include <sstream>

//classes related to these tests
#include "lenmus_document.h"
#include "lenmus_document_cursor.h"

//to delete singletons
#include "lenmus_factory.h"
#include "lenmus_elements.h"


using namespace UnitTest;
using namespace std;
using namespace lenmus;

//derivate class to have access to some protected methods
class TestCursor : public DocCursor
{
public:
    TestCursor(Document* pDoc) : DocCursor(pDoc) {}

    //access to some protected methods
    inline bool now_delegating() { return is_delegating(); }
};


class DocCursorTestFixture
{
public:

    DocCursorTestFixture()     //SetUp fixture
    {
    }

    ~DocCursorTestFixture()    //TearDown fixture
    {
        delete Factory::instance();
    }
};

SUITE(DocCursorTest)
{
    TEST_FIXTURE(DocCursorTestFixture, DocCursorPointsStartOfContent)
    {
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        //cout << (*cursor)->to_string() << endl;
        CHECK( (*cursor)->to_string() == "(score (vers 1.6) (instrument (musicData (n c4 q) (r q))))" );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorNext)
    {
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        ++cursor;
        //cout << (*cursor)->to_string() << endl;
        CHECK( (*cursor)->to_string() == "(text \"this is text\")" );
        ++cursor;
        CHECK( *cursor == NULL );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorEnterTopDelegates)
    {
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        CHECK( !cursor.now_delegating() );
        cursor.enter_element();
        CHECK( cursor.now_delegating() );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorEnterOtherDoesNothing)
    {
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        cursor.enter_element();
        LdpElement* pElm = *cursor;
        cursor.enter_element();
        CHECK( cursor.now_delegating() );
        CHECK( pElm == *cursor );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorPrevAtFirstTop)
    {
        //remains at first top level element if pointing to first top level element.
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        CHECK( (*cursor)->to_string() == "(score (vers 1.6) (instrument (musicData (n c4 q) (r q))))" );
        --cursor;
        CHECK( (*cursor)->to_string() == "(score (vers 1.6) (instrument (musicData (n c4 q) (r q))))" );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorPrevAtIntermediateTop)
    {
        //moves back to previous top level element if pointing to a top level element
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        ++cursor;
        CHECK( (*cursor)->to_string() == "(text \"this is text\")" );
        --cursor;
        CHECK( (*cursor)->to_string() == "(score (vers 1.6) (instrument (musicData (n c4 q) (r q))))" );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorPrevAtEndOfCollection)
    {
        //moves to last top level element if pointing to 'end-of-collection' value
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        ++cursor;
        ++cursor;
        CHECK( *cursor == NULL );
        --cursor;
        CHECK( (*cursor)->to_string() == "(text \"this is text\")" );
    }

    TEST_FIXTURE(DocCursorTestFixture, ScoreCursorStart)
    {
        //start: initially in first instrument, first staff, after prolog.
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        cursor.enter_element();
        CHECK( (*cursor)->to_string() == "(n c4 q)" );
    }

    TEST_FIXTURE(DocCursorTestFixture, DocCursorPrevAtStartOfSubelement)
    {
        //moves back to previous top level element if pointing to first sub-element of a top level element.
        Document doc;
        doc.from_string("(lenmusdoc (vers 0.0) (content (score (vers 1.6) (language en iso-8859-1) (instrument (musicData (n c4 q) (r q)))) (text \"this is text\")))" );
        TestCursor cursor(&doc);
        cursor.enter_element();
        CHECK( (*cursor)->to_string() == "(n c4 q)" );
        --cursor;
        cout << (*cursor)->to_string() << endl;
        CHECK( (*cursor)->to_string() == "(score (vers 1.6) (instrument (musicData (n c4 q) (r q))))" );
    }

}

#endif  // _LM_DEBUG_

