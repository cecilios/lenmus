//---------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2013 LenMus project
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

//lenmus
#include "lenmus_command_event_handler.h"
#include "lenmus_tool_page_notes.h"
#include "lenmus_tool_page_clefs.h"
#include "lenmus_tool_page_barlines.h"
#include "lenmus_tool_page_symbols.h"
#include "lenmus_tool_box_events.h"
#include "lenmus_document_canvas.h"
#include "lenmus_string.h"

//lomse
#include <lomse_command.h>
#include <lomse_ldp_exporter.h>
#include <lomse_shapes.h>
#include <lomse_glyphs.h>
#include <lomse_tasks.h>
#include <lomse_clef_engraver.h>
#include <lomse_barline_engraver.h>
#include <lomse_note_engraver.h>
#include <lomse_rest_engraver.h>
#include <lomse_score_meter.h>
#include <lomse_document_cursor.h>
using namespace lomse;

////do not re-draw the score after executing the command
//#define lmNO_REDRAW    false


namespace lenmus
{

//=======================================================================================
// ToolsInfo implementation
//=======================================================================================
ToolsInfo::ToolsInfo()
	: m_pToolBox(NULL)
    , pageID(k_page_none)
    , groupID(k_grp_Undefined)
    , toolID(k_tool_none)
    , noteType(k_quarter)
	, dots(0)
    , notehead(k_notehead_quarter)
    , acc(k_no_accidentals)
    , octave(4)
    , voice(1)
    , fIsNote(true)
    , clefType(k_clef_G2)
    , barlineType(k_barline_simple)
    , mouseMode(k_mouse_mode_pointer)

//    //to save options selected by user in ToolBox
//    bool            m_fToolBoxSavedOptions;
//    int             m_nTbAcc;
//    int             m_nTbDots;
//    int             m_nTbDuration;
{
}

//---------------------------------------------------------------------------------------
void ToolsInfo::update_toolbox_info(ToolBox* pToolBox, SelectionSet& selection,
                                    DocCursor* cursor)
{
    //get current toolbox selections: page, group, tool and mouse mode

    m_pToolBox = pToolBox;
    m_pToolBox->synchronize_tools(&selection, cursor);
    get_toolbox_info();
    get_tool_page_values();
}

//---------------------------------------------------------------------------------------
void ToolsInfo::get_toolbox_info()
{
    groupID = m_pToolBox->GetCurrentGroupID();
    toolID = m_pToolBox->GetCurrentToolID();

    //if changed page or mouse moded changed, reconfigure toolbox for current mouse mode
    EToolPageID newPageID = m_pToolBox->GetCurrentPageID();
    int newMouseMode = m_pToolBox->GetMouseMode();
	if (newMouseMode != mouseMode || pageID != newPageID)
    {
        mouseMode = newMouseMode;
        pageID = newPageID;
        m_pToolBox->GetSelectedPage()->ReconfigureForMouseMode(mouseMode);
    }

    //get values for current page
    get_tool_page_values();

    //TDO: Transform this into a method in DocumentWindow to get current drag mark?
//    //set dragging marks for current page
//    switch(pageID)
//    {
//        case k_page_notes:
//            m_nToolMarks = lmMARK_TIME_GRID | lmMARK_LEDGER_LINES;
//            break;
//
//        case k_page_clefs:
//            switch (groupID)
//            {
//                case k_grp_ClefType:
//                case k_grp_TimeType:
//                case k_grp_KeyType:
//                default:
//                    m_nToolMarks = lmMARK_MEASURE;
//            }
//            break;
//
//        default:
//            m_nToolMarks = lmMARK_NONE;
//    }
}

//---------------------------------------------------------------------------------------
void ToolsInfo::get_tool_page_values()
{
    //get user selected values for current page.

	switch(pageID)
	{
        case k_page_notes:
        {
			ToolPageNotes* pNoteOptions = m_pToolBox->GetNoteProperties();
			noteType = pNoteOptions->GetNoteDuration();
			dots = pNoteOptions->GetNoteDots();
			notehead = pNoteOptions->GetNoteheadType();
			acc = pNoteOptions->GetNoteAccidentals();
			octave = pNoteOptions->GetOctave();
			voice = pNoteOptions->GetVoice();
            fIsNote = pNoteOptions->IsNoteSelected();
            break;
        }

        case k_page_clefs:
        {
            ToolPageClefs* pPage =
                static_cast<ToolPageClefs*>( m_pToolBox->GetSelectedPage() );
            clefType = pPage->GetSelectedClef();
            break;
        }

        case k_page_barlines:
        {
            ToolPageBarlines* pPage =
                static_cast<ToolPageBarlines*>( m_pToolBox->GetSelectedPage() );
            barlineType = pPage->GetSelectedBarline();
            break;
        }

        default:
            ;
    }
}

//---------------------------------------------------------------------------------------
void ToolsInfo::enable_tools(bool fEnable)
{
    if (m_pToolBox)
        m_pToolBox->enable_tools(fEnable);
}



//=======================================================================================
// CommandEventHandler implementation
//=======================================================================================
CommandEventHandler::CommandEventHandler(DocumentWindow* pController,
                                         ToolsInfo& toolsInfo,
                                         SelectionSet& selection,
                                         DocCursor* cursor)
    : m_pController(pController)
    , m_toolsInfo(toolsInfo)
    , m_selection(selection)
    , m_cursor(cursor)
    , m_executer(pController, selection, cursor)
{
}

//---------------------------------------------------------------------------------------
CommandEventHandler::~CommandEventHandler()
{
//    //delete mouse cursors
//    std::vector<wxCursor*>::iterator it;
//    for (it = m_MouseCursors.begin(); it != m_MouseCursors.end(); ++it)
//        delete *it;
//
//    //delete cursor drag images
//    if (m_pToolBitmap)
//        delete m_pToolBitmap;
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::process_key_event(wxKeyEvent& event)
{
    check_single_key_common_commands(event);
    if (!event_processed())
    {
        KeyHandler* handler = new_key_handler_for_current_context();
        handler->process_key(event);
#if (LENMUS_DEBUG_BUILD == 1)
        if (!handler->event_processed())
        {
            wxMessageBox(_T("[CommandEventHandler::process_key_event] Key pressed but not processed."));
//        LogKeyEvent(_T("Key Press"), event, nTool);
//        event.Skip();       //pass the event. Perhaps it is a menu shortcut
        }
#endif
        delete handler;
    }
//    {
//        //the command has been processed. Clear buffer
//        m_sCmd = _T("");
//    }
//
//	//Display command
//    //GetMainFrame()->SetStatusBarMsg(wxString::Format(_T("cmd: %s"), m_sCmd.c_str() ));
}

//---------------------------------------------------------------------------------------
KeyHandler* CommandEventHandler::new_key_handler_for_current_context()
{
    //factory method for generating a key event handler

    switch(m_toolsInfo.pageID)
    {
        case k_page_none:
            return LENMUS_NEW NoToolKeyHandler(m_pController, m_toolsInfo,
                                               m_selection, m_cursor);
        case k_page_clefs:
            return LENMUS_NEW ClefsKeyHandler(m_pController, m_toolsInfo,
                                              m_selection, m_cursor);
        case k_page_notes:
            return LENMUS_NEW NotesKeyHandler(m_pController, m_toolsInfo,
                                              m_selection, m_cursor);
        case k_page_barlines:
            return LENMUS_NEW BarlinesKeyHandler(m_pController, m_toolsInfo,
                                                 m_selection, m_cursor);
        case k_page_symbols:
            return LENMUS_NEW SymbolsKeyHandler(m_pController, m_toolsInfo,
                                                m_selection, m_cursor);
        default:
            return LENMUS_NEW NullKeyHandler(m_pController, m_toolsInfo,
                                             m_selection, m_cursor);
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::process_on_click_event(SpEventMouse event)
{
    //For now, on click events are only for positioning DocCursor

    m_fEventProcessed = false;
    if (m_pController->is_edition_enabled())
    {
        if (m_toolsInfo.is_mouse_data_entry_mode())
        {
            //TODO: Modify ToolsInfo for objects other than scores
            //TODO: Add code to deal with dragging:
            //      Following code assumes a click for inserting something, but old
            //      code also deals with dragging, changing mouse pointer and other issues.
            //      See:
            //      CommandEventHandler::OnMouseEventToolMode(wxMouseEvent& event, wxDC* pDC)

            ClickHandler* handler = new_click_handler_for_current_context();
            handler->process_click(event);
#if (LENMUS_DEBUG_BUILD == 1)
            if (!handler->event_processed())
                wxMessageBox(_T("[CommandEventHandler::process_on_click_event] Click event not processed."));
#endif
            delete handler;
        }
        else
        {
            //select object or move caret to click point

            //TODO: This code is too specific for editing scores. It should be
            //      moved to another place and take into account more top level objects.

            unsigned flags = event->get_flags();
            ImoObj* pImo = event->get_imo_object();
            if (pImo->is_staffobj())
            {
                bool fAddToSeleccion = flags & k_kbd_ctrl;
                SpInteractor spIntor = m_pController->get_interactor_shared_ptr();
                spIntor->select_object(pImo, !fAddToSeleccion);
                spIntor->force_redraw();
            }
            else
                m_executer.move_caret_to_click_point(event);
        }
        m_fEventProcessed = true;
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::process_command_event(SpEventCommand event)
{
    m_fEventProcessed = false;
    if (m_pController->is_edition_enabled())
    {
        if (event->is_control_point_moved_event())
        {
            SpEventControlPointMoved pEv(
                    boost::static_pointer_cast<EventControlPointMoved>(event) );
            UPoint shift = pEv->get_shift();
            int iPoint = pEv->get_handler_index();
//            int gmoType = pEv->get_gmo_type();
//            ShapeId idx = pEv->get_shape_index();
            m_executer.move_object_point(iPoint, shift);
            m_fEventProcessed = true;
        }
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::move_caret_to_click_point(SpEventMouse event)
{
    SpInteractor spIntor = m_pController->get_interactor_shared_ptr();
    DocCursorState state = spIntor->click_event_to_cursor_state(event);
    if (state.get_top_level_id() != k_no_imoid)
        m_pController->exec_lomse_command(
            LENMUS_NEW CmdCursor(state), k_show_busy);
}

//---------------------------------------------------------------------------------------
ClickHandler* CommandEventHandler::new_click_handler_for_current_context()
{
    //factory method for generating a mouse click event handler

//void CommandEventHandler::OnToolClick(GmoObj* pGMO, UPoint uPagePos, TimeUnits rGridTime)
//    //Mouse click on a valid area while dragging a tool. Determine user action and issue
//    //the appropriate edition command
//    //AWARE: Clicks on non-valid areas are filtered-out before arriving to this method
//
//    if(!pGMO) return;

    switch(m_toolsInfo.pageID)
    {
        case k_page_notes:
            return LENMUS_NEW NoteRestClickHandler(m_pController, m_toolsInfo,
                                                m_selection, m_cursor);
//            return OnToolNotesClick(pGMO, uPagePos, rGridTime);

//        case k_page_sysmbols:
//        {
//            switch (groupID)
//            {
//                case k_grp_Symbols:
//                    return OnToolSymbolsClick(pGMO, uPagePos, rGridTime);
//                case k_grp_Harmony:
//                    return OnToolHarmonyClick(pGMO, uPagePos, rGridTime);
//                default:
//                    wxLogMessage(_T("[CommandEventHandler::OnToolClick] Missing value (%d) in switch statement"), groupID);
//                    return;
//            }
//        }

        case k_page_barlines:
            return LENMUS_NEW BarlineClickHandler(m_pController, m_toolsInfo,
                                                  m_selection, m_cursor);

        case k_page_clefs:
        {
            switch (m_toolsInfo.groupID)
            {
                case k_grp_ClefType:
                    return LENMUS_NEW ClefClickHandler(m_pController, m_toolsInfo,
                                                       m_selection, m_cursor);
//                case k_grp_TimeType:
//                    return OnToolTimeSignatureClick(pGMO, uPagePos, rGridTime);
//                case k_grp_KeyType:
//                    return OnToolKeySignatureClick(pGMO, uPagePos, rGridTime);
                default:
                    stringstream msg;
                    msg << "Missing value (" << m_toolsInfo.groupID
                        << ") in switch statement.";
                    LOMSE_LOG_ERROR(msg.str());
                    return LENMUS_NEW NullClickHandler(m_pController, m_toolsInfo,
                                                       m_selection, m_cursor);
            }
        }

        default:
            return LENMUS_NEW NullClickHandler(m_pController, m_toolsInfo,
                                               m_selection, m_cursor);
    }

//////    switch(m_toolsInfo.pageID)
//////    {
//////        case k_page_none:
//////            return LENMUS_NEW NoToolKeyHandler(m_pController, m_toolsInfo,
//////                                               m_selection, m_cursor);
//////        case k_page_clefs:
//////            return LENMUS_NEW ClefsKeyHandler(m_pController, m_toolsInfo,
//////                                              m_selection, m_cursor);
//////        case k_page_notes:
//////            return LENMUS_NEW NotesKeyHandler(m_pController, m_toolsInfo,
//////                                              m_selection, m_cursor);
//////        case k_page_barlines:
//////            return LENMUS_NEW BarlinesKeyHandler(m_pController, m_toolsInfo,
//////                                                 m_selection, m_cursor);
//////        case k_page_symbols:
//////            return LENMUS_NEW SymbolsKeyHandler(m_pController, m_toolsInfo,
//////                                                m_selection, m_cursor);
//////        default:
//////            return LENMUS_NEW NullKeyHandler(m_pController, m_toolsInfo,
//////                                             m_selection, m_cursor);
//////    }
}
//---------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------
void CommandEventHandler::process_page_changed_in_toolbox_event(ToolBox* pToolBox)
{
    common_tasks_for_toolbox_event(pToolBox);
    m_fEventProcessed = true;
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::process_tool_event(EToolID toolID, EToolGroupID groupID,
                                             ToolBox* pToolBox)
{
    m_fEventProcessed = false;
    common_tasks_for_toolbox_event(pToolBox);
    if (groupID == k_grp_MouseMode)
    {
        switch_interactor_mode_for_current_mouse_mode();
        set_drag_image_for_current_tool();
    }
    else
    {
        if (!m_selection.empty())
            command_on_selection(toolID, groupID);
        else
        {
            command_on_caret_pointed_object(toolID, groupID);
            if (!m_fEventProcessed && groupID == k_grp_Voice)
            {
                SpInteractor spInteractor = m_pController->get_interactor_shared_ptr();
                spInteractor->highlight_voice( m_toolsInfo.voice );
                spInteractor->force_redraw();
            }
        }
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::common_tasks_for_toolbox_event(ToolBox* pToolBox)
{
//    SpInteractor spInteractor = m_pController->get_interactor_shared_ptr();
    m_toolsInfo.update_toolbox_info(pToolBox, m_selection, m_cursor);

    set_drag_image_for_current_tool();

//    //determine valid areas and change icons
//    UpdateValidAreasAndMouseIcons();
//
//    //update status bar: mouse mode and selected tool
//    UpdateStatusBarToolBox(); --> update_status_bar_toolbox_data();
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::command_on_caret_pointed_object(EToolID toolID,
                                                          EToolGroupID groupID)
{
    switch (groupID)
    {
        case k_grp_Beams:   //Beam tools ------------------------------------------------
        {
            switch(toolID)
            {
                case k_tool_beams_cut:
                {
                    m_executer.break_beam();
                    m_fEventProcessed = true;
                    return;
                }

                default:
                    return;
            }
            return;
        }

        default:
            return;
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::command_on_selection(EToolID toolID, EToolGroupID groupID)
{
    switch (groupID)
    {
        case k_grp_NoteAcc: //selection of accidentals ----------------------------------
        {
            EAccidentals nAcc = m_toolsInfo.acc;
            m_executer.change_note_accidentals(nAcc);
            m_fEventProcessed = true;
            return;
        }

        case k_grp_NoteDots:    //selection of dots -------------------------------------
        {
            m_executer.change_dots(m_toolsInfo.dots);
            m_fEventProcessed = true;
            return;
        }

        case k_grp_NoteModifiers:   //Tie, Tuplet tools ---------------------------------
        {
            switch(toolID)
            {
                case k_tool_note_tie:
                {
                    ImoNote* pStartNote;
                    ImoNote* pEndNote;
                    if (m_selection.is_valid_to_add_tie(&pStartNote, &pEndNote))
                    {
                        m_executer.add_tie();
                        m_fEventProcessed = true;
                    }
                    else if (m_selection.is_valid_to_remove_tie())
                    {
                        m_executer.delete_tie();
                        m_fEventProcessed = true;
                    }
                    return;
                }

                case k_tool_note_tuplet:
                {
                    if (m_selection.is_valid_to_add_tuplet())
                    {
                        m_executer.add_tuplet();
                        m_fEventProcessed = true;
                    }
                    else if (m_selection.is_valid_to_remove_tuplet())
                    {
                        m_executer.delete_tuplet();
                        m_fEventProcessed = true;
                    }
                    return;
                }

                case k_tool_note_toggle_stem:
                {
                    m_executer.toggle_stem();
                    m_fEventProcessed = true;
                    return;
                }

                default:
                    return;
            }
            return;
        }

        case k_grp_Beams:   //Beam tools ------------------------------------------------
        {
            switch(toolID)
            {
                case k_tool_beams_join:
                {
                    m_executer.join_beam();
                    m_fEventProcessed = true;
                    return;
                }

                case k_tool_beams_flatten:
                case k_tool_beams_subgroup:
                {
                    //TODO
                    return;
                }

                default:
                    return;
            }
            return;
        }

        default:
            return;
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::switch_interactor_mode_for_current_mouse_mode()
{
    SpInteractor spInteractor = m_pController->get_interactor_shared_ptr();
    switch(m_toolsInfo.mouseMode)
    {
        case k_mouse_mode_data_entry:
            spInteractor->switch_task(TaskFactory::k_task_data_entry);
            break;

        case k_mouse_mode_pointer:
        default:
            spInteractor->switch_task(TaskFactory::k_task_selection);
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::set_drag_image_for_current_tool()
{
    SpInteractor spInteractor = m_pController->get_interactor_shared_ptr();
    if (m_toolsInfo.is_mouse_data_entry_mode())
    {
        LibraryScope& libScope = m_pController->get_library_scope();

        GmoShape* pShape = NULL;
        UPoint offset(0.0, 0.0);
        switch(m_toolsInfo.pageID)
        {
            case k_page_clefs:
            {
                ClefEngraver engraver(libScope);
                int clefType = int(m_toolsInfo.clefType);
                pShape = engraver.create_tool_dragged_shape(clefType);
                offset = engraver.get_drag_offset();
                break;
            }

            case k_page_notes:
            {
                ScoreMeter scoreMeter( m_pController->get_active_score() );
                int noteType = int(m_toolsInfo.noteType);
                int dots = m_toolsInfo.dots;
                EAccidentals acc = m_toolsInfo.acc;
                if (m_toolsInfo.fIsNote)
                {
                    NoteEngraver engraver(libScope, &scoreMeter, NULL, 0, 0);
                    pShape = engraver.create_tool_dragged_shape(noteType, acc, dots);
                    offset = engraver.get_drag_offset();
                }
                else
                {
                    RestEngraver engraver(libScope, &scoreMeter, NULL, 0, 0);
                    pShape = engraver.create_tool_dragged_shape(noteType, dots);
                    offset = engraver.get_drag_offset();
                }
                break;
            }

            case k_page_symbols:
            {
                //TODO: create drag shape
                break;
            }

            case k_page_barlines:
            {
                BarlineEngraver engraver(libScope);
                int barlineType = int(m_toolsInfo.barlineType);
                pShape = engraver.create_tool_dragged_shape(barlineType);
                offset = engraver.get_drag_offset();
                break;
            }

            default:
                ;   //TODO: Create questión mark shape
        }
        spInteractor->set_drag_image(pShape, k_get_ownership, offset);
        spInteractor->show_drag_image(true);
    }
    else
    {
        spInteractor->set_drag_image(NULL, k_get_ownership, UPoint(0.0, 0.0));
        spInteractor->show_drag_image(false);
    }
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::delete_selection_or_pointed_object()
{
    if (!m_selection.empty())
        m_executer.delete_selection();
    else
        m_executer.delete_staffobj();
    m_fEventProcessed = true;
}

//---------------------------------------------------------------------------------------
void CommandEventHandler::check_single_key_common_commands(wxKeyEvent& event)
{

    //commands only valid if document edition is enabled
    if (m_pController->is_edition_enabled())
    {
        switch (event.GetKeyCode())
        {
            //cursor keys
            case WXK_LEFT:
            case WXK_UP:
                m_pController->exec_lomse_command(
                    LENMUS_NEW CmdCursor(CmdCursor::k_move_prev), k_no_show_busy);
                m_fEventProcessed = true;
                return;

            case WXK_RIGHT:
            case WXK_DOWN:
                m_pController->exec_lomse_command(
                    LENMUS_NEW CmdCursor(CmdCursor::k_move_next), k_no_show_busy);
                m_fEventProcessed = true;
                return;

    //		case WXK_UP:
    //            exec_command("c out");
    //            return true;
    //
    //		case WXK_DOWN:
    //            exec_command("c in");
    //            return true;

            case WXK_RETURN:
                if (event.ControlDown())
                    m_pController->exec_lomse_command(
                        LENMUS_NEW CmdCursor(CmdCursor::k_exit), k_no_show_busy);
                else
                    m_pController->exec_lomse_command(
                        LENMUS_NEW CmdCursor(CmdCursor::k_enter), k_no_show_busy);
                m_fEventProcessed = true;
                return;

            case WXK_DELETE:
                delete_selection_or_pointed_object();
                return;

    //        case WXK_BACK:
    //			m_pView->CaretLeft(false);      //false: treat chords as a single object
    //			delete_selection_or_pointed_object();
    //			return;

            default:
                ;
        }
    }

    //commands also valid when documen edition is disabled

    //fix ctrol+key codes
    int nKeyCode = event.GetKeyCode();
    unsigned flags = get_keyboard_flags(event);
    if (nKeyCode > 0 && nKeyCode < 27)
    {
        nKeyCode += int('A') - 1;
        flags |= k_kbd_ctrl;
    }

    switch (nKeyCode)
    {
        case '+':
            if (flags && k_kbd_ctrl)
            {
                m_pController->zoom_in();
                m_fEventProcessed = true;
            }
            return;

        case '-':
            if (flags && k_kbd_ctrl)
            {
                m_pController->zoom_out();
                m_fEventProcessed = true;
            }
            return;

        default:
            ;
    }
        return;
}

//---------------------------------------------------------------------------------------
unsigned CommandEventHandler::get_keyboard_flags(wxKeyEvent& event)
{
    unsigned flags = 0;
    if (event.ShiftDown())   flags |= k_kbd_shift;
    if (event.AltDown()) flags |= k_kbd_alt;
    if (event.ControlDown()) flags |= k_kbd_ctrl;
    return flags;
}


////---------------------------------------------------------------------------------------
//void CommandEventHandler::LogKeyEvent(wxString name, wxKeyEvent& event, int nTool)
//{
//    wxString key = KeyCodeToName( event.GetKeyCode() );
//    key += wxString::Format(_T(" (Unicode: %#04x)"), event.GetUnicodeKey());
//
//    wxLogMessage( wxString::Format( _T("[CommandEventHandler::LogKeyEvent] Event: %s - %s, nKeyCode=%d, (flags = %c%c%c%c). Tool=%d"),
//            name.c_str(), key.c_str(), event.GetKeyCode(),
//            (event.CmdDown() ? _T('C') : _T('-') ),
//            (event.AltDown() ? _T('A') : _T('-') ),
//            (event.ShiftDown() ? _T('S') : _T('-') ),
//            (event.MetaDown() ? _T('M') : _T('-') ),
//            nTool ));
//}
//
////---------------------------------------------------------------------------------------
//wxString CommandEventHandler::KeyCodeToName(int nKeyCode)
//{
//    wxString sKey;
//    switch ( nKeyCode )
//    {
//        case WXK_BACK: sKey = _T("BACK"); break;
//        case WXK_TAB: sKey = _T("TAB"); break;
//        case WXK_RETURN: sKey = _T("RETURN"); break;
//        case WXK_ESCAPE: sKey = _T("ESCAPE"); break;
//        case WXK_SPACE: sKey = _T("SPACE"); break;
//        case WXK_DELETE: sKey = _T("DELETE"); break;
//
//        case WXK_START: sKey = _T("START"); break;
//        case WXK_LBUTTON: sKey = _T("LBUTTON"); break;
//        case WXK_RBUTTON: sKey = _T("RBUTTON"); break;
//        case WXK_CANCEL: sKey = _T("CANCEL"); break;
//        case WXK_MBUTTON: sKey = _T("MBUTTON"); break;
//        case WXK_CLEAR: sKey = _T("CLEAR"); break;
//        case WXK_SHIFT: sKey = _T("SHIFT"); break;
//        case WXK_ALT: sKey = _T("ALT"); break;
//        case WXK_CONTROL: sKey = _T("CONTROL"); break;
//        case WXK_MENU: sKey = _T("MENU"); break;
//        case WXK_PAUSE: sKey = _T("PAUSE"); break;
//        case WXK_CAPITAL: sKey = _T("CAPITAL"); break;
//        case WXK_END: sKey = _T("END"); break;
//        case WXK_HOME: sKey = _T("HOME"); break;
//        case WXK_LEFT: sKey = _T("LEFT"); break;
//        case WXK_UP: sKey = _T("UP"); break;
//        case WXK_RIGHT: sKey = _T("RIGHT"); break;
//        case WXK_DOWN: sKey = _T("DOWN"); break;
//        case WXK_SELECT: sKey = _T("SELECT"); break;
//        case WXK_PRINT: sKey = _T("PRINT"); break;
//        case WXK_EXECUTE: sKey = _T("EXECUTE"); break;
//        case WXK_SNAPSHOT: sKey = _T("SNAPSHOT"); break;
//        case WXK_INSERT: sKey = _T("INSERT"); break;
//        case WXK_HELP: sKey = _T("HELP"); break;
//        case WXK_NUMPAD0: sKey = _T("NUMPAD0"); break;
//        case WXK_NUMPAD1: sKey = _T("NUMPAD1"); break;
//        case WXK_NUMPAD2: sKey = _T("NUMPAD2"); break;
//        case WXK_NUMPAD3: sKey = _T("NUMPAD3"); break;
//        case WXK_NUMPAD4: sKey = _T("NUMPAD4"); break;
//        case WXK_NUMPAD5: sKey = _T("NUMPAD5"); break;
//        case WXK_NUMPAD6: sKey = _T("NUMPAD6"); break;
//        case WXK_NUMPAD7: sKey = _T("NUMPAD7"); break;
//        case WXK_NUMPAD8: sKey = _T("NUMPAD8"); break;
//        case WXK_NUMPAD9: sKey = _T("NUMPAD9"); break;
//        case WXK_MULTIPLY: sKey = _T("MULTIPLY"); break;
//        case WXK_ADD: sKey = _T("ADD"); break;
//        case WXK_SEPARATOR: sKey = _T("SEPARATOR"); break;
//        case WXK_SUBTRACT: sKey = _T("SUBTRACT"); break;
//        case WXK_DECIMAL: sKey = _T("DECIMAL"); break;
//        case WXK_DIVIDE: sKey = _T("DIVIDE"); break;
//        case WXK_F1: sKey = _T("F1"); break;
//        case WXK_F2: sKey = _T("F2"); break;
//        case WXK_F3: sKey = _T("F3"); break;
//        case WXK_F4: sKey = _T("F4"); break;
//        case WXK_F5: sKey = _T("F5"); break;
//        case WXK_F6: sKey = _T("F6"); break;
//        case WXK_F7: sKey = _T("F7"); break;
//        case WXK_F8: sKey = _T("F8"); break;
//        case WXK_F9: sKey = _T("F9"); break;
//        case WXK_F10: sKey = _T("F10"); break;
//        case WXK_F11: sKey = _T("F11"); break;
//        case WXK_F12: sKey = _T("F12"); break;
//        case WXK_F13: sKey = _T("F13"); break;
//        case WXK_F14: sKey = _T("F14"); break;
//        case WXK_F15: sKey = _T("F15"); break;
//        case WXK_F16: sKey = _T("F16"); break;
//        case WXK_F17: sKey = _T("F17"); break;
//        case WXK_F18: sKey = _T("F18"); break;
//        case WXK_F19: sKey = _T("F19"); break;
//        case WXK_F20: sKey = _T("F20"); break;
//        case WXK_F21: sKey = _T("F21"); break;
//        case WXK_F22: sKey = _T("F22"); break;
//        case WXK_F23: sKey = _T("F23"); break;
//        case WXK_F24: sKey = _T("F24"); break;
//        case WXK_NUMLOCK: sKey = _T("NUMLOCK"); break;
//        case WXK_SCROLL: sKey = _T("SCROLL"); break;
//        case WXK_PAGEUP: sKey = _T("PAGEUP"); break;
//        case WXK_PAGEDOWN: sKey = _T("PAGEDOWN"); break;
//
//        case WXK_NUMPAD_SPACE: sKey = _T("NUMPAD_SPACE"); break;
//        case WXK_NUMPAD_TAB: sKey = _T("NUMPAD_TAB"); break;
//        case WXK_NUMPAD_ENTER: sKey = _T("NUMPAD_ENTER"); break;
//        case WXK_NUMPAD_F1: sKey = _T("NUMPAD_F1"); break;
//        case WXK_NUMPAD_F2: sKey = _T("NUMPAD_F2"); break;
//        case WXK_NUMPAD_F3: sKey = _T("NUMPAD_F3"); break;
//        case WXK_NUMPAD_F4: sKey = _T("NUMPAD_F4"); break;
//        case WXK_NUMPAD_HOME: sKey = _T("NUMPAD_HOME"); break;
//        case WXK_NUMPAD_LEFT: sKey = _T("NUMPAD_LEFT"); break;
//        case WXK_NUMPAD_UP: sKey = _T("NUMPAD_UP"); break;
//        case WXK_NUMPAD_RIGHT: sKey = _T("NUMPAD_RIGHT"); break;
//        case WXK_NUMPAD_DOWN: sKey = _T("NUMPAD_DOWN"); break;
//        case WXK_NUMPAD_PAGEUP: sKey = _T("NUMPAD_PAGEUP"); break;
//        case WXK_NUMPAD_PAGEDOWN: sKey = _T("NUMPAD_PAGEDOWN"); break;
//        case WXK_NUMPAD_END: sKey = _T("NUMPAD_END"); break;
//        case WXK_NUMPAD_BEGIN: sKey = _T("NUMPAD_BEGIN"); break;
//        case WXK_NUMPAD_INSERT: sKey = _T("NUMPAD_INSERT"); break;
//        case WXK_NUMPAD_DELETE: sKey = _T("NUMPAD_DELETE"); break;
//        case WXK_NUMPAD_EQUAL: sKey = _T("NUMPAD_EQUAL"); break;
//        case WXK_NUMPAD_MULTIPLY: sKey = _T("NUMPAD_MULTIPLY"); break;
//        case WXK_NUMPAD_ADD: sKey = _T("NUMPAD_ADD"); break;
//        case WXK_NUMPAD_SEPARATOR: sKey = _T("NUMPAD_SEPARATOR"); break;
//        case WXK_NUMPAD_SUBTRACT: sKey = _T("NUMPAD_SUBTRACT"); break;
//        case WXK_NUMPAD_DECIMAL: sKey = _T("NUMPAD_DECIMAL"); break;
//        case WXK_NUMPAD_DIVIDE: sKey = _T("NUMPAD_DIVIDE"); break;
//
//        // the following key codes are only generated under Windows currently
//         case WXK_WINDOWS_LEFT: sKey = _T("WINDOWS_LEFT"); break;
//         case WXK_WINDOWS_RIGHT: sKey = _T("WINDOWS_RIGHT"); break;
//         case WXK_WINDOWS_MENU: sKey = _T("WINDOWS_MENU"); break;
//         case WXK_COMMAND: sKey = _T("COMMAND"); break;
//
//        // Hardware-specific buttons
//         case WXK_SPECIAL1: sKey = _T("SPECIAL1"); break;
//         case WXK_SPECIAL2: sKey = _T("SPECIAL2"); break;
//         case WXK_SPECIAL3: sKey = _T("SPECIAL3"); break;
//         case WXK_SPECIAL4: sKey = _T("SPECIAL4"); break;
//         case WXK_SPECIAL5: sKey = _T("SPECIAL5"); break;
//         case WXK_SPECIAL6: sKey = _T("SPECIAL6"); break;
//         case WXK_SPECIAL7: sKey = _T("SPECIAL7"); break;
//         case WXK_SPECIAL8: sKey = _T("SPECIAL8"); break;
//         case WXK_SPECIAL9: sKey = _T("SPECIAL9"); break;
//         case WXK_SPECIAL10: sKey = _T("SPECIAL10"); break;
//         case WXK_SPECIAL11: sKey = _T("SPECIAL11"); break;
//         case WXK_SPECIAL12: sKey = _T("SPECIAL12"); break;
//         case WXK_SPECIAL13: sKey = _T("SPECIAL13"); break;
//         case WXK_SPECIAL14: sKey = _T("SPECIAL14"); break;
//         case WXK_SPECIAL15: sKey = _T("SPECIAL15"); break;
//         case WXK_SPECIAL16: sKey = _T("SPECIAL16"); break;
//         case WXK_SPECIAL17: sKey = _T("SPECIAL17"); break;
//         case WXK_SPECIAL18: sKey = _T("SPECIAL18"); break;
//         case WXK_SPECIAL19: sKey = _T("SPECIAL19"); break;
//         case WXK_SPECIAL20: sKey = _T("SPECIAL20"); break;
//
//
//        default:
//        {
//            if ( wxIsprint((int)nKeyCode) )
//                sKey.Printf(_T("'%c'"), (char)nKeyCode);
//            else if ( nKeyCode > 0 && nKeyCode < 27 )
//                sKey.Printf(_T("Ctrl-%c"), _T('A') + nKeyCode - 1);
//            else
//                sKey.Printf(_T("unknown (%d)"), nKeyCode);
//        }
//    }
//    return sKey;
//}


//=======================================================================================
// CommandGenerator implementation
//=======================================================================================
CommandGenerator::CommandGenerator(DocumentWindow* pController, SelectionSet& selection,
                                   DocCursor* cursor)
    : m_pController(pController)
    , m_selection(selection)
    , m_cursor(cursor)
{
}

//---------------------------------------------------------------------------------------
void CommandGenerator::add_tie()
{
    //Tie the selected notes

	string name = to_std_string(_("Add tie"));
	m_pController->exec_lomse_command( LENMUS_NEW CmdAddTie(name) );
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::AddTitle()
//{
//    //Create a new block of text and attach it to the score
//
//    //create the new text.
//    //Text creation requires to create an empty TextItem and editing it using the properties
//    //dialog. And this, in turn, requires the TextItem to edit to be already included in the
//    //score. Therefore, I will attach it provisionally to the score
//
//    lmScore* pScore = m_pDoc->GetScore();
//    lmTextStyle* pStyle = pScore->GetStyleInfo(_("Title"));
//    wxASSERT(pStyle);
//    wxString sTitle = _T("");
//    lmScoreTitle* pNewTitle
//        = new lmScoreTitle(pScore, lmNEW_ID, sTitle, lmBLOCK_ALIGN_BOTH,
//                           lmHALIGN_DEFAULT, lmVALIGN_DEFAULT, pStyle);
//	pScore->AttachAuxObj(pNewTitle);
//
//    //show dialog to create the text
//	DlgProperties dlg((DocumentWindow*)NULL);
//	pNewTitle->OnEditProperties(&dlg);
//	dlg.Layout();
//	if (dlg.ShowModal() == wxID_OK)
//        pScore->OnPropertiesChanged();
//
//    //get title info
//    sTitle = pNewTitle->GetText();
//    pStyle = pNewTitle->GetStyle();
//    lmEHAlign nAlign = pNewTitle->GetAlignment();
//
//	//dettach the text from the score and delete the text item
//	pScore->DetachAuxObj(pNewTitle);
//    delete pNewTitle;
//
//    //Now issue the command to attach the title to to the score
//	wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//    if (sTitle != _T(""))
//	    m_pController->exec_lomse_command(LENMUS_NEW CmdAddTitle(lmCMD_NORMAL, m_pDoc, sTitle, pStyle,
//                                      nAlign));
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::add_tuplet()
{
    // Add a tuplet to the selected notes/rests (there could be other objects selected
    // beetween the notes)
    //
    // Precondition:
    //      it has been checked that all notes/rest in the seleccion are not in a tuplet,
    //      are consecutive and are in the same voice.

    ImoNoteRest* pStart = NULL;
    ImoNoteRest* pEnd = NULL;
	m_selection.get_start_end_note_rests(&pStart, &pEnd);
	if (pStart && pEnd)
	{
        string name = to_std_string(_("Add tuplet"));
        m_pController->exec_lomse_command(
            LENMUS_NEW CmdAddTuplet("(t + 2 3)", name) );
	}
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::AttachNewText(lmComponentObj* pTarget)
//{
//    //Create a new text and attach it to the received object
//
//    //create the new text.
//    //Text creation requires to create an empty TextItem and editing it using the properties
//    //dialog. And this, in turn, requires the TextItem to edit to be already included in the
//    //score. Therefore, I will attach it provisionally to the score
//
//    lmScore* pScore = m_pDoc->GetScore();
//    lmTextStyle* pStyle = pScore->GetStyleInfo(_("Normal text"));
//    wxASSERT(pStyle);
//    wxString sText = _T("");
//    lmTextItem* pNewText = new lmTextItem(pScore, lmNEW_ID, sText, lmHALIGN_DEFAULT, pStyle);
//	pScore->AttachAuxObj(pNewText);
//
//    //show dialog to edit the text
//	DlgProperties dlg((DocumentWindow*)NULL);
//	pNewText->OnEditProperties(&dlg);
//	dlg.Layout();
//	dlg.ShowModal();
//
//    //get text info
//    sText = pNewText->GetText();
//    pStyle = pNewText->GetStyle();
//    lmEHAlign nAlign = pNewText->GetAlignment();
//
//	//dettach the text from the score and delete the text item
//	pScore->DetachAuxObj(pNewText);
//    delete pNewText;
//
//    //Now issue the command to attach the text to the received target object
//	wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//    if (sText != _T(""))
//	    m_pController->exec_lomse_command(LENMUS_NEW CmdAttachText(lmCMD_NORMAL, m_pDoc, sText, pStyle,
//                                        nAlign, pTarget));
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::break_beam()
{
    //Break beamed group at selected note (the one pointed by cursor)

    ImoNoteRest* pNR = dynamic_cast<ImoNoteRest*>( m_cursor->get_pointee() );
    if (pNR)
    {
        string name = to_std_string(_("Break a beam"));
        m_pController->exec_lomse_command( LENMUS_NEW CmdBreakBeam(name) );
    }
}

//---------------------------------------------------------------------------------------
void CommandGenerator::change_attribute(ImoObj* pImo, int attrb, int newValue)
{
	string name = to_std_string(_("Change numeric property"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdChangeAttribute(pImo, EImoAttribute(attrb), newValue) );
}

//---------------------------------------------------------------------------------------
void CommandGenerator::change_attribute(ImoObj* pImo, int attrb, Color newValue)
{
	string name = to_std_string(_("Change color property"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdChangeAttribute(pImo, EImoAttribute(attrb), newValue) );
}

//---------------------------------------------------------------------------------------
void CommandGenerator::change_attribute(ImoObj* pImo, int attrb, double newValue)
{
	string name = to_std_string(_("Change numeric property"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdChangeAttribute(pImo, EImoAttribute(attrb), newValue) );
}

//---------------------------------------------------------------------------------------
void CommandGenerator::change_attribute_bool(ImoObj* pImo, int attrb, bool newValue)
{
	string name = to_std_string(_("Change true/false property"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdChangeAttribute(pImo, EImoAttribute(attrb), newValue) );
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::ChangeBarline(lmBarline* pBL, lmEBarline nType, bool fVisible)
//{
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	m_pController->exec_lomse_command(LENMUS_NEW CmdChangeBarline(lmCMD_NORMAL, m_pDoc, pBL, nType, fVisible) );
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::change_dots(int dots)
{
	//change dots for current selected notes/rests

	string name = to_std_string(_("Change note dots"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdChangeDots(dots, name) ); //m_selection.filter_notes_rests(), dots, name) );
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::ChangeFiguredBass(lmFiguredBass* pFB, wxString& sFigBass)
//{
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	m_pController->exec_lomse_command(LENMUS_NEW CmdChangeFiguredBass(lmCMD_NORMAL, m_pDoc, pFB, sFigBass) );
//}

////---------------------------------------------------------------------------------------
//void CommandGenerator::ChangeMidiSettings(lmInstrument* pInstr, int nMidiChannel,
//                                       int nMidiInstr)
//{
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	m_pController->exec_lomse_command(LENMUS_NEW CmdChangeMidiSettings(lmCMD_NORMAL, m_pDoc, pInstr, nMidiChannel,
//                                            nMidiInstr) );
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::change_note_accidentals(EAccidentals acc)
{
	//change note accidentals for current selected notes

	string name = to_std_string(_("Change note accidentals"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdChangeAccidentals(acc, name));    //m_selection.filter_notes_rests(), acc, name));
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::ChangeNotePitch(int nSteps)
//{
//	//change pith of note at current cursor position
//    lmScoreCursor* pCursor = m_pDoc->GetScore()->GetCursor();
//	wxASSERT(pCursor);
//    lmStaffObj* pCursorSO = pCursor->GetStaffObj();
//	wxASSERT(pCursorSO);
//	wxASSERT(pCursorSO->IsNote());
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	string name = to_std_string(_("Change note pitch"));
//	m_pController->exec_lomse_command(LENMUS_NEW CmdChangeNotePitch(lmCMD_NORMAL, sName, m_pDoc,
//                                         (ImoNote*)pCursorSO, nSteps) );
//}
//
////---------------------------------------------------------------------------------------
//void CommandGenerator::ChangePageMargin(GmoObj* pGMO, int nIdx, int nPage, lmLUnits uPos)
//{
//	//Updates the position of a margin
//
//	wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	string name = to_std_string(_("Change margin"));
//	m_pController->exec_lomse_command(LENMUS_NEW CmdChangePageMargin(lmCMD_NORMAL, name, m_pDoc,
//                                          pGMO, nIdx, nPage, uPos));
//}

////---------------------------------------------------------------------------------------
//void CommandGenerator::ChangeText(lmScoreText* pST, wxString sText, lmEHAlign nAlign,
//                               lmLocation tPos, lmTextStyle* pStyle, int nHintOptions)
//{
//	//change properties of a lmTextItem object
//
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	string name = to_std_string(_("Change text"));
//	m_pController->exec_lomse_command(LENMUS_NEW CmdChangeText(lmCMD_NORMAL, sName, m_pDoc, pST, sText,
//                                    nAlign, tPos, pStyle, nHintOptions) );
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::delete_selection()
{
    //Deleted all objects in the selection.

    list<ImoObj*>& objects = m_selection.get_all_objects();

    //if no object, ignore command
    if (objects.size() > 0)
    {
        string name = to_std_string(_("Delete selection"));
        m_pController->exec_lomse_command( LENMUS_NEW CmdDeleteSelection(name) );   //objects, name) );
    }
}

//---------------------------------------------------------------------------------------
void CommandGenerator::delete_staffobj()
{
	//delete the StaffObj at current caret position

	//get object pointed by the cursor
    ImoStaffObj* pSO = dynamic_cast<ImoStaffObj*>( m_cursor->get_pointee() );

    //if no object, ignore command. i.e. user clicking 'Del' key on no object
    if (pSO)
    {
        string name = to_std_string(
            wxString::Format(_("Delete %s"), to_wx_string(pSO->get_name()).c_str() ));
        m_pController->exec_lomse_command( LENMUS_NEW CmdDeleteStaffObj(name) );    //pSO, name) );
    }
}

//---------------------------------------------------------------------------------------
void CommandGenerator::delete_tie()
{
    //remove tie between the selected notes

	string name = to_std_string(_("Delete tie"));
    m_pController->exec_lomse_command( LENMUS_NEW CmdDeleteRelation(k_tie, name) );
}

//---------------------------------------------------------------------------------------
void CommandGenerator::delete_tuplet()
{
    // Remove all selected tuplet

    string name = to_std_string(_("Delete tuplet"));
    m_pController->exec_lomse_command(
        LENMUS_NEW CmdDeleteRelation(k_imo_tuplet, name) );
}

//---------------------------------------------------------------------------------------
void CommandGenerator::insert_barline(int barlineType)
{
	//insert a barline at current cursor position

    stringstream src;
    src << "(barline "
        << LdpExporter::barline_type_to_ldp(barlineType)
        << ")";
    string name = to_std_string(_("Insert barline"));
    insert_staffobj(src.str(), name);
}

//---------------------------------------------------------------------------------------
void CommandGenerator::insert_clef(int clefType, int staff)
{
	//insert a Clef at current cursor position

    stringstream src;
    src << "(clef "
        << LdpExporter::clef_type_to_ldp(clefType)
        << " p"
        << staff+1
        << ")";
    string name = to_std_string(_("Insert clef"));
    insert_staffobj(src.str(), name);
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::InsertFiguredBass()
//{
//    //Create a new figured bass and add it to the VStaff
//
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	wxString sFigBass = _T("5 3");
//	m_pController->exec_lomse_command(LENMUS_NEW CmdInsertFiguredBass(lmCMD_NORMAL, m_pDoc, sFigBass) );
//}

////---------------------------------------------------------------------------------------
//void CommandGenerator::InsertFiguredBassLine()
//{
//    //Create a new figured bass line and add it to the VStaff
//
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	m_pController->exec_lomse_command(LENMUS_NEW CmdInsertFBLine(lmCMD_NORMAL, m_pDoc) );
//}

////---------------------------------------------------------------------------------------
//void CommandGenerator::InsertKeySignature(int nFifths, bool fMajor, bool fVisible)
//{
//    //insert a key signature at current cursor position
//
//    //wxLogMessage(_T("[CommandGenerator::InsertKeySignature] fifths=%d, %s"),
//    //             nFifths, (fMajor ? _T("major") : _T("minor")) );
//
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	string name = to_std_string(_("Insert key signature"));
//	m_pController->exec_lomse_command(LENMUS_NEW CmdInsertKeySignature(lmCMD_NORMAL, sName, m_pDoc, nFifths,
//                                            fMajor, fVisible) );
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::insert_staffobj(string ldpSrc, string name)
{
	//insert an staffobj at current cursor position

    m_pController->exec_lomse_command( LENMUS_NEW CmdInsertStaffObj(ldpSrc, name) );
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::InsertTimeSignature(int nBeats, int nBeatType, bool fVisible)
//{
//    //insert a time signature at current cursor position
//
//    //wxLogMessage(_T("[CommandGenerator::InsertTimeSignature] nBeats=%d, nBeatType=%d"), nBeats, nBeatType);
//
//    wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	string name = to_std_string(_("Insert time signature"));
//	m_pController->exec_lomse_command(LENMUS_NEW CmdInsertTimeSignature(lmCMD_NORMAL, sName, m_pDoc,
//                                             nBeats, nBeatType, fVisible) );
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::join_beam()
{
    //depending on current selection content, either:
    // - create a beamed group with the selected notes,
    // - join two or more beamed groups
    // - or add a note to a beamed group

	string name = to_std_string(_("Add beam"));
	m_pController->exec_lomse_command(LENMUS_NEW CmdJoinBeam(name) );
}

//---------------------------------------------------------------------------------------
void CommandGenerator::move_caret_to_click_point(SpEventMouse event)
{
    SpInteractor spIntor = m_pController->get_interactor_shared_ptr();
    DocCursorState state = spIntor->click_event_to_cursor_state(event);
    if (state.get_top_level_id() != k_no_imoid)
        m_pController->exec_lomse_command( LENMUS_NEW CmdCursor(state), k_show_busy );
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::MoveObject(GmoObj* pGMO, const UPoint& uPos)
//{
//	//Generate move command to move the lmComponentObj and update the document
//
//	wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	wxString sName = wxString::Format(_("Move %s"), pGMO->GetName().c_str() );
//	m_pController->exec_lomse_command(LENMUS_NEW CmdMoveObject(lmCMD_NORMAL, sName, m_pDoc, pGMO, uPos));
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::move_object_point(int iPoint, UPoint shift)
{
	string name = to_std_string(_("Move control point"));
    m_pController->exec_lomse_command( LENMUS_NEW CmdMoveObjectPoint(iPoint, shift),
                                       k_show_busy );
}

////---------------------------------------------------------------------------------------
//void CommandGenerator::MoveNote(GmoObj* pGMO, const UPoint& uPos, int nSteps)
//{
//	//Generate move command to move the note and change its pitch
//
//	wxCommandProcessor* pCP = m_pDoc->GetCommandProcessor();
//	m_pController->exec_lomse_command(LENMUS_NEW CmdMoveNote(lmCMD_NORMAL, m_pDoc, (ImoNote*)pGMO->GetScoreOwner(), uPos, nSteps));
//}

//---------------------------------------------------------------------------------------
void CommandGenerator::toggle_stem()
{
    //toggle stem in all selected notes.

//TODO
//	m_pController->exec_lomse_command(LENMUS_NEW CmdToggleNoteStem(lmCMD_NORMAL, m_pDoc, m_pView->GetSelection()) );
}







////-------------------------------------------------------------------------------------
//// Mouse click on a valid area while dragging a tool
////-------------------------------------------------------------------------------------

//=======================================================================================
// ClickHandler implementation
//=======================================================================================
ClickHandler::ClickHandler(DocumentWindow* pController, ToolsInfo& toolsInfo,
                       SelectionSet& selection, DocCursor* cursor)
    : m_pController(pController)
    , m_toolsInfo(toolsInfo)
    , m_selection(selection)
    , m_cursor(cursor)
    , m_fEventProcessed(false)
    , m_executer(pController, selection, cursor)
{
}


//=======================================================================================
// BarlineClickHandler implementation
//=======================================================================================
void BarlineClickHandler::process_click(SpEventMouse event)
{
//    //click only valid if on staff
//    if (m_pCurShapeStaff)
    {
        m_executer.move_caret_to_click_point(event);
        m_executer.insert_barline(m_toolsInfo.barlineType);
        m_fEventProcessed = true;
    }
}


//=======================================================================================
// ClefClickHandler implementation
//=======================================================================================
void ClefClickHandler::process_click(SpEventMouse event)
{
//    //click only valid if on staff
//    if (m_pCurShapeStaff)
    {
        m_executer.move_caret_to_click_point(event);
        ScoreCursor* pCursor = static_cast<ScoreCursor*>( m_cursor->get_inner_cursor() );
        m_executer.insert_clef(m_toolsInfo.clefType, pCursor->staff());
        m_fEventProcessed = true;
    }
}


//=======================================================================================
// NoteRestClickHandler implementation
//=======================================================================================
void NoteRestClickHandler::process_click(SpEventMouse event)
{
//    //Click on staff
//    if (m_pCurShapeStaff)
//    {
        //move cursor to insertion point and get staff number
        m_executer.move_caret_to_click_point(event);
        ScoreCursor* pCursor = static_cast<ScoreCursor*>( m_cursor->get_inner_cursor() );
        int staff = pCursor->staff() + 1;
        int voice = m_toolsInfo.voice;

//        //in 'TheoHarmonyCtrol' edit mode, force staff depending on voice
//        lmEditorMode* pEditorMode = m_pDoc->GetEditMode();
//        if (pEditorMode && pEditorMode->GetModeName() == _T("TheoHarmonyCtrol"))
//        {
//            if (voice == 1 || voice == 2)
//                staff = 1;
//            else
//                staff = 2;
//        }

        stringstream src;
        if (m_toolsInfo.fIsNote)
        {
            src << "(n ";
            if (m_toolsInfo.acc != k_no_accidentals)
                src << LdpExporter::accidentals_to_string(m_toolsInfo.acc);

            //get pitch from mouse position on staff
            DiatonicPitch dp = m_pController->get_pitch_at(event->get_x(), event->get_y());
            if (dp == DiatonicPitch(k_no_pitch))
                src << "* ";
            else
                src << dp.get_ldp_name() << " ";
        }
        else
            src << "(r ";

        src << LdpExporter::notetype_to_string(m_toolsInfo.noteType, m_toolsInfo.dots);
        src << " v" << voice;
        src << " p" << staff << ")";

        string name = to_std_string( _("Insert note") );
        m_executer.insert_staffobj(src.str(), name);
        m_fEventProcessed = true;
}




////---------------------------------------------------------------------------------------
//void CommandEventHandler::OnToolHarmonyClick(GmoObj* pGMO, UPoint uPagePos,
//                                       TimeUnits rGridTime)
//{
//    //Click on Note/rest: Add figured bass
//    if (pGMO->IsShape())
//    {
//        ToolBox* pToolBox = GetMainFrame()->GetActiveToolBox();
//	    wxASSERT(pToolBox);
//        ToolPageSymbols* pPage = (ToolPageSymbols*)pToolBox->GetSelectedPage();
//        lmEToolID nTool = pPage->GetCurrentToolID();
//        lmScoreObj* pSCO = ((lmShape*)pGMO)->GetScoreOwner();
//        switch(nTool)
//        {
//            case lmTOOL_FIGURED_BASS:
//            {
//                //Move cursor to insertion position and insert figured bass
//                wxASSERT(pSCO->IsNote() || pSCO->IsRest());
//                m_pDoc->GetScore()->GetCursor()->MoveCursorToObject((lmStaffObj*)pSCO);
//                InsertFiguredBass();
//                break;
//            }
//
//            case lmTOOL_FB_LINE:
//                //Move cursor to insertion position and insert figured bass line
//                wxASSERT(pSCO->IsNote() || pSCO->IsRest());
//                m_pDoc->GetScore()->GetCursor()->MoveCursorToObject((lmStaffObj*)pSCO);
//                InsertFiguredBassLine();
//                break;
//
//            default:
//                wxASSERT(false);
//        }
//    }
//}
//
////---------------------------------------------------------------------------------------
//void CommandEventHandler::OnToolSymbolsClick(GmoObj* pGMO, UPoint uPagePos,
//                                       TimeUnits rGridTime)
//{
//    //TODO
//    //Click on Note/rest: Add symbol
//    if (pGMO->IsShape())
//    {
//        ToolBox* pToolBox = GetMainFrame()->GetActiveToolBox();
//	    wxASSERT(pToolBox);
//        ToolPageSymbols* pPage = (ToolPageSymbols*)pToolBox->GetSelectedPage();
//        lmEToolID nTool = pPage->GetCurrentToolID();
//        lmScoreObj* pSCO = ((lmShape*)pGMO)->GetScoreOwner();
//        switch(nTool)
//        {
//            case lmTOOL_LINES:
//                lmTODO(_T("[CommandEventHandler::OnToolSymbolsClick] TODO: handle LINES tool"));
//                break;
//
//            case lmTOOL_TEXTBOX:
//                lmTODO(_T("[CommandEventHandler::OnToolSymbolsClick] TODO: handle TEXTBOX tool"));
//                break;
//
//            case lmTOOL_TEXT:
//                lmTODO(_T("[CommandEventHandler::OnToolSymbolsClick] TODO: handle TEXT tool"));
//                break;
//
//            default:
//                wxASSERT(false);
//        }
//    }
//}

////---------------------------------------------------------------------------------------
//void CommandEventHandler::OnToolTimeSignatureClick(GmoObj* pGMO, UPoint uPagePos,
//                                             TimeUnits rGridTime)
//{
//    //Click on staff
//    if (m_pCurShapeStaff)
//    {
//        //Move cursor to insertion position (start of pointed measure)
//	    int nStaff = m_pCurShapeStaff->GetNumStaff();
//        MoveCursorTo(m_pCurBSI, nStaff, 0.0f, false);    //true: move to end of time
//
//        //do insert Time Signature
//        ToolPageClefs* pPage = (ToolPageClefs*)m_pToolBox->GetSelectedPage();
//        int nBeats = pPage->GetTimeBeats();
//        int nBeatType = pPage->GetTimeBeatType();
//        InsertTimeSignature(nBeats, nBeatType);
//    }
//}
//
////---------------------------------------------------------------------------------------
//void CommandEventHandler::OnToolKeySignatureClick(GmoObj* pGMO, UPoint uPagePos,
//                                            TimeUnits rGridTime)
//{
//    //Click on staff
//    if (m_pCurShapeStaff)
//    {
//        //Move cursor to insertion position (start of pointed measure)
//	    int nStaff = m_pCurShapeStaff->GetNumStaff();
//        MoveCursorTo(m_pCurBSI, nStaff, 0.0f, false);    //true: move to end of time
//
//        //do insert Key Signature
//        ToolPageClefs* pPage = (ToolPageClefs*)m_pToolBox->GetSelectedPage();
//        bool fMajor = pPage->IsMajorKeySignature();
//        int nFifths = pPage->GetFifths();
//        InsertKeySignature(nFifths, fMajor);
//    }
//}




////-------------------------------------------------------------------------------------
//// implementation of DocumentWindow
////-------------------------------------------------------------------------------------
//
//
//
//BEGIN_EVENT_TABLE(DocumentWindow, wxEvtHandler)
//	EVT_CHAR(DocumentWindow::OnKeyPress)
//	EVT_KEY_DOWN(DocumentWindow::OnKeyDown)
//    EVT_ERASE_BACKGROUND(DocumentWindow::OnEraseBackground)
//
//	//contextual menus
//	EVT_MENU	(lmPOPUP_Cut, DocumentWindow::OnCut)
//    EVT_MENU	(lmPOPUP_Copy, DocumentWindow::OnCopy)
//    EVT_MENU	(lmPOPUP_Paste, DocumentWindow::OnPaste)
//    EVT_MENU	(lmPOPUP_Color, DocumentWindow::OnColor)
//    EVT_MENU	(lmPOPUP_Properties, DocumentWindow::OnProperties)
//    EVT_MENU	(lmPOPUP_DeleteTiePrev, DocumentWindow::OnDeleteTiePrev)
//    EVT_MENU	(lmPOPUP_AttachText, DocumentWindow::OnAttachText)
//    EVT_MENU	(lmPOPUP_Score_Titles, DocumentWindow::OnScoreTitles)
//    EVT_MENU	(lmPOPUP_View_Page_Margins, DocumentWindow::OnViewPageMargins)
//    EVT_MENU	(lmPOPUP_ToggleStem, DocumentWindow::OnToggleStem)
//#ifdef _LM_DEBUG_
//    EVT_MENU	(lmPOPUP_DumpShape, DocumentWindow::OnDumpShape)
//#endif
//    EVT_MENU	(lmTOOL_VOICE_SOPRANO, DocumentWindow::OnToolPopUpMenuEvent)
//	EVT_MENU	(lmTOOL_VOICE_ALTO, DocumentWindow::OnToolPopUpMenuEvent)
//	EVT_MENU	(lmTOOL_VOICE_TENOR, DocumentWindow::OnToolPopUpMenuEvent)
//	EVT_MENU	(lmTOOL_VOICE_BASS, DocumentWindow::OnToolPopUpMenuEvent)
//
//
//END_EVENT_TABLE()

////----------------------------------------------------------------------------
//// Helper class to display popup window with information about dragged tool
////----------------------------------------------------------------------------
//class lmInfoWindow : public wxPopupTransientWindow
//{
//public:
//    lmInfoWindow( wxWindow *parent );
//    virtual ~lmInfoWindow();
//
//    wxScrolledWindow* GetChild() { return m_panel; }
//
//private:
//    wxScrolledWindow *m_panel;
//
//    DECLARE_CLASS(lmInfoWindow)
//};
//
////----------------------------------------------------------------------------
//// lmInfoWindow
////----------------------------------------------------------------------------
//IMPLEMENT_CLASS(lmInfoWindow,wxPopupTransientWindow)
//
//lmInfoWindow::lmInfoWindow( wxWindow *parent )
//    : wxPopupTransientWindow( parent )
//{
//    m_panel = new wxScrolledWindow( this, wxID_ANY );
//    m_panel->SetBackgroundColour( wxColour(255,255,170) );    //pale yellow
//
//    wxStaticText *text = new wxStaticText( m_panel, wxID_ANY,
//                          _T("Hola. Nota C4") );
//
//
//    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );
//    topSizer->Add( text, 0, wxALL, 5 );
//
//    m_panel->SetAutoLayout( true );
//    m_panel->SetSizer( topSizer );
//    topSizer->Fit(m_panel);
//    topSizer->Fit(this);
//}
//
//lmInfoWindow::~lmInfoWindow()
//{
//}
//
//
//
//
////-------------------------------------------------------------------------------------
//// implementation of CommandEventHandler
////-------------------------------------------------------------------------------------
//
//IMPLEMENT_CLASS(CommandEventHandler, DocumentWindow)
//
//// keys pressed when a mouse event
//enum {
//    lmKEY_NONE = 0x0000,
//    lmKEY_ALT = 0x0001,
//    lmKEY_CTRL = 0x0002,
//    lmKEY_SHIFT = 0x0004,
//};
//
//// Dragging states
//enum
//{
//	lmDRAG_NONE = 0,
//	lmDRAG_START_LEFT,
//	lmDRAG_CONTINUE_LEFT,
//	lmDRAG_START_RIGHT,
//	lmDRAG_CONTINUE_RIGHT,
//};
//
//
//
//void CommandEventHandler::CaptureTheMouse()
//{
//    wxLogMessage(_T("[CommandEventHandler::CaptureTheMouse] HasCapture=%s"),
//                 (HasCapture() ? _T("yes") : _T("no")) );
//    if (!HasCapture())
//        CaptureMouse();
//}
//
//void CommandEventHandler::ReleaseTheMouse()
//{
//    wxLogMessage(_T("[CommandEventHandler::ReleaseTheMouse] HasCapture=%s"),
//                 (HasCapture() ? _T("yes") : _T("no")) );
//    if (HasCapture())
//        ReleaseMouse();
//}
//
////#ifdef _LM_WINDOWS_
//void CommandEventHandler::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
//{
//    //Any application which captures the mouse in the beginning of some operation
//    //must handle wxMouseCaptureLostEvent and cancel this operation when it receives
//    //the event. The event handler must not recapture mouse.
//    wxLogMessage(_T("[CommandEventHandler::OnMouseCaptureLost] HasCapture=%s"),
//                 (HasCapture() ? _T("yes") : _T("no")) );
//    //m_pView->OnImageEndDrag();>OnObjectEndDragLeft(event, pDC, vCanvasPos, vCanvasOffset,
//    //                             uPagePos, nKeys);
//    //SetDraggingObject(false);
//    //m_nDragState = lmDRAG_NONE;
//    //SetDraggingObject(false);
//}
////#endif
//
//void CommandEventHandler::OnPaint(wxPaintEvent &WXUNUSED(event))
//{
//    // In a paint event handler, the application must always create a wxPaintDC object,
//    // even if it is not used. Otherwise, under MS Windows, refreshing for this and
//    // other windows will go wrong.
//    wxPaintDC dc(this);
//    if (!m_pView) return;
//
//    // get the updated rectangles list
//    wxRegionIterator upd(GetUpdateRegion());
//
//    // iterate to redraw each damaged rectangle
//    // The rectangles are in pixels, referred to the client area, and are unscrolled
//    m_pView->PrepareForRepaint(&dc);
//    while (upd)
//    {
//        wxRect rect = upd.GetRect();
//        m_pView->RepaintScoreRectangle(&dc, rect);
//        upd++;
//    }
//    m_pView->TerminateRepaint(&dc);
//}
//
//void CommandEventHandler::OnMouseEvent(wxMouseEvent& event)
//{
//    //handle mouse event
//
//    //if no view, nothimg to do
//    if (!m_pView)
//        return;
//
//    wxClientDC dc(this);
//
//        //First, for better performance, filter out non-used events
//
//    //filter out non-handled events
//    wxEventType nEventType = event.GetEventType();
//    if (nEventType==wxEVT_MIDDLE_DOWN || nEventType==wxEVT_MIDDLE_UP ||
//        nEventType==wxEVT_MIDDLE_DCLICK)
//    {
//        return;
//    }
//
//
//        //Now deal with events that do not require to compute mouse position and/or which graphical
//        //object is under the mouse
//
//
//    // check for mouse entering/leaving the window events
//	if (event.Entering())    //wxEVT_ENTER_WINDOW
//	{
//		//the mouse is entering the window. Change mouse icon as appropriate
//		//TODO
//		return;
//	}
//	if (event.Leaving())    //wxEVT_LEAVE_WINDOW
//	{
//		//the mouse is leaving the window. Change mouse icon as appropriate
//		//TODO
//		return;
//	}
//
//    //deal with mouse wheel events
//	if (nEventType == wxEVT_MOUSEWHEEL)
//    {
//        m_pView->OnMouseWheel(event);
//		return;
//    }
//
//
//        //From this point we need information about mouse position. Let's compute it an
//        //update GUI (rules markers, status bar, etc.). Get also information about any possible
//        //key being pressed whil mouse is moving, and about dragging
//
//
//	// get mouse point (pixels, referred to CommandEventHandler origin)
//    m_vMouseCanvasPos = event.GetPosition();
//
//    // Set DC in logical units and scaled, so that
//    // transformations logical/device and viceversa can be computed
//    m_pView->ScaleDC(&dc);
//
//    //compute mouse point in logical units. Get also different origins and values
//    bool fInInterpageGap;           //mouse click out of page
//	m_pView->DeviceToLogical(m_vMouseCanvasPos, m_uMousePagePos, &m_vMousePagePos,
//                             &m_vPageOrg, &m_vCanvasOffset, &m_nNumPage,
//                             &fInInterpageGap);
//
//	#ifdef _LM_DEBUG_
//	bool fDebugMode = g_pLogger->IsAllowedTraceMask(_T("OnMouseEvent"));
//	#endif
//
//    //update mouse num page
//    m_pView->UpdateNumPage(m_nNumPage);
//
//	////for testing and debugging methods DeviceToLogical [ok] and LogicalToDevice [ok]
//	//lmDPoint tempPagePosD;
//	//LogicalToDevice(tempPagePosL, tempPagePosD);
//
//    // draw markers on the rulers
//    m_pView->UpdateRulerMarkers(m_vMousePagePos);
//
//    // check if dragging (moving with a button pressed), and filter out mouse movements small
//    //than tolerance
//	bool fDragging = event.Dragging();
//	if (fDragging && m_fCheckTolerance)
//	{
//		// Check if we're within the tolerance for mouse movements.
//		// If we're very close to the position we started dragging
//		// from, this may not be an intentional drag at all.
//		lmLUnits uAx = abs(dc.DeviceToLogicalXRel((long)(m_vMouseCanvasPos.x - m_vStartDrag.x)));
//		lmLUnits uAy = abs(dc.DeviceToLogicalYRel((long)(m_vMouseCanvasPos.y - m_vStartDrag.y)));
//        lmLUnits uTolerance = m_pView->GetMouseTolerance();
//		if (uAx <= uTolerance && uAy <= uTolerance)
//			return;
//		else
//            //I will not allow for a second involuntary small movement. Therefore
//			//if we have ignored a drag, smaller than tolerance, then do not check for
//            //tolerance the next time in this drag.
//			m_fCheckTolerance = false;
//	}
//
//    //At this point it has been determined all basic mouse position information. Now we
//    //start dealing with mouse moving, mouse clicks and dragging events. Behaviour from
//    //this point is different, depending on mouse mode (pointer, data entry, eraser, etc.).
//    //Therefore, processing is splitted at this point
//
//    if (mouseMode == k_mouse_mode_data_entry)
//        OnMouseEventToolMode(event, &dc);
//    else
//        OnMouseEventSelectMode(event, &dc);
//
//}
//
//void CommandEventHandler::OnMouseEventSelectMode(wxMouseEvent& event, wxDC* pDC)
//{
//    //If we reach this point is because it is a mouse dragging or a mouse click event.
//    //Let's deal with them.
//
//	#ifdef _LM_DEBUG_
//	bool fDebugMode = g_pLogger->IsAllowedTraceMask(_T("OnMouseEvent"));
//	#endif
//
//	bool fDragging = event.Dragging();
//
//
//    //determine type of area pointed by mouse. Also collect information
//    //about current staff for point pointed by mouse and about related BoxSliceInstr.
//    GetPointedAreaInfo();
//
//    //check moving events
//    if (event.GetEventType() == wxEVT_MOTION && !fDragging)
//    {
//	    if (m_pCurGMO)
//        {
//            //Mouse is currently pointing to an object (shape or box)
//            if (m_pMouseOverGMO)
//            {
//                //mouse was previously over an object. If it is the same than current one there is
//                //nothing to do
//                if (m_pMouseOverGMO == m_pCurGMO)
//                    return;     //nothing to do. Mouse continues over object
//
//                //It is a new object. Inform previous object that it is left
//                m_pMouseOverGMO->OnMouseOut(this, m_uMousePagePos);
//            }
//            m_pMouseOverGMO = m_pCurGMO;
//            m_pCurGMO->OnMouseIn(this, m_uMousePagePos);
//        }
//
//        //mouse is not pointing neither to a shape nor to a box. If mouse was poining to an object
//        //inform it that it has been left
//        else
//        {
//            if (m_pMouseOverGMO)
//            {
//                //mouse was previously over an object. Inform it that it is left
//                m_pMouseOverGMO->OnMouseOut(this, m_uMousePagePos);
//                m_pMouseOverGMO = (GmoObj*)NULL;
//            }
//        }
//        return;
//    }
//
//    // check if a key is pressed
//    int nKeysPressed = lmKEY_NONE;
//    if (event.ShiftDown())
//        nKeysPressed |= lmKEY_SHIFT;
//    if (event.CmdDown())
//        nKeysPressed |= lmKEY_CTRL;
//    if (event.AltDown())
//        nKeysPressed |= lmKEY_ALT;
//
//	if (!fDragging)
//	{
//		// Non-dragging events.
//        // In MS Windows the 'end of drag' event is a non-dragging event
//
//		m_fCheckTolerance = true;
//
//		#ifdef _LM_DEBUG_
//		if(fDebugMode) g_pLogger->LogDebug(_T("Non-dragging event"));
//		#endif
//
//		if (event.IsButton())
//		{
//			#ifdef _LM_DEBUG_
//			if(fDebugMode) g_pLogger->LogDebug(_T("button event"));
//			#endif
//
//			//find the object pointed with the mouse
//			GmoObj* m_pCurGMO = m_pView->FindShapeAt(m_nNumPage, m_uMousePagePos, false);
//			if (m_pCurGMO) // Object event
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("button on object event"));
//				#endif
//
//				if (event.LeftDown())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: event.LeftDown()"));
//					#endif
//
//					//Save data for a possible start of dragging
//					m_pDraggedGMO = m_pCurGMO;
//					m_nDragState = lmDRAG_START_LEFT;
//					m_vStartDrag.x = m_vMouseCanvasPos.x;
//					m_vStartDrag.y = m_vMouseCanvasPos.y;
//                    m_uStartDrag = m_uMousePagePos;
//
//					m_uDragStartPos = m_uMousePagePos;	// save mouse position (page logical coordinates)
//					// compute the location of the drag position relative to the upper-left
//					// corner of the image (pixels)
//					m_uHotSpotShift = m_uMousePagePos - m_pDraggedGMO->GetObjectOrigin();
//					m_vDragHotSpot.x = pDC->LogicalToDeviceXRel((int)m_uHotSpotShift.x);
//					m_vDragHotSpot.y = pDC->LogicalToDeviceYRel((int)m_uHotSpotShift.y);
//				}
//				else if (event.LeftUp())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: event.LeftUp()"));
//					#endif
//
//			        if (m_nDragState == lmDRAG_CONTINUE_LEFT)
//			        {
//                        if (m_fDraggingObject)
//                        {
//                            //draggin. Finish left object dragging
//				            OnObjectContinueDragLeft(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				            OnObjectEndDragLeft(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//                        }
//                        else
//                        {
//                            //draggin. Finish left canvas dragging
//				            OnCanvasContinueDragLeft(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				            OnCanvasEndDragLeft(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                       }
//			        }
//                    else
//                    {
//					    //click on object. Only send a click event if the same object
//                        //was involved in 'down' and 'up' events
//					    if (m_pCurGMO == m_pDraggedGMO)
//						    OnLeftClickOnObject(m_pCurGMO, m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                    }
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_NONE;
//                    m_fCheckTolerance = true;
//				}
//				else if (event.LeftDClick())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: event.LeftDClick()"));
//					#endif
//
//					OnLeftDoubleClickOnObject(m_pCurGMO, m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_NONE;
//				}
//				else if (event.RightDown())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: event.RightDown()"));
//					#endif
//
//					//Save data for a possible start of dragging
//					m_pDraggedGMO = m_pCurGMO;
//					m_nDragState = lmDRAG_START_RIGHT;
//					m_vStartDrag.x = m_vMouseCanvasPos.x;
//					m_vStartDrag.y = m_vMouseCanvasPos.y;
//                    m_uStartDrag = m_uMousePagePos;
//
//					m_uDragStartPos = m_uMousePagePos;	// save mouse position (page logical coordinates)
//					// compute the location of the drag position relative to the upper-left
//					// corner of the image (pixels)
//					m_uHotSpotShift = m_uMousePagePos - m_pDraggedGMO->GetObjectOrigin();
//					m_vDragHotSpot.x = pDC->LogicalToDeviceXRel((int)m_uHotSpotShift.x);
//					m_vDragHotSpot.y = pDC->LogicalToDeviceYRel((int)m_uHotSpotShift.y);
//				}
//				else if (event.RightUp())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: event.RightUp()"));
//					#endif
//
//			        if (m_nDragState == lmDRAG_CONTINUE_RIGHT)
//			        {
//                        if (m_fDraggingObject)
//                        {
//                            //draggin. Finish right object dragging
//				            OnObjectContinueDragRight(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				            OnObjectEndDragRight(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//                        }
//                        else
//                        {
//                            //draggin. Finish right canvas dragging
//				            OnCanvasContinueDragRight(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				            OnCanvasEndDragRight(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                       }
//			        }
//                    else
//                    {
//					    //click on object. Only send a click event if the same object
//                        //was involved in 'down' and 'up' events
//					    if (m_pCurGMO == m_pDraggedGMO)
//						    OnRightClickOnObject(m_pCurGMO, m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                    }
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_NONE;
//                    m_fCheckTolerance = true;
//				}
//				else if (event.RightDClick())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: event.RightDClick()"));
//					#endif
//
//					OnRightDoubleClickOnObject(m_pCurGMO, m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_NONE;
//				}
//				else
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on object: no identified event"));
//					#endif
//				}
//
//			}
//			else // Canvas event (no pointed object)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas event"));
//				#endif
//
//				if (event.LeftDown())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: event.LeftDown()"));
//					#endif
//
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_START_LEFT;
//					m_vStartDrag.x = m_vMouseCanvasPos.x;
//					m_vStartDrag.y = m_vMouseCanvasPos.y;
//                    m_uStartDrag = m_uMousePagePos;
//				}
//				else if (event.LeftUp())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: event.LeftUp()"));
//					#endif
//
//			        if (m_nDragState == lmDRAG_CONTINUE_LEFT)
//			        {
//                        if (m_pDraggedGMO)
//                        {
//							#ifdef _LM_DEBUG_
//							if(fDebugMode) g_pLogger->LogDebug(_T("dragging object: Finish left dragging"));
//							#endif
//
//	                            //draggin. Finish left dragging
//				            OnObjectContinueDragLeft(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				            OnObjectEndDragLeft(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//                        }
//                        else
//                        {
//							#ifdef _LM_DEBUG_
//							if(fDebugMode) g_pLogger->LogDebug(_T("dragging on canvas: Finish left dragging"));
//							#endif
//
//                            //draggin. Finish left dragging
//				            OnCanvasContinueDragLeft(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				            OnCanvasEndDragLeft(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                        }
//			        }
//                    else
//                    {
//						#ifdef _LM_DEBUG_
//						if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: non-dragging. Left click on object"));
//						#endif
//
//                        //non-dragging. Left click on object
//					    OnLeftClickOnCanvas(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                    }
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_NONE;
//                    m_fCheckTolerance = true;
//				}
//				else if (event.RightDown())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: event.RightDown()"));
//					#endif
//
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_START_RIGHT;
//					m_vStartDrag.x = m_vMouseCanvasPos.x;
//					m_vStartDrag.y = m_vMouseCanvasPos.y;
//                    m_uStartDrag = m_uMousePagePos;
//				}
//				else if (event.RightUp())
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: event.RightUp()"));
//					#endif
//
//			        if (m_nDragState == lmDRAG_CONTINUE_RIGHT)
//			        {
//                        if (m_pDraggedGMO)
//                        {
//							#ifdef _LM_DEBUG_
//							if(fDebugMode) g_pLogger->LogDebug(_T("dragging object: Finish right dragging"));
//							#endif
//
//	                            //draggin. Finish right dragging
//				            OnObjectContinueDragRight(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				            OnObjectEndDragRight(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//                        }
//                        else
//                        {
//							#ifdef _LM_DEBUG_
//							if(fDebugMode) g_pLogger->LogDebug(_T("dragging on canvas: Finish right dragging"));
//							#endif
//
//                            //draggin. Finish right dragging
//				            OnCanvasContinueDragRight(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				            OnCanvasEndDragRight(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                        }
//			        }
//                    else
//                    {
//						#ifdef _LM_DEBUG_
//						if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: non-dragging. Right click on object"));
//						#endif
//
//                        //non-dragging. Right click on object
//					    OnRightClickOnCanvas(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//                    }
//					m_pDraggedGMO = (GmoObj*)NULL;
//					m_nDragState = lmDRAG_NONE;
//                    m_fCheckTolerance = true;
//				}
//                else
//				{
//					#ifdef _LM_DEBUG_
//					if(fDebugMode) g_pLogger->LogDebug(_T("button on canvas: no identified event"));
//					#endif
//				}
//			}
//		}
//        else
//		{
//			#ifdef _LM_DEBUG_
//			if(fDebugMode) g_pLogger->LogDebug(_T("non-dragging: no button event"));
//			#endif
//		}
//	}
//
//	else	//dragging events
//	{
//		#ifdef _LM_DEBUG_
//		if(fDebugMode) g_pLogger->LogDebug(_T("dragging event"));
//		#endif
//
//		if (m_pDraggedGMO)
//		{
//			#ifdef _LM_DEBUG_
//			if(fDebugMode) g_pLogger->LogDebug(_T("draggin an object"));
//			#endif
//
//			//draggin an object
//			if (event.LeftUp() && m_nDragState == lmDRAG_CONTINUE_LEFT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: event.LeftUp() && m_nDragState == lmDRAG_CONTINUE_LEFT"));
//				#endif
//
//				m_nDragState = lmDRAG_NONE;
//				m_fCheckTolerance = true;
//				OnObjectContinueDragLeft(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				OnObjectEndDragLeft(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				m_pDraggedGMO = (GmoObj*)NULL;
//			}
//			else if (event.RightUp() && m_nDragState == lmDRAG_CONTINUE_RIGHT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: event.RightUp() && m_nDragState == lmDRAG_CONTINUE_RIGHT"));
//				#endif
//
//				m_nDragState = lmDRAG_NONE;
//				m_fCheckTolerance = true;
//				OnObjectContinueDragRight(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				OnObjectEndDragRight(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				m_pDraggedGMO = (GmoObj*)NULL;
//			}
//			else if (m_nDragState == lmDRAG_START_LEFT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: m_nDragState == lmDRAG_START_LEFT"));
//				#endif
//
//				m_nDragState = lmDRAG_CONTINUE_LEFT;
//
//				if (m_pDraggedGMO->IsLeftDraggable())
//                {
//					OnObjectBeginDragLeft(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//                }
//				else
//				{
//					//the object is not draggable: transfer message to canvas
//				    #ifdef _LM_DEBUG_
//				    if(fDebugMode) g_pLogger->LogDebug(_T("object is not left draggable. Drag cancelled"));
//				    #endif
//					m_pDraggedGMO = (GmoObj*)NULL;
//					OnCanvasBeginDragLeft(m_vStartDrag, m_uMousePagePos, nKeysPressed);
//				}
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//			else if (m_nDragState == lmDRAG_CONTINUE_LEFT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: m_nDragState == lmDRAG_CONTINUE_LEFT"));
//				#endif
//
//				// Continue dragging
//				OnObjectContinueDragLeft(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				OnObjectContinueDragLeft(event, pDC, true, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//			else if (m_nDragState == lmDRAG_START_RIGHT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: m_nDragState == lmDRAG_START_RIGHT"));
//				#endif
//
//				m_nDragState = lmDRAG_CONTINUE_RIGHT;
//
//				if (m_pDraggedGMO->IsRightDraggable())
//				{
//					OnObjectBeginDragRight(event, pDC, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//                }
//				else
//				{
//					//the object is not draggable: transfer message to canvas
//					m_pDraggedGMO = (GmoObj*)NULL;
//					OnCanvasBeginDragRight(m_vStartDrag, m_uMousePagePos, nKeysPressed);
//				}
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//			else if (m_nDragState == lmDRAG_CONTINUE_RIGHT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: m_nDragState == lmDRAG_CONTINUE_RIGHT"));
//				#endif
//
//				// Continue dragging
//				OnObjectContinueDragRight(event, pDC, false, m_vEndDrag, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				OnObjectContinueDragRight(event, pDC, true, m_vMouseCanvasPos, m_vCanvasOffset, m_uMousePagePos, nKeysPressed);
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//            else
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("object: no identified event"));
//				#endif
//			}
//		}
//
//		else	// dragging but no object: events sent to canvas
//		{
//			#ifdef _LM_DEBUG_
//			if(fDebugMode) g_pLogger->LogDebug(_T("dragging but no object: canvas"));
//			#endif
//
//			if (event.LeftUp() && m_nDragState == lmDRAG_CONTINUE_LEFT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: event.LeftUp() && m_nDragState == lmDRAG_CONTINUE_LEFT"));
//				#endif
//
//				m_nDragState = lmDRAG_NONE;
//				m_fCheckTolerance = true;
//
//				OnCanvasContinueDragLeft(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				OnCanvasEndDragLeft(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//				m_pDraggedGMO = (GmoObj*)NULL;
//			}
//			else if (event.RightUp() && m_nDragState == lmDRAG_CONTINUE_RIGHT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: event.RightUp() && m_nDragState == lmDRAG_CONTINUE_RIGHT"));
//				#endif
//
//				m_nDragState = lmDRAG_NONE;
//				m_fCheckTolerance = true;
//
//				OnCanvasContinueDragRight(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				OnCanvasEndDragRight(m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//				m_pDraggedGMO = (GmoObj*)NULL;
//			}
//			else if (m_nDragState == lmDRAG_START_LEFT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: m_nDragState == lmDRAG_START_LEFT"));
//				#endif
//
//				m_nDragState = lmDRAG_CONTINUE_LEFT;
//				OnCanvasBeginDragLeft(m_vStartDrag, m_uMousePagePos, nKeysPressed);
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//			else if (m_nDragState == lmDRAG_CONTINUE_LEFT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: m_nDragState == lmDRAG_CONTINUE_LEFT"));
//				#endif
//
//				// Continue dragging
//				OnCanvasContinueDragLeft(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				OnCanvasContinueDragLeft(true, m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//			else if (m_nDragState == lmDRAG_START_RIGHT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: m_nDragState == lmDRAG_START_RIGHT"));
//				#endif
//
//				m_nDragState = lmDRAG_CONTINUE_RIGHT;
//				OnCanvasBeginDragRight(m_vStartDrag, m_uMousePagePos, nKeysPressed);
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//			else if (m_nDragState == lmDRAG_CONTINUE_RIGHT)
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: m_nDragState == lmDRAG_CONTINUE_RIGHT"));
//				#endif
//
//				// Continue dragging
//				OnCanvasContinueDragRight(false, m_vEndDrag, m_uMousePagePos, nKeysPressed);
//				OnCanvasContinueDragRight(true, m_vMouseCanvasPos, m_uMousePagePos, nKeysPressed);
//				m_vEndDrag = m_vMouseCanvasPos;
//			}
//            else
//			{
//				#ifdef _LM_DEBUG_
//				if(fDebugMode) g_pLogger->LogDebug(_T("canvas: no identified event"));
//				#endif
//			}
//		}
//	}
//
//}

////---------------------------------------------------------------------------------------
//void CommandEventHandler::OnMouseEventToolMode(wxMouseEvent& event, wxDC* pDC)
//{
//    //this method deals with mouse clicks when mouse is in data entry mode
//
//    //At this point mouse position and page information has been already
//    //computed
//
//    //determine type of area pointed by mouse. Also collect information
//    //about current staff for point pointed by mouse and about related BoxSliceInstr.
//    long nOldMousePointedArea = m_nMousePointedArea;
//    GetPointedAreaInfo();
//
//    //if harmony exercise, allow notes data entry only valid staff for current voice
//    lmEditorMode* pEditorMode = m_pDoc->GetEditMode();
//    if (pEditorMode && pEditorMode->GetModeName() == _T("TheoHarmonyCtrol")
//        && (m_nMousePointedArea == lmMOUSE_OnStaff
//            || m_nMousePointedArea == lmMOUSE_OnBelowStaff
//            || m_nMousePointedArea == lmMOUSE_OnAboveStaff) )
//    {
//        int nStaff = m_pCurShapeStaff->GetNumStaff();
//        if ( ((m_nSelVoice == 1 || m_nSelVoice == 2) && (nStaff != 1))
//             || ((m_nSelVoice == 3 || m_nSelVoice == 4) && (nStaff != 2)) )
//            m_nMousePointedArea = lmMOUSE_OnOther;
//    }
//
//    long nNowOnValidArea = m_nMousePointedArea & m_nValidAreas;
//
//    //Now we start dealing with mouse moving and mouse clicks. Dragging (moving the mouse
//    //with a mouse button clicked) is a meaningless operation and will be treated as
//    //moving. Therefore, only two type of events will be considered: mouse click and
//    //mouse move. Let's start with mouse click events.
//
//	if (event.IsButton() && nNowOnValidArea != 0L)
//	{
//        //mouse click: terminate any possible tool drag operation, process the click, and
//        //restart the tool drag operation
//
//        //first, terminate any possible tool drag operation
//        if (m_fDraggingTool)
//            TerminateToolDrag(pDC);
//
//////        THIS IS NOW IMPLEMENTED USING ClickHandler class
//////        //now process the click. To avoid double data entry (first, on button down and then on
//////        //button up) only button up will trigger the processing
//////        if (event.ButtonUp(wxMOUSE_BTN_LEFT))
//////        {
//////            OnToolClick(m_pCurGMO, m_uMousePagePos, m_rCurGridTime);
//////            //AWARE: after processing the click the graphical model could have been chaged.
//////            //Therefore all pointers to GMObjects are no longer valid!!!
//////        }
//////        else if (event.ButtonUp(wxMOUSE_BTN_RIGHT))
//////        {
//////            //show too contextual menu
//////            wxMenu* pMenu = GetContextualMenuForTool();
//////            if (pMenu)
//////	            PopupMenu(pMenu);
//////        }
//
//        //finally, set up information to restart the tool drag operation when moving again
//        //the mouse
//        m_nMousePointedArea = 0;
//        m_pLastShapeStaff = m_pCurShapeStaff;
//        m_pLastBSI = m_pCurBSI;
//
//        return;
//    }
//
//
//    //process mouse moving events
//
//    if (event.GetEventType() != wxEVT_MOTION)
//        return;
//
//
//
//    //check if pointed area has changed from valid to invalid or vice versa
//
//    long nBeforeOnValidArea = nOldMousePointedArea & m_nValidAreas;
//    if (nBeforeOnValidArea != 0 && nNowOnValidArea == 0 || nBeforeOnValidArea == 0 && nNowOnValidArea != 0)
//    {
//        //change from valida area to invalid, or vice versa, change drag status
//        if (nNowOnValidArea)
//        {
//            //entering on staff influence area. Start dragging tool
//            StartToolDrag(pDC);
//        }
//        else
//        {
//            //exiting staff influence area
//            TerminateToolDrag(pDC);
//        }
//    }
//    else
//    {
//        //no change valid<->invalida area. If we continue in a valid area drag marks
//        //lines if necessary
//        if (nNowOnValidArea)
//            ContinueToolDrag(event, pDC);
//    }
//
//    //update saved information
//    m_pLastShapeStaff = m_pCurShapeStaff;
//    m_pLastBSI = m_pCurBSI;
//
//    //determine needed icon and change mouse icon if necessary
//    wxCursor* pNeeded = m_pCursorElse;
//    if (nNowOnValidArea)
//        pNeeded = m_pCursorOnValidArea;
//
//    if (m_pCursorCurrent != pNeeded)
//    {
//        //change cursor
//        m_pCursorCurrent = pNeeded;
//        const wxCursor& oCursor = *pNeeded;
//        SetCursor( oCursor );
//    }
//
//    //update status bar info
//    UpdateToolInfoString();
//}

//void CommandEventHandler::GetPointedAreaInfo()
//{
//    //determine type of area pointed by mouse and classify it. Also collect information
//    //about current staff for point pointed by mouse and about related BoxSliceInstr.
//    //Returns the pointed box/shape
//    //Save found data in member variables:
//    //          m_pCurShapeStaff
//    //          m_pCurBSI
//    //          m_pCurGMO
//    //          m_nMousePointedArea
//    //          m_rCurGridTime
//
//    m_pCurShapeStaff = (lmShapeStaff*)NULL;
//    m_pCurBSI = (lmBoxSliceInstr*)NULL;
//
//    //check if pointing to a shape (not to a box)
//	m_pCurGMO = m_pView->FindShapeAt(m_nNumPage, m_uMousePagePos, true);
//    if (m_pCurGMO)
//    {
//        //pointing to a selectable shape. Get pointed area type
//        if (m_pCurGMO->IsShapeStaff())
//        {
//            m_nMousePointedArea = lmMOUSE_OnStaff;
//            m_pCurShapeStaff = (lmShapeStaff*)m_pCurGMO;
//        }
//        else if (m_pCurGMO->IsShapeNote() || m_pCurGMO->IsShapeRest())
//        {
//            m_nMousePointedArea = lmMOUSE_OnNotesRests;
//        }
//        else
//            m_nMousePointedArea = lmMOUSE_OnOtherShape;
//
//        //get the SliceInstr.
//        GmoObj* pBox = m_pView->FindBoxAt(m_nNumPage, m_uMousePagePos);
//        //AWARE: Returned box is the smallest one containig to mouse point. If point is
//        //only in lmBoxPage NULL is returned.
//        if (pBox)
//        {
//            if (pBox->IsBoxSliceInstr())
//                m_pCurBSI = (lmBoxSliceInstr*)pBox;
//            else if (pBox->IsBoxSystem())
//            {
//                //empty score. No BoxSliceInstr.
//                //for now assume it is pointing to first staff.
//                //TODO: check if mouse point is over a shape staff and select it
//                //m_pCurShapeStaff = ((lmBoxSystem*)m_pCurGMO)->GetStaffShape(1);
//            }
//            else
//            {
//                wxLogMessage(_T("[CommandEventHandler::GetPointedAreaInfo] Unknown case '%s'"),
//                            m_pCurGMO->GetName().c_str());
//                wxASSERT(false);    //Unknown case. Is it possible??????
//            }
//        }
//
//        //get the ShapeStaff
//        if (m_pCurBSI && !m_pCurShapeStaff)
//        {
//            if (m_pCurGMO->GetScoreOwner()->IsStaffObj())
//            {
//                int nStaff = ((lmStaffObj*)m_pCurGMO->GetScoreOwner())->GetStaffNum();
//                m_pCurShapeStaff = m_pCurBSI->GetStaffShape(nStaff);
//            }
//            else if (m_pCurGMO->GetScoreOwner()->IsAuxObj())
//            {
//                lmStaff* pStaff = ((lmAuxObj*)m_pCurGMO->GetScoreOwner())->GetStaff();
//                if (pStaff)
//                    m_pCurShapeStaff = m_pCurBSI->GetStaffShape( pStaff->GetNumberOfStaff() );
//            }
//        }
//    }
//
//    //not pointing to a shape
//    else
//    {
//        //check if pointing to a box
//        m_pCurGMO = m_pView->FindBoxAt(m_nNumPage, m_uMousePagePos);
//        if (m_pCurGMO)
//        {
//            if (m_pCurGMO->IsBoxSliceInstr())
//            {
//                m_pCurBSI = (lmBoxSliceInstr*)m_pCurGMO;
//                //determine staff
//                if (m_pLastBSI != m_pCurBSI)
//                {
//                    //first time on this BoxInstrSlice, between two staves
//                    m_pCurShapeStaff = m_pCurBSI->GetNearestStaff(m_uMousePagePos);
//                }
//                else
//                {
//                    //continue in this BoxInstrSlice, in same inter-staves area
//                    m_pCurShapeStaff = m_pLastShapeStaff;
//                }
//                //determine position (above/below) relative to staff
//                if (m_uMousePagePos.y > m_pCurShapeStaff->GetBounds().GetLeftBottom().y)
//                    m_nMousePointedArea = lmMOUSE_OnBelowStaff;
//                else
//                    m_nMousePointedArea = lmMOUSE_OnAboveStaff;
//            }
//            else
//                m_nMousePointedArea = lmMOUSE_OnOtherBox;
//        }
//        else
//            m_nMousePointedArea = lmMOUSE_OnOther;
//    }
//
//    //determine timepos at mouse point, by using time grid
//    if (m_pCurBSI)
//    {
//        lmBoxSlice* pBSlice = (lmBoxSlice*)m_pCurBSI->GetParentBox();
//        m_rCurGridTime = pBSlice->GetGridTimeForPosition(m_uMousePagePos.x);
//        GetMainFrame()->SetStatusBarMouseData(m_nNumPage, m_rCurGridTime,
//                                              pBSlice->GetNumMeasure(),
//                                              m_uMousePagePos);
//    }
//    ////DBG --------------------------------------
//    //wxString sSO = (m_pCurGMO ? m_pCurGMO->GetName() : _T("No object"));
//    //wxLogMessage(_T("[CommandEventHandler::GetPointedAreaInfo] LastBSI=0x%x, CurBSI=0x%x, LastStaff=0x%x, CurStaff=0x%x, Area=%d, Object=%s"),
//    //             m_pLastBSI, m_pCurBSI, m_pLastShapeStaff, m_pCurShapeStaff,
//    //             m_nMousePointedArea, sSO.c_str() );
//    ////END DBG ----------------------------------
//
//}
//
//wxMenu* CommandEventHandler::GetContextualMenuForTool()
//{
//	ToolBox* pToolBox = GetMainFrame()->GetActiveToolBox();
//	if (!pToolBox)
//        return (wxMenu*)NULL;
//
//	return pToolBox->GetContextualMenuForSelectedPage();
//}
//
//void CommandEventHandler::StartToolDrag(wxDC* pDC)
//{
//    PrepareToolDragImages();
//    wxBitmap* pCursorDragImage = (wxBitmap*)NULL;
//    if (m_pToolBitmap)
//    {
//        pCursorDragImage = new wxBitmap(*m_pToolBitmap);
//        m_vDragHotSpot = m_vToolHotSpot;
//
//        //wxLogMessage(_T("[CommandEventHandler::StartToolDrag] OnImageBeginDrag. m_nMousePointedArea=%d, MousePagePos=(%.2f, %.2f)"),
//        //                m_nMousePointedArea, m_uMousePagePos.x, m_uMousePagePos.y);
//
//        m_fDraggingTool = true;
//    }
//    else
//        m_fDraggingTool = false;
//
//    m_pView->OnImageBeginDrag(true, pDC, m_vCanvasOffset, m_uMousePagePos,
//                            (GmoObj*)NULL, m_vDragHotSpot, m_uHotSpotShift,
//                            pCursorDragImage );
//}
//
//void CommandEventHandler::ContinueToolDrag(wxMouseEvent& event, wxDC* pDC)
//{
//    //wxLogMessage(_T("[CommandEventHandler::ContinueToolDrag] OnImageContinueDrag. m_nMousePointedArea=%d, MousePagePos=(%.2f, %.2f)"),
//    //                m_nMousePointedArea, m_uMousePagePos.x, m_uMousePagePos.y);
//
//    m_pView->OnImageContinueDrag(event, true, pDC, m_vCanvasOffset,
//                                 m_uMousePagePos, m_vMouseCanvasPos);
//}
//
//void CommandEventHandler::TerminateToolDrag(wxDC* pDC)
//{
//    //wxLogMessage(_T("[CommandEventHandler::TerminateToolDrag] Terminate drag. m_nMousePointedArea=%d, MousePagePos=(%.2f, %.2f)"),
//    //                m_nMousePointedArea, m_uMousePagePos.x, m_uMousePagePos.y);
//
//    if (!m_fDraggingTool)
//        return;
//
//    m_pView->OnImageEndDrag(true, pDC, m_vCanvasOffset, m_uMousePagePos);
//    m_fDraggingTool = false;
//}
//
//void CommandEventHandler::TerminateToolDrag()
//{
//    // Set a DC in logical units and scaled, so that
//    // transformations logical/device and viceversa can be computed
//    wxClientDC dc(this);
//    m_pView->ScaleDC(&dc);
//    TerminateToolDrag(&dc);
//}
//
//void CommandEventHandler::StartToolDrag()
//{
//    // Set a DC in logical units and scaled, so that
//    // transformations logical/device and viceversa can be computed
//    wxClientDC dc(this);
//    m_pView->ScaleDC(&dc);
//    StartToolDrag(&dc);
//}
//
////------------------------------------------------------------------------------------------
////call backs from lmScoreView to paint marks for mouse dragged tools.
////
////  - pPaper is already prepared for dirct XOR paint and is scaled and the origin set.
////  - uPos is the mouse current position and they must return the nearest
////    valid notehead position
////------------------------------------------------------------------------------------------
//
//UPoint CommandEventHandler::OnDrawToolMarks(lmPaper* pPaper, const UPoint& uPos)
//{
//    UPoint uFinalPos = uPos;
//
//    //draw ledger lines
//    if (RequiresLedgerLines())
//    {
//        if (m_pCurShapeStaff)
//            uFinalPos = m_pCurShapeStaff->OnMouseStartMoving(pPaper, uPos);
//    }
//
//    ////show tool tip
//    //m_pInfoWindow = new lmInfoWindow(this);
//    //wxSize sz = m_pInfoWindow->GetSize();
//    //wxPoint pos(m_vMouseCanvasPos.x + m_vCanvasOffset.x,
//    //            m_vMouseCanvasPos.y + m_vCanvasOffset.y );
//    //m_pInfoWindow->Position(pos, sz );
//    //m_pInfoWindow->Popup();
//    //this->SetFocus();
//
//
//
//    //draw time grid
//    if (m_pCurBSI && RequiresTimeGrid())
//        m_pCurBSI->DrawTimeGrid(pPaper);
//
//    //draw measure frame
//    if (m_pCurBSI && RequiresMeasureFrame())
//        m_pCurBSI->DrawMeasureFrame(pPaper);
//
//    return uFinalPos;
//}
//
//UPoint CommandEventHandler::OnRedrawToolMarks(lmPaper* pPaper, const UPoint& uPos)
//{
//    UPoint uFinalPos = uPos;
//
//    //remove and redraw ledger lines
//    if (RequiresLedgerLines())
//    {
//        if (m_pLastShapeStaff && m_pLastShapeStaff != m_pCurShapeStaff)
//        {
//            m_pLastShapeStaff->OnMouseEndMoving(pPaper, uPos);
//            if (m_pCurShapeStaff)
//                uFinalPos = m_pCurShapeStaff->OnMouseStartMoving(pPaper, uPos);
//        }
//        else if (m_pCurShapeStaff)
//            uFinalPos = m_pCurShapeStaff->OnMouseMoving(pPaper, uPos);
//    }
//
//    //remove previous grid
//    if (RequiresTimeGrid())
//    {
//        if (m_pLastBSI && m_pLastBSI != m_pCurBSI)
//            m_pLastBSI->DrawTimeGrid(pPaper);
//
//        //draw new grid
//        if (m_pCurBSI && (!m_pLastBSI || m_pLastBSI != m_pCurBSI))
//            m_pCurBSI->DrawTimeGrid(pPaper);
//    }
//
//    //remove previous measure frame
//    if (RequiresMeasureFrame())
//    {
//        if (m_pLastBSI && m_pLastBSI != m_pCurBSI)
//            m_pLastBSI->DrawMeasureFrame(pPaper);
//
//        //draw measure frame
//        if (m_pCurBSI && (!m_pLastBSI || m_pLastBSI != m_pCurBSI))
//            m_pCurBSI->DrawMeasureFrame(pPaper);
//    }
//
//    return uFinalPos;
//}
//
//UPoint CommandEventHandler::OnRemoveToolMarks(lmPaper* pPaper, const UPoint& uPos)
//{
//    UPoint uFinalPos = uPos;
//
//    //remove ledger lines
//    if (RequiresLedgerLines())
//    {
//        if (m_pLastShapeStaff && m_pLastShapeStaff != m_pCurShapeStaff)
//            m_pLastShapeStaff->OnMouseEndMoving(pPaper, uPos);
//        else if (m_pCurShapeStaff)
//            m_pCurShapeStaff->OnMouseEndMoving(pPaper, uPos);
//    }
//
//    //remove previous time grid
//    if (m_pLastBSI && RequiresTimeGrid())
//        m_pLastBSI->DrawTimeGrid(pPaper);
//
//    //remove previous measure frame
//    if (m_pLastBSI && RequiresMeasureFrame())
//        m_pLastBSI->DrawMeasureFrame(pPaper);
//
//    return uFinalPos;
//}
//
//
//
////------------------------------------------------------------------------------------------
//
//void CommandEventHandler::ChangeMouseIcon()
//{
//    //change mouse icon if necessary. Type of area currently pointed by mouse is in global variable
//    //m_nMousePointedArea.
//
//    //determine needed icon
//    wxCursor* pNeeded = m_pCursorElse;
//    if (m_nMousePointedArea & m_nValidAreas)
//        pNeeded = m_pCursorOnValidArea;
//
//    //get current cursor
//    if (m_pCursorCurrent != pNeeded)
//    {
//        m_pCursorCurrent = pNeeded;
//        const wxCursor& oCursor = *pNeeded;
//        SetCursor( oCursor );
//    }
//}
//
//void CommandEventHandler::PlayScore(bool fFromCursor, bool fCountOff)
//{
//    //get the score
//    lmScore* pScore = m_pDoc->GetScore();
//
//	//determine measure from cursor or start of selection
//	int nMeasure = 1;
//	lmGMSelection* pSel = m_pView->GetSelection();
//	bool fFromMeasure = fFromCursor || pSel->NumObjects() > 0;
//	if (pSel->NumObjects() > 0)
//	{
//		nMeasure = ((ImoNote*)pSel->GetFirst()->GetScoreOwner())->GetSegment()->GetNumSegment() + 1;
//		m_pView->DeselectAllGMObjects(true);	//redraw, to remove selection highlight
//	}
//	else
//		nMeasure = m_pView->GetCursorMeasure();
//
//	//play back the score
//	if (fFromMeasure)
//		pScore->PlayFromMeasure(nMeasure, lmVISUAL_TRACKING, fCountOff,
//                                ePM_NormalInstrument, 0, this);
//	else
//		pScore->Play(lmVISUAL_TRACKING, fCountOff, ePM_NormalInstrument, 0, this);
//}
//
//void CommandEventHandler::StopPlaying(bool fWait)
//{
//    //get the score
//    lmScore* pScore = m_pDoc->GetScore();
//    if (!pScore) return;
//
//    //request it to stop playing
//    pScore->Stop();
//    if (fWait)
//        pScore->WaitForTermination();
//}
//
//void CommandEventHandler::PausePlaying()
//{
//    //get the score
//    lmScore* pScore = m_pDoc->GetScore();
//
//    //request it to pause playing
//    pScore->Pause();
//}
//
//void CommandEventHandler::OnVisualHighlight(lmScoreHighlightEvent& event)
//{
//    m_pView->OnVisualHighlight(event);
//}
//




////---------------------------------------------------------------------------------------
//
//wxCursor* CommandEventHandler::LoadMouseCursor(wxString sFile, int nHotSpotX, int nHotSpotY)
//{
//    //loads all mouse cursors used in CommandEventHandler
//
//    wxString sPath = g_pPaths->GetCursorsPath();
//    wxFileName oFilename(sPath, sFile, wxPATH_NATIVE);
//    wxCursor* pCursor;
//
//    //load image
//    wxImage oImage(oFilename.GetFullPath(), wxBITMAP_TYPE_PNG);
//    if (!oImage.IsOk())
//    {
//        wxLogMessage(_T("[CommandEventHandler::LoadMouseCursor] Failure loading mouse cursor image '%s'"),
//                     oFilename.GetFullPath().c_str());
//        return NULL;
//    }
//
//    //set hot spot point
//    oImage.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, nHotSpotX);
//    oImage.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, nHotSpotY);
//
//    //create the cursor
//    pCursor = new wxCursor(oImage);
//    if (!pCursor->IsOk())
//    {
//        wxLogMessage(_T("[CommandEventHandler::LoadMouseCursor] Failure creating mouse cursor from image '%s'"),
//                     oFilename.GetFullPath().c_str());
//        delete pCursor;
//        return NULL;
//    }
//
//    return pCursor;
//}
//
//void CommandEventHandler::LoadAllMouseCursors()
//{
//    //loads all mouse cursors used in CommandEventHandler
//
//    //AWARE: Cursors MUST BE LOADED in the same order than enum lmEMouseCursor.
//    //Therefore, this has been implemented as a 'for' loop, to ensure previous
//    //requirement and simplify code maintenance
//
//    wxCursor* pCursor;
//    for (int iCursor = 0; iCursor < (int)lm_eCursor_Max; iCursor++)
//    {
//        switch((lmEMouseCursor)iCursor)
//        {
//            case lm_eCursor_Pointer:
//                pCursor = new wxCursor(wxCURSOR_ARROW);
//                break;
//            case lm_eCursor_Cross:
//                pCursor = new wxCursor(wxCURSOR_CROSS);
//                break;
//            case lm_eCursor_BullsEye:
//                pCursor = new wxCursor(wxCURSOR_BULLSEYE);
//                break;
//            case lm_eCursor_Hand:
//                pCursor = new wxCursor(wxCURSOR_HAND);
//                break;
//            case lm_eCursor_Note:
//                pCursor = LoadMouseCursor(_T("cursor-note.png"), 8, 8);
//                break;
//            case lm_eCursor_Note_Forbidden:
//                pCursor = new wxCursor(wxCURSOR_NO_ENTRY);
//                break;
//            default:
//                wxLogMessage(_T("[CommandEventHandler::LoadAllMouseCursors] Missing value (%d) in swith statement"),
//                             iCursor);
//                pCursor = new wxCursor(wxCURSOR_ARROW);
//        }
//        m_MouseCursors.push_back(pCursor);
//    }
//
//    ////lm_eCursor_Pointer
//    //wxCursor* pCursor = new wxCursor(wxCURSOR_BULLSEYE );   //wxCURSOR_ARROW);
//    //m_MouseCursors.push_back(pCursor);
//
//    ////lm_eCursor_Cross
//    //pCursor = new wxCursor(wxCURSOR_CROSS);
//    //m_MouseCursors.push_back(pCursor);
//
//    ////lm_eCursor_BullsEye
//    //wxCursor* pCursor = new wxCursor(wxCURSOR_BULLSEYE );   //wxCURSOR_ARROW);
//    //m_MouseCursors.push_back(pCursor);
//
//    //// lm_eCursor_Note
//    //m_MouseCursors.push_back( LoadMouseCursor(_T("cursor-note.png"), 8, 8) );
//
//    //// lm_eCursor_Note_Forbidden,
//    //pCursor = new wxCursor(wxCURSOR_NO_ENTRY);
//    //m_MouseCursors.push_back(pCursor);
//    ////m_MouseCursors.push_back( LoadMouseCursor(_T("cursor-note-forbidden.png"), 8, 8) );
//
//
//    //set default cursors
//    m_pCursorOnSelectedObject = GetMouseCursor(lm_eCursor_Pointer);
//    m_pCursorOnValidArea = GetMouseCursor(lm_eCursor_Pointer);
//    m_pCursorElse = GetMouseCursor(lm_eCursor_Pointer);
//
//}
//
//wxCursor* CommandEventHandler::GetMouseCursor(lmEMouseCursor nCursorID)
//{
//    return m_MouseCursors[nCursorID];
//}
//
//void CommandEventHandler::UpdateValidAreasAndMouseIcons()
//{
//    //Determine valid areas and cursor icons. This will depend on selected tool
//
//    //default values
//    lmEMouseCursor nSelected = lm_eCursor_Pointer;
//    lmEMouseCursor nValidArea = lm_eCursor_Pointer;
//    lmEMouseCursor nElse = lm_eCursor_Pointer;
//
//
//    if (mouseMode == k_mouse_mode_data_entry)
//    {
//        //Determine valid areas and cursor icons. This will depend on the
//        //selected tool
//        switch(pageID)
//        {
//            case k_page_clefs:
//                switch (groupID)
//                {
//                    case k_grp_ClefType:
//                        m_nValidAreas = lmMOUSE_OnStaff;
//                        break;
//                    case k_grp_TimeType:
//                    case k_grp_KeyType:
//                    default:
//                        m_nValidAreas = lmMOUSE_OnStaff | lmMOUSE_OnNotesRests;
//                }
//                nValidArea = lm_eCursor_Hand;
//                nSelected = lm_eCursor_Pointer;
//                nElse = lm_eCursor_Note_Forbidden;
//                break;
//
//            case k_page_notes:
//                m_nValidAreas = lmMOUSE_OnStaff | lmMOUSE_OnAboveStaff | lmMOUSE_OnBelowStaff;
//                nSelected = lm_eCursor_Pointer;
//                nValidArea = lm_eCursor_Pointer;
//                nElse = lm_eCursor_Note_Forbidden;
//                break;
//
//            case k_page_barlines:
//                m_nValidAreas = lmMOUSE_OnStaff;
//                nValidArea = lm_eCursor_Hand;
//                nSelected = lm_eCursor_Pointer;
//                nElse = lm_eCursor_Note_Forbidden;
//                break;
//
//            case k_page_sysmbols:
//                m_nValidAreas = lmMOUSE_OnNotesRests;
//                nSelected = lm_eCursor_Pointer;
//                nValidArea = lm_eCursor_Hand;
//                nElse = lm_eCursor_Note_Forbidden;
//                break;
//
//            default:
//                wxASSERT(false);
//        }
//
//        //hide caret
//        m_pView->CaretOff();
//    }
//    else
//    {
//        m_nValidAreas = lmMOUSE_OnAny;
//        //show caret
//        m_pView->CaretOn();
//    }
//
//    //set cursors
//    m_pCursorOnSelectedObject = GetMouseCursor(nSelected);
//    m_pCursorOnValidArea = GetMouseCursor(nValidArea);
//    m_pCursorElse = GetMouseCursor(nElse);
//    ChangeMouseIcon();
//}
//
//
//
//void CommandEventHandler::UpdateStatusBarToolBox(wxString sMoreInfo)
//{
//    //update status bar: mouse mode and selected tool
//
//    wxString sMsg = _T("");
//    if (mouseMode == k_mouse_mode_pointer)
//        sMsg = _("Pointer mode");
//    else if (mouseMode == k_mouse_mode_data_entry)
//    {
//	    ToolBox* pToolBox = GetMainFrame()->GetActiveToolBox();
//	    wxASSERT(pToolBox);
//        sMsg = pToolBox->GetToolShortDescription();
//        sMsg += sMoreInfo;
//    }
//
//    GetMainFrame()->SetStatusBarMsg(sMsg);
//}

//void CommandEventHandler::PrepareToolDragImages()
//{
//    //prepare drag image for current selected tool.
//    //This method must set variables m_pToolBitmap and m_vToolHotSpot
//    //If m_pToolBitmap is set to NULL it will imply that no drag image will be used.
//
//    //TODO: It should be responsibility of each tool to provide this information. Move
//    //this code to each ToolPage in the ToolBox. Later,move to each tool group (not to
//    //options groups)
//
//    //TODO: Drag image is related to mouse cursor to use. Should we merge this method with
//    //the one used to determine mouse cursor to use? --> Yes, as hotspot position depends
//    //on chosen mouse cursor
//
//
//    //delete previous bitmaps
//    if (m_pToolBitmap)
//        delete m_pToolBitmap;
//
//    //create new ones
//    wxColour colorF = *wxBLUE;
//    wxColour colorB = *wxWHITE;
//    lmStaff* pStaff = m_pView->GetDocument()->GetScore()->GetFirstInstrument()->GetVStaff()->GetFirstStaff();
//    double rPointSize = pStaff->GetMusicFontSize();
//    double rScale = m_pView->GetScale() * lmSCALE;
//
//    //Determine glyph to use for current tool. Value GLYPH_NONE means "Do not use a drag image".
//    //And in this case only mouse cursor will be used
//    lmEGlyphIndex nGlyph = GLYPH_NONE;      //Default: Do not use a drag image
//    switch (pageID)
//    {
//        case k_page_notes:
//            {
//                //in 'TheoHarmonyCtrol' edit mode, force stem depending on voice
//                bool fStemDown = false;
//                lmEditorMode* pEditorMode = m_pDoc->GetEditMode();
//                if (pEditorMode && pEditorMode->GetModeName() == _T("TheoHarmonyCtrol"))
//                {
//                    switch(m_nSelVoice)
//                    {
//                        case 1: fStemDown = false;  break;
//                        case 2: fStemDown = true;   break;
//                        case 3: fStemDown = false;  break;
//                        case 4: fStemDown = true;   break;
//                        default:
//                            fStemDown = false;
//                    }
//                }
//
//                //select glyph
//                nGlyph = lmGetGlyphForNoteRest(m_nSelNoteType, m_fSelIsNote, fStemDown);
//                break;
//            }
//
//        case k_page_sysmbols:
//            {
//                switch (m_nToolID)
//                {
//                    case lmTOOL_FIGURED_BASS:
//                    case lmTOOL_TEXT:
//                    case lmTOOL_LINES:
//                    case lmTOOL_TEXTBOX:
//                        nGlyph = GLYPH_NONE;    //GLYPH_TOOL_GENERIC;
//                        break;
//                    default:
//                        nGlyph = GLYPH_NONE;    //GLYPH_TOOL_GENERIC;
//                }
//                break;
//            }
//
//        case k_page_clefs:
//            {
//                switch (groupID)
//                {
//                    case k_grp_ClefType:
//                        nGlyph = lmGetGlyphForCLef(m_nClefType);
//                        break;
//                    case k_grp_TimeType:
//                    case k_grp_KeyType:
//                        nGlyph = GLYPH_NONE;    //GLYPH_TOOL_GENERIC;
//                        break;
//                    default:
//                        nGlyph = GLYPH_NONE;    //GLYPH_TOOL_GENERIC;
//                }
//                break;
//            }
//
//        case k_page_barlines:
//            {
//                nGlyph = GLYPH_NONE;    //GLYPH_TOOL_GENERIC;
//                break;
//            }
//    }
//
//    //create the bitmap
//    if (nGlyph != GLYPH_NONE)
//    {
//        m_pToolBitmap = GetBitmapForGlyph(rScale, nGlyph, rPointSize, colorF, colorB);
//        float rxScale = aGlyphsInfo[nGlyph].txDrag / aGlyphsInfo[nGlyph].thWidth;
//        float ryScale = aGlyphsInfo[nGlyph].tyDrag / aGlyphsInfo[nGlyph].thHeight;
//        m_vToolHotSpot = lmDPoint(m_pToolBitmap->GetWidth() * rxScale,
//                                m_pToolBitmap->GetHeight() * ryScale );
//    }
//    else
//    {
//        //No drag image. Only mouse cursor
//        m_pToolBitmap = (wxBitmap*)NULL;
//    }
//
//}
//
//void CommandEventHandler::UpdateToolInfoString()
//{
//    //Add note pitch in status bar inofo
//    if (!(pageID == k_page_notes && m_fSelIsNote) || m_pCurShapeStaff == NULL)
//        return;
//
//    lmDPitch dpNote = GetNotePitchFromPosition(m_pCurShapeStaff, m_uMousePagePos);
//    wxString sMoreInfo = wxString::Format(_T(" %s"), DPitch_ToLDPName(dpNote).c_str() );
//    UpdateStatusBarToolBox(sMoreInfo);
//}
//
//lmDPitch CommandEventHandler::GetNotePitchFromPosition(lmShapeStaff* pShapeStaff, UPoint uPagePos)
//{
//    //return pitch for specified mouse position on staff
//
//    //get step and octave from mouse position on staff
//    int nLineSpace = pShapeStaff->GetLineSpace(uPagePos.y);     //0=first ledger line below staff
//    //to determine octave and step it is necessary to know the clef. As caret is
//    //placed at insertion point we could get these information from caret
//    lmContext* pContext = m_pDoc->GetScore()->GetCursor()->GetCurrentContext();
//    lmEClefType nClefType = (pContext ? pContext->GetClefType() : lmE_Undefined);
//    if (nClefType == lmE_Undefined)
//        nClefType = ((lmStaff*)(pShapeStaff->GetScoreOwner()))->GetDefaultClef();
//    lmDPitch dpNote = ::GetFirstLineDPitch(nClefType);  //get diatonic pitch for first line
//    dpNote += (nLineSpace - 2);     //pitch for note to insert
//
//    return dpNote;
//}
//
//void CommandEventHandler::OnKeyDown(wxKeyEvent& event)
//{
//    //wxLogMessage(_T("EVT_KEY_DOWN"));
//    switch ( event.GetKeyCode() )
//    {
//        case WXK_SHIFT:
//        case WXK_ALT:
//        case WXK_CONTROL:
//            break;      //do nothing
//
//        default:
//            //save key down info
//            m_nKeyDownCode = event.GetKeyCode();
//            m_fShift = event.ShiftDown();
//            m_fAlt = event.AltDown();
//            m_fCmd = event.CmdDown();
//
//            //If a key down (EVT_KEY_DOWN) event is caught and the event handler does not
//            //call event.Skip() then the corresponding char event (EVT_CHAR) will not happen.
//            //This is by design of wxWidgets and enables the programs that handle both types of
//            //events to be a bit simpler.
//
//            //event.Skip();       //to generate Key char event
//            process_key(event);
//    }
//}
//
//void CommandEventHandler::OnKeyPress(wxKeyEvent& event)
//{
//    //wxLogMessage(_T("[CommandEventHandler::OnKeyPress] KeyCode=%s (%d), KeyDown data: Keycode=%s (%d), (flags = %c%c%c%c)"),
//    //        KeyCodeToName(event.GetKeyCode()).c_str(), event.GetKeyCode(),
//    //        KeyCodeToName(m_nKeyDownCode).c_str(), m_nKeyDownCode,
//    //        (m_fCmd ? _T('C') : _T('-') ),
//    //        (m_fAlt ? _T('A') : _T('-') ),
//    //        (m_fShift ? _T('S') : _T('-') ),
//    //        (event.MetaDown() ? _T('M') : _T('-') )
//    //        );
//    //process_key(event);
//}


//=======================================================================================
// KeyHandler implementation
//=======================================================================================
KeyHandler::KeyHandler(DocumentWindow* pController, ToolsInfo& toolsInfo,
                       SelectionSet& selection, DocCursor* cursor)
    : m_pController(pController)
    , m_toolsInfo(toolsInfo)
    , m_selection(selection)
    , m_cursor(cursor)
    , m_fEventProcessed(false)
    , m_executer(pController, selection, cursor)
{
}

//---------------------------------------------------------------------------------------
void KeyHandler::add_to_command_buffer(int nKeyCode)
{
    if (wxIsprint(nKeyCode))
    {
        //TODO: add to command buffer
        //m_sCmd += wxString::Format(_T("%c"), (char)nKeyCode);
        m_fEventProcessed = true;
    }
}

//=======================================================================================
// NoToolKeyHandler implementation
//=======================================================================================
void NoToolKeyHandler::process_key(wxKeyEvent& event)
{
    m_fEventProcessed = false;
    int nKeyCode = event.GetKeyCode();

    //fix ctrol+key codes
    if (nKeyCode > 0 && nKeyCode < 27)
        nKeyCode += int('A') - 1;

    add_to_command_buffer(nKeyCode);
}


//=======================================================================================
// ClefsKeyHandler implementation
//=======================================================================================
void ClefsKeyHandler::process_key(wxKeyEvent& event)
{
    m_fEventProcessed = false;
//    int nKeyCode = event.GetKeyCode();

//        case k_page_clefs:	//---------------------------------------------------------
//        {
//   //         fUnknown = false;       //assume it will be processed
//            //switch (nKeyCode)
//            //{
//               // case int('G'):	// 'g' insert G clef
//               // case int('g'):
//                  //  InsertClef(lmE_Sol);
//                  //  break;
//
//               // case int('F'):	// 'f' insert F4 clef
//               // case int('f'):
//                  //  InsertClef(lmE_Fa4);
//                  //  break;
//
//               // case int('C'):    // 'c' insert C3 clef
//               // case int('c'):
//                  //  InsertClef(lmE_Do3);
//                  //  break;
//
//               // default:
//   //                 if (wxIsprint(nKeyCode))
//   //                     m_sCmd += wxString::Format(_T("%c"), (char)nKeyCode);
//                  //  fUnknown = true;
//            //}
//            break;
//        }
}


//=======================================================================================
// NotesKeyHandler implementation
//=======================================================================================
void NotesKeyHandler::process_key(wxKeyEvent& event)
{
    m_fEventProcessed = false;
//    int nKeyCode = event.GetKeyCode();

    //general automata structure:
    //    if terminal symbol
    //        add_to_command_string()
    //        process_command_string()
    //    else if command to change options in Tool Box
    //        edit_tools_change()
    //    else if single key command (options taken from context)   <-- terminal keys can be considered as single cmd key
    //        exec_lomse_command()
    //    else
    //        add_to_command_buffer()


//        case k_page_notes:	//---------------------------------------------------------
//        {
//            ToolPageNotes* pNoteOptions = pToolBox->GetNoteProperties();
//            m_nSelNoteType = pNoteOptions->GetNoteDuration();
//            m_nSelDots = pNoteOptions->GetNoteDots();
//            m_nSelNotehead = pNoteOptions->GetNoteheadType();
//            m_nSelAcc = pNoteOptions->GetNoteAccidentals();
//            m_nOctave = pNoteOptions->GetOctave();
//            m_nSelVoice = pNoteOptions->GetVoice();
//
//            bool fTiedPrev = false;
//
//            //if terminal symbol, analyze full command
//            if ((nKeyCode >= int('A') && nKeyCode <= int('G')) ||
//                (nKeyCode >= int('a') && nKeyCode <= int('g')) ||
//                nKeyCode == int(' ') )
//            {
//                if (m_sCmd != _T(""))
//                {
//                    lmKbdCmdParser oCmdParser;
//                    if (oCmdParser.ParserCommand(m_sCmd))
//                    {
//                        m_nSelAcc = oCmdParser.GetAccidentals();
//                        m_nSelDots = oCmdParser.GetDots();
//                        fTiedPrev = oCmdParser.GetTiedPrev();
//                    }
//                }
//            }
//
//            //compute note/rest duration
//            float rDuration = lmLDPParser::GetDefaultDuration(m_nSelNoteType, m_nSelDots, 0, 0);
//
//            //insert note
//            if ((nKeyCode >= int('A') && nKeyCode <= int('G')) ||
//                (nKeyCode >= int('a') && nKeyCode <= int('g')) )
//            {
//                //convert key to upper case
//                if (nKeyCode > int('G'))
//                    nKeyCode -= 32;
//
//                // determine octave
//                if (event.ShiftDown())
//                    ++m_nOctave;
//                else if (event.CmdDown())
//                    --m_nOctave;
//
//                //limit octave 0..9
//                if (m_nOctave < 0)
//                    m_nOctave = 0;
//                else if (m_nOctave > 9)
//                    m_nOctave = 9;
//
//                //get step
//                static wxString sSteps = _T("abcdefg");
//                int nStep = LetterToStep( sSteps.GetChar( nKeyCode - int('A') ));
//
//                //check if the note is added to form a chord and determine base note
//                ImoNote* pBaseOfChord = (ImoNote*)NULL;
//                if (event.AltDown())
//                {
//                    lmStaffObj* pSO = m_pDoc->GetScore()->GetCursor()->GetStaffObj();
//                    if (pSO && pSO->IsNote())
//                        pBaseOfChord = (ImoNote*)pSO;
//                }
//
//                //do insert note
//                InsertNote(lm_ePitchRelative, nStep, m_nOctave, m_nSelNoteType, rDuration,
//                           m_nSelDots, m_nSelNotehead, m_nSelAcc, m_nSelVoice, pBaseOfChord,
//                           fTiedPrev, lmSTEM_DEFAULT);
//
//                fUnknown = false;
//            }
//
//            //insert rest
//            if (nKeyCode == int(' '))
//            {
//                //do insert rest
//                InsertRest(m_nSelNoteType, rDuration, m_nSelDots, m_nSelVoice);
//
//                fUnknown = false;
//            }
//
//            //commands to change options in Tool Box
//
//
//            //Select note duration:     digits 0..9
//            //Select octave:            ctrl + digits 0..9
//            //Select voice:             alt + digits 0..9
//            if (fUnknown && nKeyCode >= int('0') && nKeyCode <= int('9'))
//            {
//                if (event.CmdDown())
//                    //octave: ctrl + digits 0..9
//                    SelectOctave(nKeyCode - int('0'));
//
//                else if (event.AltDown())
//                    //Voice: alt + digits 0..9
//                    SelectVoice(nKeyCode - int('0'));
//
//                else
//                    //Note duration: digits 0..9
//                    SelectNoteDuration(nKeyCode - int('0'));
//
//                fUnknown = false;
//            }
//
//            //increment/decrement octave: up (ctrl +), down (ctrl -)
//            else if (fUnknown && event.CmdDown()
//                     && (nKeyCode == int('+') || nKeyCode == int('-')) )
//            {
//                SelectOctave(nKeyCode == int('+'));
//                fUnknown = false;
//            }
//
//            //increment/decrement voice: up (alt +), down (alt -)
//            else if (fUnknown && event.AltDown()
//                     && (nKeyCode == int('+') || nKeyCode == int('-')) )
//            {
//                SelectVoice(nKeyCode == int('+'));
//                fUnknown = false;
//            }
//
//
//#if 0   //old code, to select accidentals and dots
//       //     if (fUnknown)
//       //     {
//       //         fUnknown = false;       //assume it
//                //switch (nKeyCode)
//                //{
//       //             //select accidentals
//                   // case int('+'):      // '+' increment accidentals
//       //                 SelectNoteAccidentals(true);
//       //                 break;
//
//       //             case int('-'):      // '-' decrement accidentals
//       //                 SelectNoteAccidentals(false);
//       //                 break;
//
//       //             //select dots
//                   // case int('.'):      // '.' increment/decrement dots
//       //                 if (event.AltDown())
//       //                     SelectNoteDots(false);      // Alt + '.' decrement dots
//       //                 else
//       //                     SelectNoteDots(true);       // '.' increment dots
//       //                 break;
//
//       //             //unknown
//                   // default:
//                      //  fUnknown = true;
//       //         }
//       //     }
//#endif
//
//                //commands requiring to have a note/rest selected
//
//                ////change selected note pitch
//                //case WXK_UP:
//                //	if (nAuxKeys==0)
//                //		ChangeNotePitch(1);		//step up
//                //	else if (nAuxKeys && lmKEY_SHIFT)
//                //		ChangeNotePitch(7);		//octave up
//                //	else
//                //		fUnknown = true;
//                //	break;
//
//                //case WXK_DOWN:
//                //	if (nAuxKeys==0)
//                //		ChangeNotePitch(-1);		//step down
//                //	else if (nAuxKeys && lmKEY_SHIFT)
//                //		ChangeNotePitch(-7);		//octave down
//                //	else
//                //		fUnknown = true;
//                //	break;
//
//
//               // //invalid key
//               // default:
//                  //  fUnknown = true;
//            //}
//
//            //save char if unused
//            if (fUnknown && wxIsprint(nKeyCode))
//                m_sCmd += wxString::Format(_T("%c"), (char)nKeyCode);
//
//            break;      //case k_page_notes
//        }
}


//=======================================================================================
// BarlinesKeyHandler implementation
//=======================================================================================
void BarlinesKeyHandler::process_key(wxKeyEvent& event)
{
    m_fEventProcessed = false;
    int nKeyCode = event.GetKeyCode();

    switch (nKeyCode)
    {
        case int('B'):	//insert selected barline type
        case int('b'):
            m_executer.insert_barline(m_toolsInfo.barlineType);
            m_fEventProcessed = true;
            return;

        default:
            add_to_command_buffer(nKeyCode);
    }
}


//=======================================================================================
// SymbolsKeyHandler implementation
//=======================================================================================
void SymbolsKeyHandler::process_key(wxKeyEvent& event)
{
    m_fEventProcessed = false;
    //No key commands
}


//=======================================================================================


////---------------------------------------------------------------------------------------
//void CommandEventHandler::RestoreToolBoxSelections()
//{
//    //restore toolbox selected options to those previously selected by user
//
//    if (!m_fToolBoxSavedOptions) return;        //nothing to do
//
//    m_fToolBoxSavedOptions = false;
//
//	ToolBox* pToolBox = GetMainFrame()->GetActiveToolBox();
//	if (!pToolBox) return;
//
//    switch( pToolBox->GetCurrentPageID() )
//    {
//        case k_page_none:
//            return;         //nothing selected!
//
//        case k_page_notes:
//            //restore duration, dots, accidentals
//            {
//                ToolPageNotes* pTool = (ToolPageNotes*)pToolBox->GetToolPanel(k_page_notes);
//                pTool->SetNoteDotsButton(m_nTbDots);
//                pTool->SetNoteAccButton(m_nTbAcc);
//                pTool->SetNoteDurationButton(m_nTbDuration);
//            }
//            break;
//
//        case k_page_clefs:
//        case k_page_barlines:
//        case k_page_sysmbols:
//            lmTODO(_T("[CommandEventHandler::RestoreToolBoxSelections] Code to restore this tool"));
//            break;
//
//        default:
//            wxASSERT(false);
//    }
//}
//



//=======================================================================================



//void CommandEventHandler::SetDraggingObject(bool fValue)
//{
//    if (m_fDraggingObject != fValue)
//    {
//        //change of state. Capture or release mouse
//        if (m_fDraggingObject)
//            ReleaseTheMouse();
//        else
//            CaptureTheMouse();
//    }
//
//    m_fDraggingObject = fValue;
//}
//
////dragging on canvas with left button: selection
//void CommandEventHandler::OnCanvasBeginDragLeft(lmDPoint vCanvasPos, UPoint uPagePos,
//                                          int nKeys)
//{
//    //Begin a selection with left button
//
//	WXUNUSED(nKeys);
//
//    wxClientDC dc(this);
//	dc.SetLogicalFunction(wxINVERT);
//    SetDraggingObject(false);
//
//	m_pView->DrawSelectionArea(dc, m_vStartDrag.x, m_vStartDrag.y, vCanvasPos.x, vCanvasPos.y);
//}
//
//void CommandEventHandler::OnCanvasContinueDragLeft(bool fDraw, lmDPoint vCanvasPos,
//                                             UPoint uPagePos, int nKeys)
//{
//    //Continue a selection with left button
//	//fDraw:  true -> draw a rectangle, false -> remove rectangle
//
//    WXUNUSED(fDraw);
//    WXUNUSED(nKeys);
//
//    wxClientDC dc(this);
//    dc.SetLogicalFunction(wxINVERT);
//    SetDraggingObject(false);
//
//    m_pView->DrawSelectionArea(dc, m_vStartDrag.x, m_vStartDrag.y, vCanvasPos.x, vCanvasPos.y);
//}
//
//void CommandEventHandler::OnCanvasEndDragLeft(lmDPoint vCanvasPos, UPoint uPagePos,
//                                        int nKeys)
//{
//    //End a selection with left button
//
//    WXUNUSED(nKeys);
//
//	//remove selection rectangle
//    //dc.SetLogicalFunction(wxINVERT);
//    //DrawSelectionArea(dc, m_vStartDrag.x, m_vStartDrag.y, vCanvasPos.x, vCanvasPos.y);
//    SetDraggingObject(false);
//
//	//save final point
//	m_vEndDrag = vCanvasPos;
//
//    //select all objects within the selection area
//    lmLUnits uXMin, uXMax, uYMin, uYMax;
//    uXMin = wxMin(uPagePos.x, m_uStartDrag.x);
//    uXMax = wxMax(uPagePos.x, m_uStartDrag.x);
//    uYMin = wxMin(uPagePos.y, m_uStartDrag.y);
//    uYMax = wxMax(uPagePos.y, m_uStartDrag.y);
//
//    //find all objects whithin the selected area and create a selection
//    //
//    //TODO
//    //  The selected area could cross page boundaries. Therefore it is necessary
//    //  to locate the affected pages and invoke CreateSelection / AddToSelecction
//    //  for each affected page
//    //
//    if (nKeys == lmKEY_NONE)
//    {
//        m_pView->SelectGMObjectsInArea(m_nNumPage, uXMin, uXMax, uYMin, uYMax, true);     //true: redraw view content
//    }
//    //else if (nKeys & lmKEY_CTRL)
//    //{
//    //    //find all objects in drag area and add them to 'selection'
//    //    m_graphMngr.AddToSelection(m_nNumPage, uXMin, uXMax, uYMin, uYMax);
//    //    //mark as 'selected' all objects in the selection
//    //    m_pCanvas->SelectObjects(lmSELECT, m_graphMngr.GetSelection());
//    //}
//}
//
////dragging on canvas with right button
//void CommandEventHandler::OnCanvasBeginDragRight(lmDPoint vCanvasPos, UPoint uPagePos,
//                                           int nKeys)
//{
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//    SetDraggingObject(false);
//}
//
//void CommandEventHandler::OnCanvasContinueDragRight(bool fDraw, lmDPoint vCanvasPos,
//                                              UPoint uPagePos, int nKeys)
//{
//    WXUNUSED(fDraw);
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//    SetDraggingObject(false);
//}
//
//void CommandEventHandler::OnCanvasEndDragRight(lmDPoint vCanvasPos, UPoint uPagePos,
//                                         int nKeys)
//{
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//    SetDraggingObject(false);
//    SetFocus();
//}
//
//
////dragging object with left button
//void CommandEventHandler::OnObjectBeginDragLeft(wxMouseEvent& event, wxDC* pDC,
//                                          lmDPoint vCanvasPos, lmDPoint vCanvasOffset,
//                                          UPoint uPagePos, int nKeys)
//{
//    SetDraggingObject(true);
//    if (!m_pView->OnObjectBeginDragLeft(event, pDC, vCanvasPos, vCanvasOffset,
//                                        uPagePos, nKeys, m_pDraggedGMO,
//                                        m_vDragHotSpot, m_uHotSpotShift) )
//    {
//        m_nDragState = lmDRAG_NONE;
//        SetDraggingObject(false);
//    }
//}
//
//void CommandEventHandler::OnObjectContinueDragLeft(wxMouseEvent& event, wxDC* pDC,
//                                             bool fDraw, lmDPoint vCanvasPos,
//                                             lmDPoint vCanvasOffset, UPoint uPagePos,
//                                             int nKeys)
//{
//    SetDraggingObject(true);
//    m_pView->OnObjectContinueDragLeft(event, pDC, fDraw, vCanvasPos,
//                                      vCanvasOffset, uPagePos, nKeys);
//}
//
//void CommandEventHandler::OnObjectEndDragLeft(wxMouseEvent& event, wxDC* pDC,
//                                        lmDPoint vCanvasPos, lmDPoint vCanvasOffset,
//                                        UPoint uPagePos, int nKeys)
//{
//    m_pView->OnObjectEndDragLeft(event, pDC, vCanvasPos, vCanvasOffset,
//                                 uPagePos, nKeys);
//    SetDraggingObject(false);
//}
//
//
////dragging object with right button
//void CommandEventHandler::OnObjectBeginDragRight(wxMouseEvent& event, wxDC* pDC,
//                                           lmDPoint vCanvasPos, lmDPoint vCanvasOffset,
//                                           UPoint uPagePos, int nKeys)
//{
//    WXUNUSED(event);
//    WXUNUSED(pDC);
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(vCanvasOffset);
//    WXUNUSED(nKeys);
//    WXUNUSED(uPagePos);
//
//    SetDraggingObject(true);
//	m_pView->HideCaret();
//    SetFocus();
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnObjectBeginDragRight()"));
//	#endif
//
//}
//
//void CommandEventHandler::OnObjectContinueDragRight(wxMouseEvent& event, wxDC* pDC,
//                                              bool fDraw, lmDPoint vCanvasPos,
//                                              lmDPoint vCanvasOffset,
//                                              UPoint uPagePos, int nKeys)
//{
//    WXUNUSED(event);
//    WXUNUSED(pDC);
//    WXUNUSED(fDraw);
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(vCanvasOffset);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnObjectContinueDragRight()"));
//	#endif
//
//    SetDraggingObject(true);
//}
//
//void CommandEventHandler::OnObjectEndDragRight(wxMouseEvent& event, wxDC* pDC,
//                                         lmDPoint vCanvasPos, lmDPoint vCanvasOffset,
//                                         UPoint uPagePos, int nKeys)
//{
//    WXUNUSED(event);
//    WXUNUSED(pDC);
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(vCanvasOffset);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnObjectEndDragRight()"));
//	#endif
//
//	m_pView->ShowCaret();
//    SetDraggingObject(false);
//}
//
//void CommandEventHandler::MoveCursorTo(lmBoxSliceInstr* pBSI, int nStaff, float rTime,
//                                 bool fEndOfTime)
//{
//    //Move cursor to specified position
//
//    if (pBSI)
//    {
//	    lmVStaff* pVStaff = pBSI->GetInstrument()->GetVStaff();
//	    int nMeasure = pBSI->GetNumMeasure();
//        m_pView->MoveCursorTo(pVStaff, nStaff, nMeasure, rTime, fEndOfTime);
//    }
//    else
//    {
//        //empty score. Move to start
//        //TODO
//    }
//}
//
//void CommandEventHandler::MoveCursorNearTo(lmBoxSliceInstr* pBSI, UPoint uPagePos, int nStaff)
//{
//    //Move cursor to nearest object after position uPagePos, constrained to specified
//    //segment (specified by BoxSliceInstr) and staff. This method is mainly to position
//    //cursor at mouse click point
//
//    if (pBSI)
//    {
//	    lmVStaff* pVStaff = pBSI->GetInstrument()->GetVStaff();
//	    int nMeasure = pBSI->GetNumMeasure();
//	    m_pView->MoveCursorNearTo(uPagePos, pVStaff, nStaff, nMeasure);
//    }
//    else
//    {
//        //empty score. Move to start
//        //TODO
//    }
//}
//
////non-dragging events: click on an object
//void CommandEventHandler::OnLeftClickOnObject(GmoObj* pGMO, lmDPoint vCanvasPos,
//                                        UPoint uPagePos, int nKeys)
//{
//    // mouse left click on object
//    // uPagePos: click point, referred to current page origin
//
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(nKeys);
//
//    //AWARE: pGMO must exist and it must be a shape. It can not be an lmBox as, since renderization
//    //of shapes is organized in layers, lmBoxes are no longer taken into account in hit testing.
//    wxASSERT(pGMO && pGMO->IsShape());
//
//
//	m_pView->HideCaret();
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnLeftClickOnObject()"));
//	#endif
//
//    m_pView->DeselectAllGMObjects(true);
//    SetFocus();
//
//    if (pGMO->IsShapeStaff())
//    {
//	    //Click on a staff. Move cursor to that staff and nearest note/rest to click point
//        lmShapeStaff* pSS = (lmShapeStaff*)pGMO;
//        lmBox* pBox = m_pView->FindBoxAt(m_nNumPage, uPagePos);
//        wxASSERT(pBox && pBox->IsBoxSliceInstr());
//        lmBoxSliceInstr* pBSI = (lmBoxSliceInstr*)pBox;
//	    lmVStaff* pVStaff = pBSI->GetInstrument()->GetVStaff();
//	    int nMeasure = pBSI->GetNumMeasure();
//	    int nStaff = pSS->GetNumStaff();
//	    m_pView->MoveCaretNearTo(uPagePos, pVStaff, nStaff, nMeasure);
//    }
//    else
//    {
//        //if it is a staffobj move cursor to it. Else do nothing
//        //wxLogMessage(_T("[CommandEventHandler::OnLeftClickOnObject] Click on shape"));
//        lmScoreObj* pSCO = pGMO->GetScoreOwner();
//        if (pSCO->IsComponentObj())
//            m_pView->MoveCaretToObject(pGMO);
//    }
//
//    m_pView->ShowCaret();
//}
//
//void CommandEventHandler::OnRightClickOnObject(GmoObj* pGMO, lmDPoint vCanvasPos,
//                                         UPoint uPagePos, int nKeys)
//{
//    // mouse right click on object: show contextual menu for that object
//
//    WXUNUSED(uPagePos);
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnRightClickOnObject()"));
//	#endif
//
//	m_pView->HideCaret();
//    m_pView->DeselectAllGMObjects();
//    SetFocus();
//
//    if (pGMO->IsSelectable())
//        m_pView->SelectGMObject(pGMO, true);     //true: redraw view content
//    pGMO->OnRightClick(this, vCanvasPos, nKeys);
//	m_pView->ShowCaret();
//}
//
//void CommandEventHandler::OnLeftDoubleClickOnObject(GmoObj* pGMO, lmDPoint vCanvasPos,
//                                              UPoint uPagePos, int nKeys)
//{
//    // mouse left double click: Select/deselect the object pointed by mouse
//
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnLeftDoubleClickOnObject()"));
//	#endif
//
//	m_pView->HideCaret();
//    SetFocus();
//
//    //ComponentObjs and other score objects (lmBoxXXXX) has all its measurements
//    //relative to each page start position
//
//    //select/deselect the object
//    if (pGMO->IsSelectable())
//        m_pView->SelectGMObject(pGMO, true);     //true: redraw view content
//
//	m_pView->ShowCaret();
//}
//
//void CommandEventHandler::OnRightDoubleClickOnObject(GmoObj* pGMO, lmDPoint vCanvasPos,
//                                               UPoint uPagePos, int nKeys)
//{
//    // mouse right double click: To be defined
//
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//	#ifdef _LM_DEBUG_
//	g_pLogger->LogTrace(_T("OnMouseEvent"), _T("OnRightDoubleClickOnObject()"));
//	#endif
//
//	m_pView->HideCaret();
//    m_pView->DeselectAllGMObjects(true);
//    SetFocus();
//	m_pView->ShowCaret();
//}
//
//
////non-dragging events: click on canvas
//void CommandEventHandler::OnRightClickOnCanvas(lmDPoint vCanvasPos, UPoint uPagePos,
//                                         int nKeys)
//{
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//    m_pView->DeselectAllGMObjects(true);     //true: redraw view content
//
//    lmScore* pScore = m_pDoc->GetScore();
//    pScore->PopupMenu(this, (GmoObj*)NULL, vCanvasPos);
//}
//
//void CommandEventHandler::OnLeftClickOnCanvas(lmDPoint vCanvasPos, UPoint uPagePos,
//                                        int nKeys)
//{
//    WXUNUSED(vCanvasPos);
//    WXUNUSED(uPagePos);
//    WXUNUSED(nKeys);
//
//    m_pView->DeselectAllGMObjects(true);     //true: redraw view content
//    SetFocus();
//}
//
//void CommandEventHandler::OnViewUpdated()
//{
//    //The view informs that it has updated the display
//
//    //clear mouse information
//    m_nDragState = lmDRAG_NONE;
//	m_vEndDrag = lmDPoint(0, 0);
//	m_vStartDrag.x = 0;
//	m_vStartDrag.y = 0;
//}
//
//void CommandEventHandler::OnNewGraphicalModel()
//{
//    //Called by the view when the graphical model has been recreated.
//    //This implies that any saved pointer to a lmObject is no longer valid.
//    //This method should deal with these pointer.
//
//	m_pDraggedGMO = (GmoObj*)NULL;	    //object being dragged
//	m_pMouseOverGMO = (GmoObj*)NULL;	//object on which mouse was flying over
//}
//
//
////---------------------------------------------------------------------------
//// Implementation of class lmEditorMode: Helper class to define editor modes
////---------------------------------------------------------------------------
//
//
//lmEditorMode::lmEditorMode(wxClassInfo* pControllerInfo, wxClassInfo* pScoreProcInfo)
//    : m_pControllerInfo(pControllerInfo)
//    , m_pScoreProcInfo(pScoreProcInfo)
//    , m_sCreationModeName(wxEmptyString)
//    , m_sCreationModeVers(wxEmptyString)
//    , m_pScoreProc((lmScoreProcessor*)NULL)
//{
//    for (int i=0; i < k_page_max; ++i)
//        m_ToolPagesInfo[i] = (wxClassInfo*)NULL;
//}
//
//lmEditorMode::lmEditorMode(wxString& sCreationMode, wxString& sCreationVers)
//    : m_pControllerInfo(CLASSINFO(CommandEventHandler))
//    , m_pScoreProcInfo(CLASSINFO(lmHarmonyProcessor))
//    , m_sCreationModeName(sCreationMode)
//    , m_sCreationModeVers(sCreationVers)
//    , m_pScoreProc((lmScoreProcessor*)NULL)
//{
//    for (int i=0; i < k_page_max; ++i)
//        m_ToolPagesInfo[i] = (wxClassInfo*)NULL;
//
//    m_ToolPagesInfo[k_page_notes] = CLASSINFO(ToolPageNotesHarmony);
//}
//
//lmEditorMode::~lmEditorMode()
//{
//    if (m_pScoreProc)
//        lmProcessorMngr::GetInstance()->DeleteScoreProcessor(m_pScoreProc);
//}
//
//void lmEditorMode::ChangeToolPage(int nPageID, wxClassInfo* pToolPageInfo)
//{
//	wxASSERT(nPageID > k_page_none && nPageID < k_page_max);
//    m_ToolPagesInfo[nPageID] = pToolPageInfo;
//}
//
//void lmEditorMode::CustomizeToolBoxPages(ToolBox* pToolBox)
//{
//    //create the configuration
//    for (int i=0; i < k_page_max; ++i)
//    {
//        if (m_ToolPagesInfo[i])
//        {
//            ToolPage* pPage = (ToolPage*)m_ToolPagesInfo[i]->CreateObject();
//            pPage->CreatePage(pToolBox, (lmEToolPageID)i);
//            pPage->CreateGroups();
//            pToolBox->AddPage(pPage, i);
//            pToolBox->SetAsActive(pPage, i);
//            pToolBox->SelectToolPage( pToolBox->GetCurrentPageID() );
//        }
//    }
//}
//
//lmScoreProcessor* lmEditorMode::CreateScoreProcessor()
//{
//    m_pScoreProc = (lmScoreProcessor*)NULL;
//    if (m_pScoreProcInfo)
//    {
//        //create the score processor
//        //m_pScoreProc = (lmScoreProcessor*)m_pScoreProcInfo->CreateObject();
//        lmProcessorMngr* pMngr = lmProcessorMngr::GetInstance();
//        m_pScoreProc = pMngr->CreateScoreProcessor( m_pScoreProcInfo );
//        m_pScoreProc->SetTools();
//    }
//    return m_pScoreProc;
//}




}   // namespace lenmus