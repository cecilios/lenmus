//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2009 LenMus project
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
#include "../score/VStaff.h"
#include "../score/EditCmd.h"
#include "../score/properties/DlgProperties.h"
#include "ScoreCommand.h"
#include "ScoreDoc.h"
#include "TheApp.h"
#include "../graphic/GMObject.h"
#include "../graphic/ShapeArch.h"
#include "../graphic/ShapeBeam.h"
#include "../graphic/ShapeText.h"
#include "../app/Preferences.h"
#include "../ldp_parser/LDPParser.h"


//----------------------------------------------------------------------------------------
// lmScoreCommand abstract class implementation
//
// Do() method will return true to indicate that the action has taken place, false
// otherwise. Returning false will indicate to the command processor that the action is
// not undoable and should not be added to the command history.
//----------------------------------------------------------------------------------------

lmScoreCommand::lmScoreCommand(const wxString& sName, lmDocument *pDoc,
                               lmVStaffCursor* pVCursor, bool fHistory, int nOptions,
                               bool fUpdateViews)
    : wxCommand(true, sName)
      , m_pDoc(pDoc)
	  , m_fDocModified(false)
      , m_fHistory(fHistory)
      , m_nOptions(nOptions)
      , m_fUpdateViews(fUpdateViews)
      , m_pSCO((lmScoreObj*)NULL)
{
    if (pVCursor)
        m_tCursorState = pVCursor->GetState();
    else
        m_tCursorState = g_tNoVCursorState;
}

lmScoreCommand::~lmScoreCommand()
{
}

bool lmScoreCommand::Undo()
{
    //Default implementation valid for most commands using the undo log.

    //undelete the actions
    m_UndoLog.UndoAll();
    return CommandUndone();
}

bool lmScoreCommand::CommandDone(bool fScoreModified, int nOptions)
{
    //common code after executing a command:
    //- save document current modification status flag, to restore it if command undo
    //- set document as 'modified'
    //- update the views with the changes
    //
    // Returns false to indicate that the action must not be added to the command history.

	m_fDocModified = m_pDoc->IsModified();
	m_pDoc->Modify(fScoreModified);
    //if (m_fUpdateViews)
        m_pDoc->UpdateAllViews((wxView*)NULL, new lmUpdateHint(m_nOptions | nOptions) );

    return m_fHistory;
}

bool lmScoreCommand::CommandUndone(int nOptions)
{
    //common code after executing an Undo operation:
    //- reset document to previous 'modified' state
    //- update the views with the changes
    //
    //Returns true to indicate that the action has taken place, false otherwise.
    //Returning false will indicate to the command processor that the action is
    //not redoable and no change should be made to the command history.

    //restore cursor
    if (!IsEmptyState(m_tCursorState))
        m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);

	m_pDoc->Modify(m_fDocModified);

    //update views
    //if (m_fUpdateViews)
    {
        //re-built the graphic model
        m_pDoc->UpdateAllViews((wxView*)NULL, new lmUpdateHint(m_nOptions | nOptions));
    }
    //else
    //{
    //    //the model was not updated when issuing the Do command. Instead the object
    //    //was selected and only that object was re-rendered. Do it in that way again.
    //    //if (m_pSCO)
    //    //{
    //    //    //m_pSCO->->Select
    //    //    m_pSCO->GetShapeFromIdx(m_nShapeIdx)->RenderWithHandlers(m_pPaper);
    //    //}
    //}

    return true;
}

//void lmScoreCommand::SetDirectRedrawData(lmScoreObj* pSCO, int nShapeIdx,
//                                         lmPaper* pPaper)
//{
//    m_pSCO = pSCO;
//    m_nShapeIdx = nShapeIdx
//    m_pPaper = pPaper;
//}




//----------------------------------------------------------------------------------------
// lmNewScoreCommand abstract class implementation
//
// Do() method will return true to indicate that the action has taken place, false
// otherwise. Returning false will indicate to the command processor that the action is
// not undoable and should not be added to the command history.
//----------------------------------------------------------------------------------------

lmNewScoreCommand::lmNewScoreCommand(const wxString& sName, lmDocument *pDoc,
                                     lmCursorState& tCursorState, bool fUndoable,
                                     int nOptions, bool fUpdateViews)
    : wxCommand(true, sName)
      , m_pDoc(pDoc)
	  , m_fDocModified(false)
      , m_fUndoable(fUndoable)
      , m_nOptions(nOptions)
      , m_fUpdateViews(fUpdateViews)
      , m_pSCO((lmScoreObj*)NULL)
      , m_tCursorState(tCursorState)
{
}

lmNewScoreCommand::~lmNewScoreCommand()
{
}

void lmNewScoreCommand::LogCommand()
{
    //log command if undoable

    if (!m_fUndoable)
        return;

    //get score and save source code
    m_sOldSource = m_pDoc->GetScore()->SourceLDP(true);     //true: export cursor
}

bool lmNewScoreCommand::Undo()
{
    //Default implementation: Restore previous state from LDP source code
    //Returns true to indicate that the action has taken place, false otherwise.
    //Returning false will indicate to the command processor that the action is
    //not redoable and no change should be made to the command history.

    //recover old score
    lmLDPParser parser;
    lmScore* pScore = parser.ParseScoreFromText(m_sOldSource);
    if (!pScore)
    {
        wxASSERT(false);
        return false;
    }

    ////restore cursor state
    //m_tCursorState.pSO = (lmStaffObj*)NULL; //TODO: pSO is no longer valid
    //pScore->GetCursor()->SetState(&m_tCursorState);
    
    //ask document to replace current score by old one
    m_pDoc->ReplaceScore(pScore);
    return true;        //undo action has taken place
}

bool lmNewScoreCommand::CommandDone(bool fScoreModified, int nOptions)
{
    //common code after executing a command:
    //- save document current modification status flag, to restore it if command undo
    //- set document as 'modified'
    //- update the views with the changes
    //
    // Returns false to indicate that the action must not be added to the command history.

	m_fDocModified = m_pDoc->IsModified();
	m_pDoc->Modify(fScoreModified);
    //if (m_fUpdateViews)
        m_pDoc->UpdateAllViews((wxView*)NULL, new lmUpdateHint(m_nOptions | nOptions) );

    return m_fUndoable;
}

bool lmNewScoreCommand::CommandUndone(int nOptions)
{
    //common code after executing an Undo operation:
    //- reset document to previous 'modified' state
    //- update the views with the changes
    //
    //Returns true to indicate that the action has taken place, false otherwise.
    //Returning false will indicate to the command processor that the action is
    //not redoable and no change should be made to the command history.

    //restore cursor
    if (!IsEmptyState(m_tCursorState))
        m_pDoc->GetScore()->SetCursorState(&m_tCursorState);

	m_pDoc->Modify(m_fDocModified);

    //update views
    //if (m_fUpdateViews)
    {
        //re-built the graphic model
        m_pDoc->UpdateAllViews((wxView*)NULL, new lmUpdateHint(m_nOptions | nOptions));
    }
    //else
    //{
    //    //the model was not updated when issuing the Do command. Instead the object
    //    //was selected and only that object was re-rendered. Do it in that way again.
    //    //if (m_pSCO)
    //    //{
    //    //    //m_pSCO->->Select
    //    //    m_pSCO->GetShapeFromIdx(m_nShapeIdx)->RenderWithHandlers(m_pPaper);
    //    //}
    //}

    return true;
}

//void lmNewScoreCommand::SetDirectRedrawData(lmScoreObj* pSCO, int nShapeIdx,
//                                         lmPaper* pPaper)
//{
//    m_pSCO = pSCO;
//    m_nShapeIdx = nShapeIdx
//    m_pPaper = pPaper;
//}



//----------------------------------------------------------------------------------------
// lmCmdDeleteSelection implementation
//----------------------------------------------------------------------------------------

enum        //type of object to delete
{
    //lm_eObjArch,
    lm_eObjBeam,
    //lm_eObjBrace,
    //lm_eObjBracket,
    //lm_eObjGlyph,
    //lm_eObjInvisible,
    //lm_eObjLine,
    //lm_eObjStaff,
    lm_eObjStaffObj,
    //lm_eObjStem,
    lm_eObjText,
	lm_eObjTextBlock,
    lm_eObjTie,
    lm_eObjTuplet,
};

lmCmdDeleteSelection::lmCmdDeleteSelection(lmVStaffCursor* pVCursor, const wxString& name,
                                           lmDocument *pDoc, lmGMSelection* pSelection)
        : lmScoreCommand(name, pDoc, pVCursor)
{
    //loop to save objects to delete and its parameters
    wxString sCmdName;
    lmGMObject* pGMO = pSelection->GetFirst();
    while (pGMO)
    {
        switch(pGMO->GetType())
        {
            case eGMO_ShapeTie:
                {
                    lmDeletedSO* pSOData = new lmDeletedSO;
                    pSOData->nObjType = lm_eObjTie;
                    pSOData->pObj = (void*)NULL;
                    pSOData->fObjDeleted = false;
                    pSOData->pParm1 = (void*)( ((lmShapeTie*)pGMO)->GetEndNote() );     //end note
                    pSOData->pParm2 = (void*)NULL;

                    m_ScoreObjects.push_back( pSOData );
                    sCmdName = _T("Delete tie");
                }
                break;

            case eGMO_ShapeBarline:
            case eGMO_ShapeClef:
            case eGMO_ShapeNote:
            case eGMO_ShapeRest:
                {
                    lmDeletedSO* pSOData = new lmDeletedSO;
                    pSOData->nObjType = lm_eObjStaffObj;
                    pSOData->pObj = (void*)pGMO->GetScoreOwner();
                    pSOData->fObjDeleted = false;
                    pSOData->pParm1 = (void*)NULL;
                    pSOData->pParm2 = (void*)NULL;

                    m_ScoreObjects.push_back( pSOData );

                    //select command name
                    switch(pGMO->GetType())
                    {
                        case eGMO_ShapeBarline:
                            sCmdName = _T("Delete barline");    break;
                        case eGMO_ShapeClef:
                            sCmdName = _T("Delete clef");       break;
                        case eGMO_ShapeNote:
                            sCmdName = _T("Delete note");       break;
                        case eGMO_ShapeRest:
                            sCmdName = _T("Delete rest");       break;
                        default:
                            wxASSERT(false);
                    }
               }
                break;

            case eGMO_ShapeTuplet:
                {
                    lmDeletedSO* pSOData = new lmDeletedSO;
                    pSOData->nObjType = lm_eObjTuplet;
                    pSOData->pObj = (void*)NULL;
                    pSOData->fObjDeleted = false;
                    pSOData->pParm1 = (void*)( ((lmShapeTuplet*)pGMO)->GetScoreOwner() );   //start note
                    pSOData->pParm2 = (void*)NULL;

                    m_ScoreObjects.push_back( pSOData );
                    sCmdName = _T("Delete tuplet");
                }
                break;

            case eGMO_ShapeBeam:
                {
                    lmDeletedSO* pSOData = new lmDeletedSO;
                    pSOData->nObjType = lm_eObjBeam;
                    pSOData->pObj = (void*)NULL;
                    pSOData->fObjDeleted = false;
                    pSOData->pParm1 = (void*)( ((lmShapeBeam*)pGMO)->GetScoreOwner() );   //a note in the beam
                    pSOData->pParm2 = (void*)NULL;

                    m_ScoreObjects.push_back( pSOData );
                    sCmdName = _T("Delete beam");
                }
                break;

            case eGMO_ShapeText:
                {
                    lmDeletedSO* pSOData = new lmDeletedSO;
                    pSOData->nObjType = lm_eObjText;
                    pSOData->pObj = (void*)NULL;
                    pSOData->fObjDeleted = false;
                    pSOData->pParm1 = (void*)( ((lmShapeText*)pGMO)->GetScoreOwner() );
                    pSOData->pParm2 = (void*)NULL;

                    m_ScoreObjects.push_back( pSOData );
                    sCmdName = _T("Delete text");
                }
                break;

			case eGMO_ShapeTextBlock:
                {
                    lmDeletedSO* pSOData = new lmDeletedSO;
                    pSOData->nObjType = lm_eObjText;
                    pSOData->pObj = (void*)NULL;
                    pSOData->fObjDeleted = false;
                    pSOData->pParm1 = (void*)( ((lmShapeTitle*)pGMO)->GetScoreOwner() );
                    pSOData->pParm2 = (void*)NULL;

                    m_ScoreObjects.push_back( pSOData );
                    sCmdName = _T("Delete text");
                }
                break;

            //case eGMO_ShapeStaff:
            //case eGMO_ShapeArch:
            //case eGMO_ShapeBrace:
            //case eGMO_ShapeBracket:
            //case eGMO_ShapeComposite:
            //case eGMO_ShapeGlyph:
            //case eGMO_ShapeInvisible:
            //case eGMO_ShapeLine:
            //case eGMO_ShapeMultiAttached:
            //case eGMO_ShapeStem:
            //    break;

            default:
                wxMessageBox(
                    wxString::Format(_T("TODO: Code in lmCmdDeleteSelection to delete %s (type %d)"),
                    pGMO->GetName().c_str(), pGMO->GetType() ));
        }
        pGMO = pSelection->GetNext();
    }

    //if only one object, change command name for better command identification
    if (pSelection->NumObjects() == 1)
        this->m_commandName = sCmdName;
}

lmCmdDeleteSelection::~lmCmdDeleteSelection()
{
    //delete frozen objects
    std::list<lmDeletedSO*>::iterator it;
    for (it = m_ScoreObjects.begin(); it != m_ScoreObjects.end(); ++it)
    {
        if ((*it)->fObjDeleted && (*it)->pObj)
        {
            switch((*it)->nObjType)
            {
                case lm_eObjTie:        delete (lmTie*)(*it)->pObj;             break;
                case lm_eObjTuplet:     delete (lmTupletBracket*)(*it)->pObj;   break;
                case lm_eObjStaffObj:   delete (lmStaffObj*)(*it)->pObj;        break;
                case lm_eObjBeam:       delete (lmStaffObj*)(*it)->pObj;        break;
				case lm_eObjText:		delete (lmTextItem*)(*it)->pObj;        break;
				case lm_eObjTextBlock:	delete (lmTextItem*)(*it)->pObj;        break;
                default:
                    wxASSERT(false);
            }
        }
        delete *it;
    }
    m_ScoreObjects.clear();
}

bool lmCmdDeleteSelection::Do()
{
    //loop to delete the objects
    bool fSuccess = true;
    bool fSkipCmd;
    lmEditCmd* pECmd;
    std::list<lmDeletedSO*>::iterator it;
    for (it = m_ScoreObjects.begin(); it != m_ScoreObjects.end(); ++it)
    {
        fSkipCmd = false;
        lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
        switch((*it)->nObjType)
        {
            case lm_eObjTie:
                {
                    lmNote* pEndNote = (lmNote*)(*it)->pParm1;
                    //if any of the owner notes is also in the selection, it could get deleted
                    //before the tie, causing automatically the removal of the tie. So let's
                    //check that the tie still exists
                    if (pEndNote->IsTiedToPrev())
                    {
                        lmVStaff* pVStaff = pEndNote->GetVStaff();      //affected VStaff
                        pECmd = new lmECmdDeleteTie(pVStaff, pUndoItem, pEndNote);
                        //wxLogMessage(_T("[lmCmdDeleteSelection::Do] Deleting tie"));
                    }
                    else
                        fSkipCmd = true;
                }
                break;

            case lm_eObjBeam:
                {
                    lmNote* pNote = (lmNote*)(*it)->pParm1;
                    //if the owner notes are also in the selection, they could get deleted
                    //before the beam, causing automatically the removal of the beam.
                    //So let's check that the beam still exists
                    if (pNote->IsBeamed())
                    {
                        lmVStaff* pVStaff = pNote->GetVStaff();
                        pECmd = new lmECmdDeleteBeam(pVStaff, pUndoItem, pNote);
                        //wxLogMessage(_T("[lmCmdDeleteSelection::Do] Deleting beam"));
                    }
                    else
                        fSkipCmd = true;
                }
                break;

            case lm_eObjStaffObj:
                {
                    lmStaffObj* pSO = (lmStaffObj*)(*it)->pObj;
                    lmVStaff* pVStaff = pSO->GetVStaff();      //affected VStaff
                    if (pSO->IsClef())
                        pECmd = new lmECmdDeleteClef(pVStaff, pUndoItem, (lmClef*)pSO);
                    else if (pSO->IsTimeSignature())
                        pECmd = new lmECmdDeleteTimeSignature(pVStaff, pUndoItem,
                                                              (lmTimeSignature*)pSO);
                    else if (pSO->IsKeySignature())
                        pECmd = new lmECmdDeleteKeySignature(pVStaff, pUndoItem,
                                                             (lmKeySignature*)pSO);
                    else
                        pECmd = new lmECmdDeleteStaffObj(pVStaff, pUndoItem, pSO);
                    //wxLogMessage(_T("[lmCmdDeleteSelection::Do] Deleting staffobj"));
                }
                break;

            case lm_eObjTuplet:
                {
                    lmNote* pStartNote = (lmNote*)(*it)->pParm1;
                    //if the owner notes are also in the selection, they could get deleted
                    //before the tuplet, causing automatically the removal of the tuplet.
                    //So let's check that the tuplet still exists
                    if (pStartNote->IsInTuplet())
                    {
                        lmVStaff* pVStaff = pStartNote->GetVStaff();
                        pECmd = new lmECmdDeleteTuplet(pVStaff, pUndoItem, pStartNote);
                        //wxLogMessage(_T("[lmCmdDeleteSelection::Do] Deleting tuplet"));
                    }
                    else
                        fSkipCmd = true;
                }
                break;

            case lm_eObjText:
			case lm_eObjTextBlock:
                {
                    lmScoreText* pText = (lmScoreText*)(*it)->pParm1;
					lmComponentObj* pAnchor = (lmComponentObj*)pText->GetParentScoreObj();
					pECmd = new lmEDeleteText(pText, pAnchor, pUndoItem);
					//wxLogMessage(_T("[lmCmdDeleteSelection::Do] Deleting tuplet. %s"),
					//	(pECmd->Success() ? _T("Success") : _T("Fail")) );
				}
                break;

			//case lm_eObjArch:
            //case lm_eObjBrace:
            //case lm_eObjBracket:
            //case lm_eObjComposite:
            //case lm_eObjGlyph:
            //case lm_eObjInvisible:
            //case lm_eObjLine:
            //case lm_eObjStaff:
            //case lm_eObjStem:
            //    break;

            default:
                wxASSERT(false);
        }

        if (fSkipCmd)
            delete pUndoItem;
        else
        {
            if (pECmd->Success())
            {
                (*it)->fObjDeleted = true;                //the Obj is no longer owned by the score
                m_UndoLog.LogCommand(pECmd, pUndoItem);
            }
            else
            {
                fSuccess = false;
                delete pUndoItem;
                delete pECmd;
            }
        }
    }

    //
    if (fSuccess)
   	    return CommandDone(lmSCORE_MODIFIED);
    else
    {
        //undo command for the deleted ScoreObjs
        Undo();
        return false;
    }
}

bool lmCmdDeleteSelection::Undo()
{
    //undelete the object
    m_UndoLog.UndoAll();

    //mark all objects as valid
    std::list<lmDeletedSO*>::iterator it;
    for (it = m_ScoreObjects.begin(); it != m_ScoreObjects.end(); ++it)
    {
        (*it)->fObjDeleted = false;      //the Obj is again owned by the score
    }

    return CommandUndone();
}




//----------------------------------------------------------------------------------------
// lmCmdDeleteStaffObj implementation
//----------------------------------------------------------------------------------------

lmCmdDeleteStaffObj::lmCmdDeleteStaffObj(lmVStaffCursor* pVCursor, const wxString& name,
                                     lmDocument *pDoc, lmStaffObj* pSO)
        : lmScoreCommand(name, pDoc, pVCursor)
{
    m_pVStaff = pSO->GetVStaff();
    m_pSO = pSO;
    m_fDeleteSO = false;                //m_pSO is still owned by the score
}

lmCmdDeleteStaffObj::~lmCmdDeleteStaffObj()
{
    if (m_fDeleteSO)
        delete m_pSO;       //delete frozen object
}

bool lmCmdDeleteStaffObj::Do()
{
    //Proceed to delete the object
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd;
    if (m_pSO->IsClef())
        pECmd = new lmECmdDeleteClef(m_pVStaff, pUndoItem, (lmClef*)m_pSO);
    else if (m_pSO->IsTimeSignature())
        pECmd = new lmECmdDeleteTimeSignature(m_pVStaff, pUndoItem,
                                                (lmTimeSignature*)m_pSO);
    else if (m_pSO->IsKeySignature())
        pECmd = new lmECmdDeleteKeySignature(m_pVStaff, pUndoItem,
                                                (lmKeySignature*)m_pSO);
    else
        pECmd = new lmECmdDeleteStaffObj(m_pVStaff, pUndoItem, m_pSO);

    if (pECmd->Success())
    {
        m_fDeleteSO = true;                //m_pSO is no longer owned by the score
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        m_fDeleteSO = false;
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}

bool lmCmdDeleteStaffObj::Undo()
{
    //undelete the object
    m_UndoLog.UndoAll();
    m_fDeleteSO = false;                //m_pSO is again owned by the score

    return CommandUndone();
}




//----------------------------------------------------------------------------------------
// lmCmdDeleteTie implementation
//----------------------------------------------------------------------------------------

lmCmdDeleteTie::lmCmdDeleteTie(const wxString& name, lmDocument *pDoc,
                               lmNote* pEndNote)
        : lmScoreCommand(name, pDoc, (lmVStaffCursor*)NULL)
{
    m_pEndNote = pEndNote;
}

lmCmdDeleteTie::~lmCmdDeleteTie()
{
}

bool lmCmdDeleteTie::Do()
{
    //Proceed to delete the tie
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdDeleteTie(m_pEndNote->GetVStaff(), pUndoItem, m_pEndNote);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}




//----------------------------------------------------------------------------------------
// lmCmdAddTie implementation
//----------------------------------------------------------------------------------------

lmCmdAddTie::lmCmdAddTie(const wxString& name, lmDocument *pDoc,
                         lmNote* pStartNote, lmNote* pEndNote)
        : lmScoreCommand(name, pDoc, (lmVStaffCursor*)NULL)
{
    m_pStartNote = pStartNote;
    m_pEndNote = pEndNote;
}

lmCmdAddTie::~lmCmdAddTie()
{
}

bool lmCmdAddTie::Do()
{
    //Proceed to add a tie
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdAddTie(m_pEndNote->GetVStaff(), pUndoItem,
                                          m_pStartNote, m_pEndNote);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdMoveObject implementation
//----------------------------------------------------------------------------------------

lmCmdMoveObject::lmCmdMoveObject(const wxString& sName, lmDocument *pDoc,
								 lmGMObject* pGMO, const lmUPoint& uPos,
                                 bool fUpdateViews)
	: lmScoreCommand(sName, pDoc, (lmVStaffCursor*)NULL, true, 0, fUpdateViews)
{
	m_tPos.x = uPos.x;
	m_tPos.y = uPos.y;
	m_tPos.xUnits = lmLUNITS;
	m_tPos.yUnits = lmLUNITS;

	m_pSO = pGMO->GetScoreOwner();
    m_nShapeIdx = pGMO->GetOwnerIDX();
    wxASSERT_MSG( m_pSO, _T("[lmCmdMoveObject::Do] No ScoreObj to move!"));
}

bool lmCmdMoveObject::Do()
{
    //Direct command. NO UNDO LOG
    m_tOldPos = m_pSO->SetUserLocation(m_tPos, m_nShapeIdx);
    if (m_fUpdateViews)
	    return CommandDone(lmSCORE_MODIFIED);
    else
	    return CommandDone(lmSCORE_MODIFIED, lmDO_ONLY_REDRAW);
}

bool lmCmdMoveObject::Undo()
{
    //Direct command. NO UNDO LOG
    m_pSO->SetUserLocation(m_tOldPos, m_nShapeIdx);
    if (m_fUpdateViews)
	    return CommandDone(0);
    else
        return CommandUndone(lmDO_ONLY_REDRAW);
}



//----------------------------------------------------------------------------------------
// lmCmdInsertBarline: Insert a barline at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertBarline::lmCmdInsertBarline(lmVStaffCursor* pVCursor, const wxString& sName,
                                       lmDocument *pDoc, lmEBarline nType)
	: lmScoreCommand(sName, pDoc, pVCursor)
{
    m_nBarlineType = nType;
}

bool lmCmdInsertBarline::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);
    lmVStaff* pVStaff = pCursor->GetVStaff();

    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdInsertBarline(pVStaff, pUndoItem, m_nBarlineType);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}




//----------------------------------------------------------------------------------------
// lmCmdInsertClef: Insert a clef at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertClef::lmCmdInsertClef(lmVStaffCursor* pVCursor, const wxString& sName,
                                 lmDocument *pDoc, lmEClefType nClefType,
                                 bool fHistory)
	: lmScoreCommand(sName, pDoc, pVCursor, fHistory)
{
    m_nClefType = nClefType;
}

bool lmCmdInsertClef::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);
    lmVStaff* pVStaff = pCursor->GetVStaff();

    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    int nStaff = pCursor->GetCursorNumStaff();
    lmEditCmd* pECmd = new lmECmdInsertClef(pVStaff, pUndoItem, m_nClefType, nStaff);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}




//----------------------------------------------------------------------------------------
// lmCmdInsertTimeSignature: Insert a time signature at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertTimeSignature::lmCmdInsertTimeSignature(lmVStaffCursor* pVCursor, const wxString& sName,
                             lmDocument *pDoc,  int nBeats, int nBeatType,
                             bool fVisible, bool fHistory)
	: lmScoreCommand(sName, pDoc, pVCursor, fHistory)
{
    m_nBeats = nBeats;
    m_nBeatType = nBeatType;
    m_fVisible = fVisible;
}

bool lmCmdInsertTimeSignature::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);
    lmVStaff* pVStaff = pCursor->GetVStaff();

    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdInsertTimeSignature(pVStaff, pUndoItem, m_nBeats,
                                                       m_nBeatType, m_fVisible);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}




//----------------------------------------------------------------------------------------
// lmCmdInsertKeySignature: Insert a key signature at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertKeySignature::lmCmdInsertKeySignature(lmVStaffCursor* pVCursor, const wxString& sName,
                             lmDocument *pDoc, int nFifths, bool fMajor,
                             bool fVisible, bool fHistory)
	: lmScoreCommand(sName, pDoc, pVCursor, fHistory)
{
    m_nFifths = nFifths;
    m_fMajor = fMajor;
    m_fVisible = fVisible;
}

bool lmCmdInsertKeySignature::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);
    lmVStaff* pVStaff = pCursor->GetVStaff();

    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdInsertKeySignature(pVStaff, pUndoItem, m_nFifths,
                                                      m_fMajor, m_fVisible);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}




//----------------------------------------------------------------------------------------
// lmCmdInsertNote: Insert a note at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertNote::lmCmdInsertNote(lmVStaffCursor* pVCursor, const wxString& sName,
                                 lmDocument *pDoc,
                                 lmEPitchType nPitchType,
								 int nStep, int nOctave,
								 lmENoteType nNoteType, float rDuration, int nDots,
								 lmENoteHeads nNotehead, lmEAccidentals nAcc,
                                 int nVoice, lmNote* pBaseOfChord, bool fTiedPrev)
	: lmScoreCommand(sName, pDoc, pVCursor)
{
	m_nNoteType = nNoteType;
	m_nPitchType = nPitchType;
	m_nStep = nStep;
	m_nOctave = nOctave;
    m_nDots = nDots;
	m_rDuration = rDuration;
	m_nNotehead = nNotehead;
	m_nAcc = nAcc;
	m_nVoice = nVoice;
	m_pBaseOfChord = pBaseOfChord;
    m_fTiedPrev = fTiedPrev;
}

lmCmdInsertNote::~lmCmdInsertNote()
{
}

bool lmCmdInsertNote::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);
    m_pVStaff = pCursor->GetVStaff();

    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);

    lmEditCmd* pECmd = new lmECmdInsertNote(m_pVStaff, pUndoItem, m_nPitchType, m_nStep,
                                             m_nOctave, m_nNoteType, m_rDuration, m_nDots,
                                             m_nNotehead, m_nAcc, m_nVoice, m_pBaseOfChord,
											 m_fTiedPrev);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }

}



//----------------------------------------------------------------------------------------
// lmCmdNewInsertNote: Insert a note at current cursor position
//----------------------------------------------------------------------------------------

lmCmdNewInsertNote::lmCmdNewInsertNote(bool fUndoable, lmCursorState& tCursorState,
                                 const wxString& sName,
                                 lmDocument *pDoc,
                                 lmEPitchType nPitchType,
								 int nStep, int nOctave,
								 lmENoteType nNoteType, float rDuration, int nDots,
								 lmENoteHeads nNotehead, lmEAccidentals nAcc,
                                 int nVoice, lmNote* pBaseOfChord, bool fTiedPrev)
	: lmNewScoreCommand(sName, pDoc, tCursorState, fUndoable)
{
	m_nNoteType = nNoteType;
	m_nPitchType = nPitchType;
	m_nStep = nStep;
	m_nOctave = nOctave;
    m_nDots = nDots;
	m_rDuration = rDuration;
	m_nNotehead = nNotehead;
	m_nAcc = nAcc;
	m_nVoice = nVoice;
	m_pBaseOfChord = pBaseOfChord;
    m_fTiedPrev = fTiedPrev;
}

lmCmdNewInsertNote::~lmCmdNewInsertNote()
{
}

bool lmCmdNewInsertNote::Do()
{
    //log command if undoable
    LogCommand();

    //insert the note
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetCursorState(&m_tCursorState);
    m_pVStaff = pCursor->GetVStaff();
    bool fAutoBar = lmPgmOptions::GetInstance()->GetBoolValue(lm_DO_AUTOBAR);

    lmNote* pNewNote = m_pVStaff->CmdNew_InsertNote(m_nPitchType, m_nStep, m_nOctave, m_nNoteType,
                                         m_rDuration, m_nDots, m_nNotehead, m_nAcc, 
                                         m_nVoice, m_pBaseOfChord, m_fTiedPrev, fAutoBar);

    //int nAlter = 0;
    //int nStaff = 1;
    //lmNote* pNewNote = m_pVStaff->AddNote(m_nPitchType,
    //                m_nStep, m_nOctave, nAlter,
    //                m_nAcc,
    //                m_nNoteType, m_rDuration, m_nDots,
    //                nStaff, m_nVoice);
				//	//bool fVisible = true,
    // //               bool fBeamed = false, lmTBeamInfo BeamInfo[] = NULL,
    // //               m_pBaseOfChord,
    // //               bool fTie = false,
    // //               lmEStemType nStem = lmSTEM_DEFAULT);
    if (pNewNote)
	    return CommandDone(lmSCORE_MODIFIED);
    else
        return false;
}



//----------------------------------------------------------------------------------------
// lmCmdInsertRest: Insert a rest at current cursor position
//----------------------------------------------------------------------------------------

lmCmdInsertRest::lmCmdInsertRest(lmVStaffCursor* pVCursor, const wxString& sName,
                                 lmDocument *pDoc, lmENoteType nNoteType,
                                 float rDuration, int nDots, int nVoice)
	: lmScoreCommand(sName, pDoc, pVCursor)
{
	m_nNoteType = nNoteType;
    m_nDots = nDots;
	m_rDuration = rDuration;
    m_nVoice = nVoice;
}

lmCmdInsertRest::~lmCmdInsertRest()
{
}

bool lmCmdInsertRest::Do()
{
    lmScoreCursor* pCursor = m_pDoc->GetScore()->SetNewCursorState(&m_tCursorState);
    m_pVStaff = pCursor->GetVStaff();

    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);

    lmEditCmd* pECmd = new lmECmdInsertRest(m_pVStaff, pUndoItem, m_nNoteType,
                                            m_rDuration, m_nDots, m_nVoice);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }

}



//----------------------------------------------------------------------------------------
// lmCmdChangeNotePitch: Change pitch of note at current cursor position
//----------------------------------------------------------------------------------------

lmCmdChangeNotePitch::lmCmdChangeNotePitch(const wxString& sName, lmDocument *pDoc,
                                 lmNote* pNote, int nSteps)
	: lmScoreCommand(sName, pDoc, (lmVStaffCursor*)NULL )
{
	m_nSteps = nSteps;
	m_pNote = pNote;
}

bool lmCmdChangeNotePitch::Do()
{
    //Direct command. NO UNDO LOG

	m_pNote->ChangePitch(m_nSteps);

	return CommandDone(lmSCORE_MODIFIED);
}

bool lmCmdChangeNotePitch::Undo()
{
    //Direct command. NO UNDO LOG

	m_pNote->ChangePitch(-m_nSteps);
    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdChangeNoteAccidentals: Change accidentals of notes in current selection
//----------------------------------------------------------------------------------------

lmCmdChangeNoteAccidentals::lmCmdChangeNoteAccidentals(
                                        lmVStaffCursor* pVCursor,
                                        const wxString& name, lmDocument *pDoc,
                                        lmGMSelection* pSelection, int nAcc)
	: lmScoreCommand(name, pDoc, pVCursor)
{
	m_nAcc = nAcc;

    //loop to save notes to modify
    lmGMObject* pGMO = pSelection->GetFirst();
    while (pGMO)
    {
        if (pGMO->GetType() == eGMO_ShapeNote)
        {
            lmCmdNoteData* pData = new lmCmdNoteData;
            pData->pNote = (lmNote*)pGMO->GetScoreOwner();
            pData->nAcc = pData->pNote->GetAPitch().Accidentals();

            m_Notes.push_back( pData );
        }
        pGMO = pSelection->GetNext();
    }
}

lmCmdChangeNoteAccidentals::~lmCmdChangeNoteAccidentals()
{
    //delete selection data
    std::list<lmCmdNoteData*>::iterator it;
    for (it = m_Notes.begin(); it != m_Notes.end(); ++it)
    {
        delete *it;
    }
    m_Notes.clear();
}

bool lmCmdChangeNoteAccidentals::Do()
{
    //AWARE: Not using UndoLog. Direct execution of command

    std::list<lmCmdNoteData*>::iterator it;
    for (it = m_Notes.begin(); it != m_Notes.end(); ++it)
    {
        (*it)->pNote->ChangeAccidentals(m_nAcc);
    }

	return CommandDone(lmSCORE_MODIFIED);
}

bool lmCmdChangeNoteAccidentals::Undo()
{
    //AWARE: Not using UndoLog. Direct execution of command

    std::list<lmCmdNoteData*>::iterator it;
    for (it = m_Notes.begin(); it != m_Notes.end(); ++it)
    {
        (*it)->pNote->ChangeAccidentals( (*it)->nAcc );
    }

    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdChangeNoteRestDots: Change dots of notes in current selection
//----------------------------------------------------------------------------------------

lmCmdChangeNoteRestDots::lmCmdChangeNoteRestDots(lmVStaffCursor* pVCursor,
                                                 const wxString& name, lmDocument *pDoc,
                                                 lmGMSelection* pSelection, int nDots)
	: lmScoreCommand(name, pDoc, pVCursor)
{
	m_nDots = nDots;

    //loop to save note/rests to modify
    lmGMObject* pGMO = pSelection->GetFirst();
    while (pGMO)
    {
        if (pGMO->GetType() == eGMO_ShapeNote || pGMO->GetType() == eGMO_ShapeRest)
        {
            m_NoteRests.push_back( (lmNoteRest*)pGMO->GetScoreOwner() );
        }
        pGMO = pSelection->GetNext();
    }
}

lmCmdChangeNoteRestDots::~lmCmdChangeNoteRestDots()
{
    //delete selection data
    m_NoteRests.clear();
}

bool lmCmdChangeNoteRestDots::Do()
{
    //loop to change dots
    bool fSuccess = true;
    lmEditCmd* pECmd;
    std::list<lmNoteRest*>::iterator it;
    for (it = m_NoteRests.begin(); it != m_NoteRests.end(); ++it)
    {
        lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
        lmVStaff* pVStaff = (*it)->GetVStaff();      //affected VStaff
        pECmd = new lmECmdChangeDots(pVStaff, pUndoItem, *it, m_nDots);

        if (pECmd->Success())
            m_UndoLog.LogCommand(pECmd, pUndoItem);     //save command in the undo log
        else
        {
            fSuccess = false;
            delete pUndoItem;
            delete pECmd;
        }
    }

    // return result
    if (fSuccess)
   	    return CommandDone(lmSCORE_MODIFIED);
    else
    {
        //undo command for the modified note/rests
        Undo();
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdDeleteTuplet implementation
//----------------------------------------------------------------------------------------

lmCmdDeleteTuplet::lmCmdDeleteTuplet(const wxString& name, lmDocument *pDoc,
                                     lmNoteRest* pStartNR)
        : lmScoreCommand(name, pDoc, (lmVStaffCursor*)NULL)
{
    m_pStartNR = pStartNR;
}

lmCmdDeleteTuplet::~lmCmdDeleteTuplet()
{
}

bool lmCmdDeleteTuplet::Do()
{
    //Proceed to delete the tuplet
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmVStaff* pVStaff = m_pStartNR->GetVStaff();
    lmEditCmd* pECmd = new lmECmdDeleteTuplet(pVStaff, pUndoItem, m_pStartNR);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdAddTuplet implementation: Add a tuplet to notes in current selection
//----------------------------------------------------------------------------------------

lmCmdAddTuplet::lmCmdAddTuplet(lmVStaffCursor* pVCursor, const wxString& name,
                               lmDocument *pDoc, lmGMSelection* pSelection,
                               bool fShowNumber, int nNumber, bool fBracket,
                               lmEPlacement nAbove, int nActual, int nNormal)
	: lmScoreCommand(name, pDoc, pVCursor)
{
    m_fShowNumber = fShowNumber;
    m_nNumber = nNumber;
    m_fBracket = fBracket;
    m_nAbove = nAbove;
    m_nActual = nActual;
    m_nNormal = nNormal;

    //loop to save note/rests to form the tuplet
    lmGMObject* pGMO = pSelection->GetFirst();
    while (pGMO)
    {
        if (pGMO->GetType() == eGMO_ShapeNote || pGMO->GetType() == eGMO_ShapeRest)
        {
            m_NotesRests.push_back( (lmNoteRest*)pGMO->GetScoreOwner() );
        }
        pGMO = pSelection->GetNext();
    }
}

lmCmdAddTuplet::~lmCmdAddTuplet()
{
    //delete selection data
    m_NotesRests.clear();
}

bool lmCmdAddTuplet::Do()
{
    //Proceed to create the tuplet
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmVStaff* pVStaff = m_NotesRests.front()->GetVStaff();
    lmEditCmd* pECmd =
        new lmECmdAddTuplet(pVStaff, pUndoItem, m_NotesRests,  m_fShowNumber, m_nNumber,
                            m_fBracket, m_nAbove, m_nActual, m_nNormal);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdBreakBeam implementation
//----------------------------------------------------------------------------------------

lmCmdBreakBeam::lmCmdBreakBeam(lmVStaffCursor* pVCursor, const wxString& name,
                                     lmDocument *pDoc, lmNoteRest* pBeforeNR)
        : lmScoreCommand(name, pDoc, pVCursor)
{
    m_pBeforeNR = pBeforeNR;
}

lmCmdBreakBeam::~lmCmdBreakBeam()
{
}

bool lmCmdBreakBeam::Do()
{
    //Proceed to delete the object
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdBreakBeam(m_pBeforeNR->GetVStaff(), pUndoItem, m_pBeforeNR);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdJoinBeam implementation
//----------------------------------------------------------------------------------------

lmCmdJoinBeam::lmCmdJoinBeam(lmVStaffCursor* pVCursor, const wxString& name,
                                     lmDocument *pDoc, lmGMSelection* pSelection)
        : lmScoreCommand(name, pDoc, pVCursor)
{
    //loop to save the note/rests to beam
    lmGMObject* pGMO = pSelection->GetFirst();
    while (pGMO)
    {
        if (pGMO->GetType() == eGMO_ShapeNote || pGMO->GetType() == eGMO_ShapeRest)
        {
            lmNoteRest* pNR = (lmNoteRest*)pGMO->GetScoreOwner();
            //exclude notes in chord
            if (pNR->IsRest() || (pNR->IsNote() &&
                                  (!((lmNote*)pNR)->IsInChord() ||
                                   ((lmNote*)pNR)->IsBaseOfChord() )) )
                m_NotesRests.push_back(pNR);
        }
        pGMO = pSelection->GetNext();
    }
}

lmCmdJoinBeam::~lmCmdJoinBeam()
{
}

bool lmCmdJoinBeam::Do()
{
    //Proceed to create the beam
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmVStaff* pVStaff = m_NotesRests.front()->GetVStaff();
    lmEditCmd* pECmd = new lmECmdJoinBeam(pVStaff, pUndoItem, m_NotesRests);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdChangeText: Change ScoreText properties
//----------------------------------------------------------------------------------------

lmCmdChangeText::lmCmdChangeText(lmVStaffCursor* pVCursor, const wxString& name,
                                 lmDocument *pDoc, lmScoreText* pST,
                                 wxString& sText, lmEHAlign nHAlign,
                                 lmLocation tPos, lmTextStyle* pStyle,
                                 int nHintOptions)
	: lmScoreCommand(name, pDoc, pVCursor, true, nHintOptions)
{
    m_pST = pST;
    m_sText = sText;
    m_nHAlign = nHAlign;
    m_tPos = tPos;
    m_pStyle = pStyle;
}

lmCmdChangeText::~lmCmdChangeText()
{
}

bool lmCmdChangeText::Do()
{
    lmUndoItem* pUndoItem = new lmUndoItem(&m_UndoLog);
    lmEditCmd* pECmd = new lmECmdChangeText(m_pST, pUndoItem, m_sText, m_nHAlign,
                                            m_tPos, m_pStyle);

    if (pECmd->Success())
    {
        m_UndoLog.LogCommand(pECmd, pUndoItem);
	    return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        delete pUndoItem;
        delete pECmd;
        return false;
    }
}



//----------------------------------------------------------------------------------------
// lmCmdChangePageMargin implementation
//----------------------------------------------------------------------------------------

lmCmdChangePageMargin::lmCmdChangePageMargin(const wxString& name, lmDocument *pDoc,
                                             lmGMObject* pGMO, int nIdx, int nPage,
											 lmLUnits uPos)
	: lmScoreCommand(name, pDoc, (lmVStaffCursor*)NULL )
{
	m_nIdx = nIdx;
	m_uNewPos = uPos;
	m_nPage = nPage;

    //save current position
    m_pScore = pDoc->GetScore();
    switch(m_nIdx)
    {
        case lmMARGIN_TOP:
            m_uOldPos = m_pScore->GetPageTopMargin(nPage);
            break;

        case lmMARGIN_BOTTOM:
            m_uOldPos = m_pScore->GetMaximumY(nPage);
            break;

        case lmMARGIN_LEFT:
            m_uOldPos = m_pScore->GetLeftMarginXPos(nPage);
            break;

        case lmMARGIN_RIGHT:
            m_uOldPos = m_pScore->GetRightMarginXPos(nPage);
            break;

        default:
            wxASSERT(false);
    }

}

bool lmCmdChangePageMargin::Do()
{
    //Direct command. NO UNDO LOG

    ChangeMargin(m_uNewPos);
	return CommandDone(lmSCORE_MODIFIED);  //, lmDO_ONLY_REDRAW);
}

bool lmCmdChangePageMargin::Undo()
{
    //Direct command. NO UNDO LOG

    ChangeMargin(m_uOldPos);
    return CommandUndone();
}

void lmCmdChangePageMargin::ChangeMargin(lmLUnits uPos)
{
    lmUSize size = m_pScore->GetPaperSize(m_nPage);

    switch(m_nIdx)
    {
        case lmMARGIN_TOP:
            m_pScore->SetPageTopMargin(uPos, m_nPage);
            break;

        case lmMARGIN_BOTTOM:
            m_pScore->SetPageBottomMargin(size.Height() - uPos, m_nPage);
            break;

        case lmMARGIN_LEFT:
            m_pScore->SetPageLeftMargin(uPos, m_nPage);
            break;

        case lmMARGIN_RIGHT:
            m_pScore->SetPageRightMargin(size.Width() - uPos, m_nPage);
            break;

        default:
            wxASSERT(false);
    }
}



//----------------------------------------------------------------------------------------
// lmCmdAttachNewText implementation
//----------------------------------------------------------------------------------------

lmCmdAttachNewText::lmCmdAttachNewText(const wxString& name, lmDocument *pDoc,
                                       lmComponentObj* pAnchor)
	: lmScoreCommand(name, pDoc, (lmVStaffCursor*)NULL )
{
	m_pAnchor = pAnchor;
    m_fDeleteText = false;

    //Create the text
    lmTextStyle* pStyle = pAnchor->GetScore()->GetStyleInfo(_("Normal text"));
    wxASSERT(pStyle);

    //create the text object
    wxString sText = _T("");
    m_pNewText = new lmTextItem(sText, lmHALIGN_DEFAULT, pStyle);

	//This is dirty: To use OnEditProperties() the text must be on the score. so I will
	//attach it provisionally to the score
	pAnchor->GetScore()->AttachAuxObj(m_pNewText);

    //show dialog to create the text
	lmDlgProperties dlg((lmController*)NULL);
	m_pNewText->OnEditProperties(&dlg);
	dlg.Layout();
	dlg.ShowModal();

	//dettach the text from the score
	pAnchor->GetScore()->DetachAuxObj(m_pNewText);
}

lmCmdAttachNewText::~lmCmdAttachNewText()
{
	if (m_pNewText && m_fDeleteText)
        delete m_pNewText;
}

bool lmCmdAttachNewText::Do()
{
    //Direct command. NO UNDO LOG

    m_pAnchor->AttachAuxObj(m_pNewText);
    m_fDeleteText = false;
	return CommandDone(lmSCORE_MODIFIED);  //, lmDO_ONLY_REDRAW);
}

bool lmCmdAttachNewText::Undo()
{
    //Direct command. NO UNDO LOG

    m_pAnchor->DetachAuxObj(m_pNewText);
    m_fDeleteText = true;
    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdAddNewTitle implementation
//----------------------------------------------------------------------------------------

lmCmdAddNewTitle::lmCmdAddNewTitle(lmDocument *pDoc)
	: lmScoreCommand(_("add title"), pDoc, (lmVStaffCursor*)NULL )
{
    m_fDeleteTitle = false;

    //Create the text
    lmTextStyle* pStyle = pDoc->GetScore()->GetStyleInfo(_("Title"));
    wxASSERT(pStyle);

    //create the text object
    wxString sTitle = _T("");
    m_pNewTitle = new lmScoreTitle(sTitle, lmBLOCK_ALIGN_BOTH, lmHALIGN_DEFAULT,
								  lmVALIGN_DEFAULT, pStyle);

	//This is dirty: To use OnEditProperties() the text must be on the score. so I will
	//attach it provisionally to the score
	pDoc->GetScore()->AttachAuxObj(m_pNewTitle);

    //show dialog to create the text
	lmDlgProperties dlg((lmController*)NULL);
	m_pNewTitle->OnEditProperties(&dlg);
	dlg.Layout();
	if (dlg.ShowModal() == wxID_OK)
        pDoc->GetScore()->OnPropertiesChanged();

	//dettach the text from the score
	pDoc->GetScore()->DetachAuxObj(m_pNewTitle);
}

lmCmdAddNewTitle::~lmCmdAddNewTitle()
{
	if (m_pNewTitle && m_fDeleteTitle)
        delete m_pNewTitle;
}

bool lmCmdAddNewTitle::Do()
{
    //Direct command. NO UNDO LOG

    if (m_pNewTitle->GetText() != _T(""))
    {
		m_pDoc->GetScore()->AttachAuxObj(m_pNewTitle);
		m_fDeleteTitle = false;
		return CommandDone(lmSCORE_MODIFIED);
    }
    else
    {
        m_fDeleteTitle = true;
        return false;
    }
}

bool lmCmdAddNewTitle::Undo()
{
    //Direct command. NO UNDO LOG

    m_pDoc->GetScore()->DetachAuxObj(m_pNewTitle);
    m_fDeleteTitle = true;
    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdChangeBarline implementation
//----------------------------------------------------------------------------------------

lmCmdChangeBarline::lmCmdChangeBarline(lmDocument *pDoc, lmBarline* pBL,
									   lmEBarline nType, bool fVisible)
	: lmScoreCommand(_("change barline"), pDoc, (lmVStaffCursor*)NULL )
{
    m_pBL = pBL;
    m_nType = nType;
	m_fVisible = fVisible;
    m_nOldType = m_pBL->GetBarlineType();
	m_fOldVisible = m_pBL->IsVisible();
}

lmCmdChangeBarline::~lmCmdChangeBarline()
{
}

bool lmCmdChangeBarline::Do()
{
    //Direct command. NO UNDO LOG

    m_pBL->SetBarlineType(m_nType);
    m_pBL->SetVisible(m_fVisible);
	return CommandDone(lmSCORE_MODIFIED);
}

bool lmCmdChangeBarline::Undo()
{
    //Direct command. NO UNDO LOG

    m_pBL->SetBarlineType(m_nOldType);
    m_pBL->SetVisible(m_fOldVisible);
    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdChangeMidiSettings implementation
//----------------------------------------------------------------------------------------

lmCmdChangeMidiSettings::lmCmdChangeMidiSettings(lmDocument *pDoc,
                                                 lmInstrument* pInstr,
                                                 int nMidiChannel,
                                                 int nMidiInstr)
	: lmScoreCommand(_("change MIDI settings"), pDoc, (lmVStaffCursor*)NULL )
{
    m_pInstr = pInstr;
    m_nMidiChannel = nMidiChannel;
    m_nMidiInstr = nMidiInstr;
    m_nOldMidiChannel = pInstr->GetMIDIChannel();
    m_nOldMidiInstr = pInstr->GetMIDIInstrument();
}

lmCmdChangeMidiSettings::~lmCmdChangeMidiSettings()
{
}

bool lmCmdChangeMidiSettings::Do()
{
    //Direct command. NO UNDO LOG

    m_pInstr->SetMIDIChannel(m_nMidiChannel);
    m_pInstr->SetMIDIInstrument(m_nMidiInstr);
	return CommandDone(lmSCORE_MODIFIED, lmDO_ONLY_REDRAW);
}

bool lmCmdChangeMidiSettings::Undo()
{
    //Direct command. NO UNDO LOG

    m_pInstr->SetMIDIChannel(m_nOldMidiChannel);
    m_pInstr->SetMIDIInstrument(m_nOldMidiInstr);
    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdMoveNote implementation
//----------------------------------------------------------------------------------------

lmCmdMoveNote::lmCmdMoveNote(lmDocument *pDoc, lmNote* pNote, const lmUPoint& uPos,
							 int nSteps)
	: lmScoreCommand(_("move note"), pDoc, (lmVStaffCursor*)NULL )
{
	m_uxPos = uPos.x;	//(g_fFreeMove ? uPos.y : pNote->GetUserShift().y);
	m_pNote = pNote;
    m_nSteps = nSteps;
}

bool lmCmdMoveNote::Do()
{
	m_pNote->ChangePitch(m_nSteps);
    m_uxOldPos = m_pNote->SetUserXLocation(m_uxPos);

	return CommandDone(lmSCORE_MODIFIED);
}

bool lmCmdMoveNote::Undo()
{
    //Direct command. NO UNDO LOG

	m_pNote->SetUserXLocation(m_uxOldPos);
	m_pNote->ChangePitch(-m_nSteps);
    return CommandUndone();
}



//----------------------------------------------------------------------------------------
// lmCmdMoveObjectPoints implementation
//----------------------------------------------------------------------------------------

lmCmdMoveObjectPoints::lmCmdMoveObjectPoints(const wxString& name,
                               lmDocument *pDoc, lmGMObject* pGMO,
                               lmUPoint uShift[], int nNumPoints, bool fUpdateViews)
	: lmScoreCommand(name, pDoc, (lmVStaffCursor*)NULL, true, 0, fUpdateViews)
      , m_nNumPoints(nNumPoints)
{
    wxASSERT(nNumPoints > 0);

    //get additional info
	m_pSCO = pGMO->GetScoreOwner();
    m_nShapeIdx = pGMO->GetOwnerIDX();

    //allocate a vector to save shifts
    m_pShifts = new lmUPoint[m_nNumPoints];
    for(int i=0; i < m_nNumPoints; i++)
        *(m_pShifts+i) = uShift[i];
}

lmCmdMoveObjectPoints::~lmCmdMoveObjectPoints()
{
    delete[] m_pShifts;
}

bool lmCmdMoveObjectPoints::Do()
{
    //Direct command. NO UNDO LOG
    m_pSCO->MoveObjectPoints(m_nNumPoints, m_nShapeIdx, m_pShifts, true);  //true->add shifts
	return CommandDone(lmSCORE_MODIFIED, lmDO_ONLY_REDRAW);
}

bool lmCmdMoveObjectPoints::Undo()
{
    //Direct command. NO UNDO LOG
    m_pSCO->MoveObjectPoints(m_nNumPoints, m_nShapeIdx, m_pShifts, false);  //false->substract shifts
    return CommandUndone(lmDO_ONLY_REDRAW);
}


