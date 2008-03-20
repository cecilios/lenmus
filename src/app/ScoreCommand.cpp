//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2008 Cecilio Salmeron
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ScoreCommand.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "../score/Score.h"
#include "../score/UndoRedo.h"
#include "ScoreCommand.h"
#include "ScoreDoc.h"
#include "TheApp.h"


//----------------------------------------------------------------------------------------
// lmScoreCommand abstract class implementation
//----------------------------------------------------------------------------------------

lmScoreCommand::lmScoreCommand(const wxString& sName, lmScoreDocument *pDoc)
    : wxCommand(true, sName)
{
    m_pDoc = pDoc;
	m_fDocModified = false;
}

lmScoreCommand::~lmScoreCommand()
{
}

void lmScoreCommand::CommandDone()
{
    //common code after executing a command: 
    //- save document current modification status flag, to restore it if command undo
    //- set document as 'modified'
    //- update the views with the changes

	m_fDocModified = m_pDoc->IsModified();
	m_pDoc->Modify(true);
    m_pDoc->UpdateAllViews();		//this could generate more changes
}




//----------------------------------------------------------------------------------------
// lmCmdSelectSingle implementation
//----------------------------------------------------------------------------------------

bool lmCmdSelectSingle::Do()
{
    return DoSelectObject();
}

bool lmCmdSelectSingle::Undo()
{
    return DoSelectObject();
}

bool lmCmdSelectSingle::DoSelectObject()
{
    wxASSERT( m_pGMO);

    //Toggle 'selected' and return 'true' to force redraw it
    bool fSelected = m_pGMO->IsSelected();
    m_pGMO->SetSelected(!fSelected);
    m_pDoc->UpdateAllViews();
    return true;
}



//----------------------------------------------------------------------------------------
// lmCmdDeleteObject implementation
//----------------------------------------------------------------------------------------

lmCmdDeleteObject::lmCmdDeleteObject(lmVStaffCursor* pVCursor, const wxString& name,
                                     lmScoreDocument *pDoc)
        : lmScoreCommand(name, pDoc)
{
    m_tCursorState = pVCursor->GetState();
    m_pVStaff = pVCursor->GetVStaff();
    m_pSO = pVCursor->GetStaffObj();
    m_fDeleteSO = false;                //m_pSO is still owned by the score
}

lmCmdDeleteObject::~lmCmdDeleteObject()
{
    if (m_fDeleteSO)
        delete m_pSO;       //delete frozen object
}

bool lmCmdDeleteObject::Do()
{
    //Proceed to delete the object
    m_UndoData.Rewind();
    m_pVStaff->Cmd_DeleteObject(&m_UndoData, m_pSO);

	CommandDone();
    m_fDeleteSO = true;                //m_pSO is no longer owned by the score
    return true;
}

bool lmCmdDeleteObject::Undo()
{
    //undelete the object

    m_UndoData.Rewind();
    m_pVStaff->Cmd_Undo_DeleteObject(&m_UndoData, m_pSO);
    m_fDeleteSO = false;                //m_pSO is again owned by the score

    //set cursor
    m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);

	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;
}



//----------------------------------------------------------------------------------------
// lmCmdMoveScoreObj implementation
//----------------------------------------------------------------------------------------

lmCmdMoveScoreObj::lmCmdMoveScoreObj(const wxString& sName, lmScoreDocument *pDoc,
									   lmScoreObj* pSO, const lmUPoint& uPos)
	: lmScoreCommand(sName, pDoc)
{
	lmUPoint uOrg = pSO->GetOrigin();
	m_tPos.x = uPos.x - uOrg.x;
	m_tPos.y = uPos.y - uOrg.y;
	m_tPos.xType = lmLOCATION_USER_RELATIVE;
	m_tPos.yType = lmLOCATION_USER_RELATIVE;
	m_tPos.xUnits = lmLUNITS;
	m_tPos.yUnits = lmLUNITS;

	m_pSO = pSO;
}

bool lmCmdMoveScoreObj::Do()
{
    wxASSERT_MSG( m_pSO, _T("[lmCmdMoveScoreObj::Do] No ScoreObj to move!"));

    m_tOldPos = m_pSO->SetUserLocation(m_tPos);

	CommandDone();
    return true;

}

bool lmCmdMoveScoreObj::Undo()
{
    wxASSERT_MSG( m_pSO, _T("[lmCmdMoveScoreObj::Undo]: No ScoreObj to move!"));

    m_pSO->SetUserLocation(m_tOldPos);
	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;

}




//----------------------------------------------------------------------------------------
// lmCmdInsertBarline: Insert a clef at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertBarline::lmCmdInsertBarline(lmVStaffCursor* pVCursor, const wxString& sName,
                                       lmScoreDocument *pDoc, lmEBarline nType)
	: lmScoreCommand(sName, pDoc)
{
    m_pVCursor = pVCursor;
    m_nBarlineType = nType;
}

bool lmCmdInsertBarline::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetCursor(m_pVCursor);
    pCursor->GetVStaff()->InsertBarline(m_nBarlineType);

	CommandDone();
    return true;
}

bool lmCmdInsertBarline::Undo()
{
    //TODO
	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;
}




//----------------------------------------------------------------------------------------
// lmCmdInsertClef: Insert a clef at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertClef::lmCmdInsertClef(lmVStaffCursor* pVCursor, const wxString& sName,
                                 lmScoreDocument *pDoc, lmEClefType nClefType)
	: lmScoreCommand(sName, pDoc)
{
    m_pVCursor = pVCursor;
    m_nClefType = nClefType;
}

bool lmCmdInsertClef::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetCursor(m_pVCursor);
    pCursor->GetVStaff()->InsertClef(m_nClefType);

	CommandDone();
    return true;
}

bool lmCmdInsertClef::Undo()
{
    //TODO
	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;
}




//----------------------------------------------------------------------------------------
// lmCmdInsertNote: Insert a note at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertNote::lmCmdInsertNote(lmVStaffCursor* pVCursor, const wxString& sName,
                                 lmScoreDocument *pDoc,
                                 lmEPitchType nPitchType,
								 wxString sStep, wxString sOctave, 
								 lmENoteType nNoteType, float rDuration,
								 lmENoteHeads nNotehead, lmEAccidentals nAcc)
	: lmScoreCommand(sName, pDoc)
{
    m_pVCursor = pVCursor;
	m_nNoteType = nNoteType;
	m_nPitchType = nPitchType;
	m_sStep = sStep;
	m_sOctave = sOctave;
	m_rDuration = rDuration;
	m_nNotehead = nNotehead;
	m_nAcc = nAcc;
}

bool lmCmdInsertNote::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetCursor(m_pVCursor);
    m_pVStaff = pCursor->GetVStaff();
    m_pNewNote = m_pVStaff->Cmd_InsertNote(&m_UndoData, m_nPitchType, m_sStep, m_sOctave,
                                       m_nNoteType, m_rDuration, m_nNotehead, m_nAcc);

	CommandDone();
    return true;
}

bool lmCmdInsertNote::Undo()
{
    m_pVStaff->DeleteObject(m_pNewNote);
    m_pDoc->GetScore()->SetCursor(m_pVCursor);

	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;
}




//----------------------------------------------------------------------------------------
// lmCmdChangeNotePitch: Change pitch of note at current cursor position
//----------------------------------------------------------------------------------------

lmCmdChangeNotePitch::lmCmdChangeNotePitch(const wxString& sName, lmScoreDocument *pDoc,
                                 lmNote* pNote, int nSteps)
	: lmScoreCommand(sName, pDoc)
{
	m_nSteps = nSteps;
	m_pNote = pNote;
}

bool lmCmdChangeNotePitch::Do()
{
	m_pNote->ChangePitch(m_nSteps);

	CommandDone();
    return true;
}

bool lmCmdChangeNotePitch::Undo()
{
	m_pNote->ChangePitch(-m_nSteps);
	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;
}




//----------------------------------------------------------------------------------------
// lmCmdChangeNoteAccidentals: Change accidentals of note at current cursor position
//----------------------------------------------------------------------------------------

lmCmdChangeNoteAccidentals::lmCmdChangeNoteAccidentals(const wxString& sName, lmScoreDocument *pDoc,
                                 lmNote* pNote, int nSteps)
	: lmScoreCommand(sName, pDoc)
{
	m_nSteps = nSteps;
	m_pNote = pNote;
}

bool lmCmdChangeNoteAccidentals::Do()
{
	m_pNote->ChangeAccidentals(m_nSteps);

	CommandDone();
    return true;
}

bool lmCmdChangeNoteAccidentals::Undo()
{
	m_pNote->ChangeAccidentals(-m_nSteps);
	m_pDoc->Modify(m_fDocModified);
    m_pDoc->UpdateAllViews();
    return true;
}

