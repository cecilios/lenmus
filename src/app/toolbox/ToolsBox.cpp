//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2008 Cecilio Salmeron
//
//    This program is free software; you can redistribute it and/or modify it under the
//    terms of the GNU General Public License as published by the Free Software Foundation,
//    either version 3 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this
//    program. If not, see <http://www.gnu.org/licenses/>.
//
//    For any comment, suggestion or feature request, please contact the manager of
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ToolsBox.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/cursor.h"
#include "wx/statline.h"

#include "../MainFrame.h"
#include "ToolsBox.h"
#include "../ArtProvider.h"        // to use ArtProvider for managing icons
#include "../TheApp.h"
#include "../ScoreCanvas.h"
#include "../../widgets/Button.h"


//-----------------------------------------------------------------------------------
//AWARE
//
//    Things to do to add a new tools panel to the Tools Box dialog:
//     1. Create a new panel class derived from lmToolPage
//     2. Look for "//TO_ADD:" tags in ToolsBox.h and follow instructions there
//     3. Look for "//TO_ADD:" tags in this file and follow instructions there
//
//-----------------------------------------------------------------------------------

// Panels
#include "ToolNotes.h"
#include "ToolClef.h"
#include "ToolBarlines.h"
//TO_ADD: add here the new tool panel include file



//layout parameters
const int SPACING = 4;          //spacing (pixels) around each sizer
const int BUTTON_SPACING = 4;	//spacing (pixels) between buttons
const int BUTTON_SIZE = 32;		//tools button size (pixels)
const int NUM_COLUMNS = 4;      //number of buttons per row
// ToolBox width = NUM_COLUMNS * BUTTON_SIZE + 2*(NUM_COLUMNS-1)*BUTTON_SPACING + 2*SPACING
//				 = 4*32 + 2*3*4 + 2*4 = 128+24+8 = 160

const int ID_BUTTON = 2200;


BEGIN_EVENT_TABLE(lmToolBox, wxPanel)
	EVT_CHAR (lmToolBox::OnKeyPress)
    EVT_COMMAND_RANGE (ID_BUTTON, ID_BUTTON+NUM_BUTTONS-1, wxEVT_COMMAND_BUTTON_CLICKED, lmToolBox::OnButtonClicked)
    EVT_SIZE (lmToolBox::OnResize) 
END_EVENT_TABLE()

IMPLEMENT_CLASS(lmToolBox, wxPanel)

// an entry for the tools buttons table
typedef struct lmToolsDataStruct {
    lmEToolPage nToolId;		// button ID
    wxString    sBitmap;		// bitmap name
	wxString	sToolTip;		// tool tip
} lmToolsData;


// Tools table
static const lmToolsData m_aToolsData[] = {
    //tool ID			bitmap name					tool tip
    //-----------		-------------				-------------
    {lmPAGE_CLEFS,		_T("tool_clefs"),			_("Select clef, key and time signature edit tools") },
    {lmPAGE_NOTES,		_T("tool_notes"),			_("Select notes / rests edit tools") },
 //   {lmPAGE_SELECTION,	_T("tool_selection"),		_("Select objects") },
	//{lmPAGE_KEY_SIGN,	_T("tool_key_signatures"),	_("Select key signature edit tools") },
	//{lmPAGE_TIME_SIGN,	_T("tool_time_signatures"),	_("Select time signatures edit tools") },
	{lmPAGE_BARLINES,	_T("tool_barlines"),		_("Select barlines and rehearsal marks edit tools") },
	//TO_ADD: Add here information about the new tool
	//NEXT ONE MUST BE THE LAST ONE
	{lmPAGE_NONE,		_T(""), _T("") },
};

lmToolBox::lmToolBox(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxPoint(0,0), wxSize(170, -1), wxBORDER_NONE)
{
	//Create the dialog
	m_nSelTool = lmPAGE_NONE;

	//set colors
	m_colors.SetBaseColor( wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE) );

	CreateControls();

	//initialize panel's array
    for (int i=0; i < (int)lmPAGE_MAX; i++)
	{
        wxPanel* pPanel = CreatePanel((lmEToolPage)i);
        if (pPanel) pPanel->Show(false);
        m_cPanels.push_back(pPanel);
    }

	SelectToolPage(lmPAGE_NOTES);

}

void lmToolBox::CreateControls()
{
    //Controls creation for ToolsBox Dlg

    //the main sizer, to contain the three areas
    wxBoxSizer* pMainSizer = new wxBoxSizer(wxVERTICAL);

    //the tool page buttons selection area
	wxPanel* pSelectPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    pSelectPanel->SetBackgroundColour(m_colors.Normal());

	wxBoxSizer* pSelectSizer = new wxBoxSizer( wxVERTICAL );
	
    wxGridSizer* pButtonsSizer = new wxGridSizer(NUM_COLUMNS);
    int iMax = sizeof(m_aToolsData)/sizeof(lmToolsData);
    wxSize btSize(BUTTON_SIZE, BUTTON_SIZE);
	for (int iB=0; iB < iMax; iB++)
	{
		if (m_aToolsData[iB].nToolId == lmPAGE_NONE) break;

        m_pButton[iB] = new lmCheckButton(pSelectPanel, ID_BUTTON + iB, wxBitmap(btSize.x, btSize.y));
        m_pButton[iB]->SetBitmapUp(m_aToolsData[iB].sBitmap, _T(""), btSize);
        m_pButton[iB]->SetBitmapDown(m_aToolsData[iB].sBitmap, _T("button_selected_flat"), btSize);
        m_pButton[iB]->SetBitmapOver(m_aToolsData[iB].sBitmap, _T("button_over_flat"), btSize);
        m_pButton[iB]->SetToolTip(m_aToolsData[iB].sToolTip);
		int sides = 0;
		if (iB > 0) sides |= wxLEFT;
		if (iB < iMax-1) sides |= wxRIGHT;
		pButtonsSizer->Add(m_pButton[iB],
						   wxSizerFlags(0).Border(sides, BUTTON_SPACING) );
	}

    pSelectSizer->Add( pButtonsSizer, 1, wxEXPAND|wxALL, SPACING );
	
	pSelectPanel->SetSizer( pSelectSizer );
	pSelectPanel->Layout();
	pSelectSizer->Fit( pSelectPanel );
	pMainSizer->Add( pSelectPanel, 0, 0, SPACING );
	
    //the pages
	m_pPageSizer = new wxBoxSizer( wxVERTICAL );
	
	int nWidth = NUM_COLUMNS * BUTTON_SIZE + 2*(NUM_COLUMNS-1)*BUTTON_SPACING + 2*SPACING;
    m_pEmptyPage = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 800),
							   wxBORDER_SUNKEN|wxTAB_TRAVERSAL );
    m_pEmptyPage->SetBackgroundColour(m_colors.Bright());
	m_pCurPage = m_pEmptyPage;
	m_pPageSizer->Add( m_pCurPage, 1, wxEXPAND, SPACING );
	
	pMainSizer->Add( m_pPageSizer, 1, wxEXPAND, SPACING );
	
	SetSizer( pMainSizer );
    pMainSizer->SetSizeHints(this);
	Layout();
}

lmToolBox::~lmToolBox()
{
}

wxPanel* lmToolBox::CreatePanel(lmEToolPage nPanel)
{
    switch(nPanel) {
		case lmPAGE_SELECTION:
            return (wxPanel*)NULL;
        case lmPAGE_CLEFS:
            return new lmToolPageClefs(this);
		case lmPAGE_KEY_SIGN:
            return (wxPanel*)NULL;
		case lmPAGE_TIME_SIGN:
            return (wxPanel*)NULL;
        case lmPAGE_NOTES:
            return new lmToolPageNotes(this);
        case lmPAGE_BARLINES:
            return new lmToolPageBarlines(this);
        //TO_ADD: Add a new case block for creating the new tool panel
        default:
            wxASSERT(false);
    }
    return (wxPanel*)NULL;

}

void lmToolBox::OnButtonClicked(wxCommandEvent& event)
{
    //identify button pressed
	SelectToolPage((lmEToolPage)(event.GetId() - ID_BUTTON));

	//lmController* pController = g_pTheApp->GetActiveController();
    //if (pController)
	//  pController->SetCursor(*wxCROSS_CURSOR);
    //wxLogMessage(_T("[lmToolBox::OnButtonClicked] Tool %d selected"), m_nSelTool);
}

void lmToolBox::SelectToolPage(lmEToolPage nTool)
{
	if (!(nTool > lmPAGE_NONE && nTool < lmPAGE_MAX)) 
        return;

    SelectButton((int)nTool);
	m_nSelTool = nTool;

    //hide current page and save it
    wxPanel* pOldPage = m_pCurPage;
    pOldPage->Hide();

    //show new one
    m_pCurPage = (m_cPanels[nTool] ? m_cPanels[nTool] : m_pEmptyPage);
    m_pCurPage->Show();
    m_pPageSizer->Replace(pOldPage, m_pCurPage); 
    m_pCurPage->SetFocus();
    GetSizer()->Layout();

    //return focus to active view
    GetMainFrame()->SetFocusOnActiveView();
}

void lmToolBox::SelectButton(int nTool)
{
	// Set selected button as 'pressed' and all others as 'released'
	for(int i=0; i < (int)lmPAGE_MAX; i++)
	{
		if (i != nTool)
			m_pButton[i]->Release();
		else
			m_pButton[i]->Press();
	}
}

void lmToolBox::OnKeyPress(wxKeyEvent& event)
{
	//redirect all key press events to the active child window
	GetMainFrame()->RedirectKeyPressEvent(event);
}

void lmToolBox::OnResize(wxSizeEvent& event)
{
    //wxSize newSize = event.GetSize();
    //wxLogMessage(_T("[lmToolBox::OnResize] New size: %d, %d"), newSize.x, newSize.y);
}

