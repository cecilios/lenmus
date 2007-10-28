//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2007 Cecilio Salmeron
//
//    This program is free software; you can redistribute it and/or modify it under the
//    terms of the GNU General Public License as published by the Free Software Foundation;
//    either version 2 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this
//    program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street,
//    Fifth Floor, Boston, MA  02110-1301, USA.
//
//    For any comment, suggestion or feature request, please contact the manager of
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------

#ifndef __CLEF_H__        //to avoid nested includes
#define __CLEF_H__

#ifdef __GNUG__
#pragma interface "Clef.cpp"
#endif

#include "wx/dc.h"

#include "Glyph.h"

//------------------------------------------------------------------------------------------------
// lmClef object
//------------------------------------------------------------------------------------------------

class lmClef: public lmStaffObj
{
public:
    //constructor and destructor
    lmClef(EClefType nClefType, lmVStaff* pStaff, int nNumStaff=1, bool fVisible=true);
    ~lmClef() {}

    //other methods
    EClefType GetClefType() {return m_nClefType;}

    //implementation of virtual methods defined in abstract base class lmStaffObj
    void DrawObject(bool fMeasuring, lmPaper* pPaper, wxColour colorC, bool fHighlight);
    void LayoutObject(lmBox* pBox, lmPaper* pPaper, wxColour colorC, bool fHighlight);
    wxBitmap* GetBitmap(double rScale);
    void MoveDragImage(lmPaper* pPaper, wxDragImage* pDragImage, lmDPoint& ptOffset,
                         const lmUPoint& ptLog, const lmUPoint& uDragStartPos, const lmDPoint& ptPixels);
    lmUPoint EndDrag(const lmUPoint& uPos);


    //    debugging
    wxString Dump();
    wxString SourceLDP();
    wxString SourceXML();

    //rendering related methods
    lmLUnits DrawAt(bool fMeasuring, lmPaper* pPaper, lmUPoint uPos, wxColour colorC = *wxBLACK);

    //methods for hiding the clef in prologs
    void Hide(bool fHide) { m_fHidden = fHide; }



private:
    // get fixed measures and values that depend on key type
    lmTenths GetGlyphOffset();
    lmEGlyphIndex GetGlyphIndex();
    lmLUnits DrawClef(bool fMeasuring, lmPaper* pPaper, wxColour colorC = *wxBLACK);

        //variables
    EClefType       m_nClefType;        //type of clef
    bool            m_fHidden;          //to hide it in system prolog

};

//
// global functions related to clefs
//
wxString GetClefLDPNameFromType(EClefType nType);

#endif    // __CLEF_H__

