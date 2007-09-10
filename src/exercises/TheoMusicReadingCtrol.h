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

#ifndef __THOEMUSICREADINGCTROL_H__        //to avoid nested includes
#define __THOEMUSICREADINGCTROL_H__

#ifdef __GNUG__
#pragma interface "TheoMusicReadingCtrol.cpp"
#endif

// for (compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Constrains.h"
#include "MusicReadingConstrains.h"

// lmTheoMusicReadingCtrol is an lmOneScoreCtrol but the controls are created
// by this class by overriding virtual method CreateControls()

class lmTheoMusicReadingCtrol : public lmOneScoreCtrol
{
   DECLARE_DYNAMIC_CLASS(lmTheoMusicReadingCtrol)

public:

    // constructor and destructor
    lmTheoMusicReadingCtrol(wxWindow* parent, wxWindowID id,
               lmMusicReadingConstrains* pOptions,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize, int style = 0);

    ~lmTheoMusicReadingCtrol();

    //implementation of virtual methods
    void InitializeStrings() {}
    void CreateAnswerButtons(int nHeight, int nSpacing, wxFont& font) {}
    void PrepareAuxScore(int nButton) {}
    wxString SetNewProblem();    
    wxDialog* GetSettingsDlg();
    void ReconfigureButtons() {}

    //overrides of virtual methods
    void CreateControls();



private:

        // member variables

    lmScoreConstrains*          m_pScoreConstrains;
    lmMusicReadingConstrains*   m_pConstrains;

    //layout
    wxBoxSizer*     m_pButtonsSizer;


};



#endif  // __THOEMUSICREADINGCTROL_H__

