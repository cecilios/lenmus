// RCS-ID: $Id: SoundEvents.h,v 1.3 2006/02/23 19:25:44 cecilios Exp $
//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2006 Cecilio Salmeron
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
/*! @file SoundEvents.h
    @brief Header file for classes realetd to sound events
    @ingroup sound_management
*/
#ifndef __SOUNDEVENTS_H__        //to avoid nested includes
#define __SOUNDEVENTS_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "../score/score.h"

//-----------------------------------------------------------------------------------------
/*! @class lmScoreHighlightEvent
    @ingroup sound_management
    @brief An event to signal different actions related to 
        highlighting / unhighlighting notes while they are being played.
*/
//-----------------------------------------------------------------------------------------

DECLARE_EVENT_TYPE( lmEVT_SCORE_HIGHLIGHT, -1 )

class lmScoreHighlightEvent : public wxEvent
{
public:
    lmScoreHighlightEvent(lmStaffObj* pSO,
                        EHighlightType nHighlightType,
                        int id=0 ) 
        : wxEvent(id, lmEVT_SCORE_HIGHLIGHT)
        {
            m_pSO = pSO;
            m_nHighlightType = nHighlightType;
        }

    // copy constructor
    lmScoreHighlightEvent(const lmScoreHighlightEvent& event) : wxEvent(event)
        {    m_nHighlightType = event.m_nHighlightType;
            m_pSO = event.m_pSO;
        }

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return new lmScoreHighlightEvent(*this); }

    // accessors
    lmStaffObj*    GetStaffObj() { return m_pSO; }
    EHighlightType GetHighlightType() { return m_nHighlightType; }


private:
    EHighlightType    m_nHighlightType;    //event type: eVisualOn, eVisualOff, eRemoveAllHighlight
    lmStaffObj*        m_pSO;                //staffobj who must be highlighted / unhighlighted
};

typedef void (wxEvtHandler::*ScoreHighlightEventFunction)(lmScoreHighlightEvent&);

#define LM_EVT_SCORE_HIGHLIGHT(fn) \
    DECLARE_EVENT_TABLE_ENTRY( lmEVT_SCORE_HIGHLIGHT, wxID_ANY, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxNotifyEventFunction) \
    wxStaticCastEvent( ScoreHighlightEventFunction, & fn ), (wxObject *) NULL ),


//-----------------------------------------------------------------------------------------
/*! @class lmEndOfPlayEvent
    @ingroup sound_management
    @brief An event to signal end of playback
*/
//-----------------------------------------------------------------------------------------

DECLARE_EVENT_TYPE( lmEVT_END_OF_PLAY, -1 )

class lmEndOfPlayEvent : public wxEvent
{
public:
    lmEndOfPlayEvent(int id = 0 )    : wxEvent(id, lmEVT_END_OF_PLAY)
        {    m_propagationLevel = wxEVENT_PROPAGATE_MAX; }

    // copy constructor
    lmEndOfPlayEvent(const lmEndOfPlayEvent& event) : wxEvent(event) {}

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return new lmEndOfPlayEvent(*this); }

};

typedef void (wxEvtHandler::*EndOfPlayEventFunction)(lmEndOfPlayEvent&);

#define LM_EVT_END_OF_PLAY(fn) \
    DECLARE_EVENT_TABLE_ENTRY( lmEVT_END_OF_PLAY, wxID_ANY, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxNotifyEventFunction) \
    wxStaticCastEvent( EndOfPlayEventFunction, & fn ), (wxObject *) NULL ),


#endif    // __SOUNDEVENTS_H__