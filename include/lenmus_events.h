//---------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2010-2018 LenMus project
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
//---------------------------------------------------------------------------------------

#ifndef __LENMUS_EVENTS_H__        //to avoid nested includes
#define __LENMUS_EVENTS_H__

//lenmus
#include "lenmus_standard_header.h"

//lomse
#include <lomse_internal_model.h>
#include <lomse_events.h>
using namespace lomse;

//wxWidgets
//#define system ::system         //bypass for bug in wxcrtbase.h: "reference to 'system' is ambiguous"
#include <wx/wxprec.h>
#include <wx/wx.h>
#undef system                   //bypass for bug in wxcrtbase.h: "reference to 'system' is ambiguous"


namespace lenmus
{
class ExerciseCtrol;
class ExerciseOptions;
class ProblemManager;
class DlgCounters;

//---------------------------------------------------------------------------------------
// lmUpdateViewportEvent
//      An event to signal the need to repaint the window and to update scrollbars
//      due to an auto-scroll while the score is being played back.
//---------------------------------------------------------------------------------------

//DECLARE_EVENT_TYPE( lmEVT_UPDATE_VIEWPORT, -1 )
class lmUpdateViewportEvent;
wxDECLARE_EVENT( lmEVT_UPDATE_VIEWPORT, lmUpdateViewportEvent );

class lmUpdateViewportEvent : public wxEvent
{
private:
    SpEventUpdateViewport m_pEvent;   //lomse event

public:
    lmUpdateViewportEvent(SpEventUpdateViewport pEvent, int id = 0)
        : wxEvent(id, lmEVT_UPDATE_VIEWPORT)
        , m_pEvent(pEvent)
    {
    }

    // copy constructor
    lmUpdateViewportEvent(const lmUpdateViewportEvent& event)
        : wxEvent(event)
        , m_pEvent( event.m_pEvent )
    {
    }

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return LENMUS_NEW lmUpdateViewportEvent(*this); }

    // accessors
    SpEventUpdateViewport get_lomse_event() { return m_pEvent; }
};

typedef void (wxEvtHandler::*UpdateViewportEventFunction)(lmUpdateViewportEvent&);

#define UpdateViewportEventHandler(func) wxEVENT_HANDLER_CAST(UpdateViewportEventFunction, func)

#define LM_EVT_UPDATE_VIEWPORT(func) \
 	wx__DECLARE_EVT1( lmEVT_UPDATE_VIEWPORT, wxID_ANY, UpdateViewportEventHandler(func))


//---------------------------------------------------------------------------------------
// lmVisualTrackingEvent
//      An event to signal different actions related to
//      highlighting / unhighlighting notes while they are being played.
//---------------------------------------------------------------------------------------

//DECLARE_EVENT_TYPE( lmEVT_SCORE_HIGHLIGHT, -1 )
class lmVisualTrackingEvent;
wxDECLARE_EVENT( lmEVT_SCORE_HIGHLIGHT, lmVisualTrackingEvent );

class lmVisualTrackingEvent : public wxEvent
{
private:
    SpEventVisualTracking m_pEvent;   //lomse event

public:
    lmVisualTrackingEvent(SpEventVisualTracking pEvent, int id = 0)
        : wxEvent(id, lmEVT_SCORE_HIGHLIGHT)
        , m_pEvent(pEvent)
    {
    }

    // copy constructor
    lmVisualTrackingEvent(const lmVisualTrackingEvent& event)
        : wxEvent(event)
        , m_pEvent( event.m_pEvent )
    {
    }

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return LENMUS_NEW lmVisualTrackingEvent(*this); }

    // accessors
    SpEventVisualTracking get_lomse_event() { return m_pEvent; }
};

typedef void (wxEvtHandler::*VisualTrackingEventFunction)(lmVisualTrackingEvent&);

#define VisualTrackingEventHandler(func) wxEVENT_HANDLER_CAST(VisualTrackingEventFunction, func)

#define LM_EVT_SCORE_HIGHLIGHT(func) \
 	wx__DECLARE_EVT1( lmEVT_SCORE_HIGHLIGHT, wxID_ANY, VisualTrackingEventHandler(func))


//---------------------------------------------------------------------------------------
// lmEndOfPlaybackEvent: An event to signal end of playback
//---------------------------------------------------------------------------------------

//DECLARE_EVENT_TYPE( lmEVT_END_OF_PLAYBACK, -1 )
class lmEndOfPlaybackEvent;
wxDECLARE_EVENT( lmEVT_END_OF_PLAYBACK, lmEndOfPlaybackEvent );

class lmEndOfPlaybackEvent : public wxEvent
{
private:
    SpEventEndOfPlayback m_pEvent;   //lomse event

public:
    lmEndOfPlaybackEvent(SpEventEndOfPlayback pEvent, int id = 0 )
        : wxEvent(id, lmEVT_END_OF_PLAYBACK)
        , m_pEvent(pEvent)
    {
    }

    // copy constructor
    lmEndOfPlaybackEvent(const lmEndOfPlaybackEvent& event)
        : wxEvent(event)
        , m_pEvent( event.m_pEvent )
    {
    }

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return LENMUS_NEW lmEndOfPlaybackEvent(*this); }

    // accessors
    SpEventEndOfPlayback get_lomse_event() { return m_pEvent; }
};

typedef void (wxEvtHandler::*EndOfPlayEventFunction)(lmEndOfPlaybackEvent&);

#define EndOfPlayEventHandler(func) wxEVENT_HANDLER_CAST(EndOfPlayEventFunction, func)

#define LM_EVT_END_OF_PLAYBACK(func) \
 	wx__DECLARE_EVT1( lmEVT_END_OF_PLAYBACK, wxID_ANY, EndOfPlayEventHandler(func))



//---------------------------------------------------------------------------------------
// CountersEvent
//      An event to signal different actions related to DlgCounters
//---------------------------------------------------------------------------------------

class CountersEvent;
wxDECLARE_EVENT( EVT_COUNTERS_DLG, CountersEvent );

class CountersEvent : public wxEvent
{
private:
    ExerciseCtrol* m_pExercise;
    ExerciseOptions* m_pConstrains;
    ProblemManager* m_pProblemManager;
    DlgCounters* m_pDlg;

public:
    CountersEvent(ExerciseCtrol* pExercise, ExerciseOptions* pConstrains,
                  ProblemManager* pProblemManager, DlgCounters* pDlg, int id = 0)
        : wxEvent(id, EVT_COUNTERS_DLG)
        , m_pExercise(pExercise)
        , m_pConstrains(pConstrains)
        , m_pProblemManager(pProblemManager)
        , m_pDlg(pDlg)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    // copy constructor
    CountersEvent(const CountersEvent& event) : wxEvent(event)
    {
        m_pExercise = event.m_pExercise;
        m_pConstrains = event.m_pConstrains;
        m_pProblemManager = event.m_pProblemManager;
        m_pDlg = event.m_pDlg;
    }

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return LENMUS_NEW CountersEvent(*this); }

    // accessors
    ExerciseCtrol* get_exercise() { return m_pExercise; }
    ExerciseOptions* get_constrains() { return m_pConstrains; }
    ProblemManager* get_problem_manager() { return m_pProblemManager; }
    DlgCounters* get_dialog() { return m_pDlg; }

};

typedef void (wxEvtHandler::*CountersEventFunction)(CountersEvent&);

#define CountersEventHandler(func) wxEVENT_HANDLER_CAST(CountersEventFunction, func)

#define LM_EVT_COUNTERS_DLG(func) \
 	wx__DECLARE_EVT1( EVT_COUNTERS_DLG, wxID_ANY, CountersEventHandler(func))


//---------------------------------------------------------------------------------------
// PageRequestEvent: An event for requesting to display an eBook page
//---------------------------------------------------------------------------------------

class PageRequestEvent;
wxDECLARE_EVENT( lmEVT_PAGE_REQUEST, PageRequestEvent );

class PageRequestEvent : public wxEvent
{
private:
    string m_url;

public:
    PageRequestEvent(const string& url, int id = 0 )
        : wxEvent(id, lmEVT_PAGE_REQUEST)
        , m_url(url)
    {
    }

    // copy constructor
    PageRequestEvent(const PageRequestEvent& event)
        : wxEvent(event)
        , m_url( event.m_url )
    {
    }

    // clone constructor. Required for sending with wxPostEvent()
    virtual wxEvent *Clone() const { return LENMUS_NEW PageRequestEvent(*this); }

    // accessors
    string& get_url() { return m_url; }
};

typedef void (wxEvtHandler::*PageRequestEventFunction)(PageRequestEvent&);

#define PageRequestEventHandler(func) wxEVENT_HANDLER_CAST(PageRequestEventFunction, func)

#define LM_EVT_PAGE_REQUEST(func) \
 	wx__DECLARE_EVT1( lmEVT_PAGE_REQUEST, wxID_ANY, PageRequestEventHandler(func))


}   // namespace lenmus


#endif    // __LENMUS_EVENTS_H__
