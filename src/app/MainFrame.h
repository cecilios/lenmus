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

#ifndef __LM_MAINFRAME_H__
#define __LM_MAINFRAME_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "MainFrame.cpp"
#endif

#include <map>

#include "wx/mdi.h"
#include "wx/docview.h"
#include "wx/docmdi.h"
#include "global.h"
#include "ScoreCanvas.h"

#include "wx/wizard.h"          //to use wxWizard classes
#include "wx/timer.h"           //to use wxTimer
#include "wx/spinctrl.h"        //to use spin control
#include "wx/html/htmlwin.h"    //to use html

#include "global.h"


#include "wx/numdlg.h"

//wxAUI
#include "wx/aui/aui.h"     //"manager.h"

#include "../mdi/DocViewMDI.h"

class lmTextBookController;
class lmHtmlWindow;
class lmHelpController;
class lmMetronome;
class lmTDIChildFrame;
class lmStatusBar;
class lmToolBox;
class lmWelcomeWnd;
class lmDocument;
class lmEditorMode;


// Class lmMainFrame defines the main MDI frame for the application
//--------------------------------------------------------------------
class lmMainFrame : public lmDocTDIParentFrame
{
    DECLARE_DYNAMIC_CLASS(lmMainFrame)

public:
    wxLocale*   m_pLocale;            // locale for internationalization

    //constructors and destructor
    lmMainFrame(lmDocManager* pDocManager, wxFrame* pFrame, const wxString& sTitle,
            const wxPoint& pos, const wxSize& size, long type);
    ~lmMainFrame();

    void CreateControls();

    // tool bars
    void CreateMyToolBar();
    void DeleteToolbar();
    void CreateTextBooksToolBar(long style, wxSize nIconSize, int nRow);

    //status bar
    void SetStatusBarMsg(const wxString& sText);
    void SetStatusBarMouseData(int nPage, float rTime, int nMeasure, lmUPoint uPos);
    void SetStatusBarCaretData(int nPage, float rTime, int nMeasure);

    //ToolBox
	inline lmToolBox* GetActiveToolBox() { return m_pToolBox; }
	bool IsToolBoxVisible();
	void ShowToolBox(bool fShow);

    // menu bar
    wxMenuBar* CreateMenuBar(wxDocument* doc, wxView* view);

    // metronome
    void SetMetronome(lmMetronome* pMtr);
    lmMetronome* GetMetronome() { return m_pMtr; }

    // File menu events
    void OnFileOpen(wxCommandEvent& event);
    void OnFileClose(wxCommandEvent& event);
    void OnFileSave(wxCommandEvent& event);
    void OnFileSaveAs(wxCommandEvent& event);
    void OnScoreWizard(wxCommandEvent& WXUNUSED(event));
    void OnFileImport(wxCommandEvent& WXUNUSED(event));
	void OnExportMusicXML(wxCommandEvent& WXUNUSED(event));
	void OnExportBMP(wxCommandEvent& WXUNUSED(event));
    void OnExportJPG(wxCommandEvent& WXUNUSED(event));
    void OnPrintPreview(wxCommandEvent& WXUNUSED(event));
    //void OnPageSetup(wxCommandEvent& WXUNUSED(event));
    void OnPrintSetup(wxCommandEvent& WXUNUSED(event));
    void OnPrint(wxCommandEvent& event);
    void OnFileUpdateUI(wxUpdateUIEvent& event);
    void OnOpenRecentFile(wxCommandEvent& event);

    void ExportAsImage(int nImgType);


    // Edit menu events
    void OnEditCut(wxCommandEvent& event);
    void OnEditCopy(wxCommandEvent& event);
    void OnEditPaste(wxCommandEvent& event);
    void OnEditUpdateUI(wxUpdateUIEvent& event);

	// Score Menu events
	void OnScoreTitles(wxCommandEvent& WXUNUSED(event));

	// Instrument menu events
	void OnInstrumentProperties(wxCommandEvent& WXUNUSED(event));

    // Debug menu events
#ifdef __WXDEBUG__
    void OnDebugForceReleaseBehaviour(wxCommandEvent& event);
    void OnDebugShowDebugLinks(wxCommandEvent& event);
    void OnDebugShowBorderOnScores(wxCommandEvent& event);
    void OnDebugRecSelec(wxCommandEvent& event);
    void OnDebugDrawBounds(wxCommandEvent& event);
    void OnDebugDrawAnchors(wxCommandEvent& event);
    void OnDebugTestMidi(wxCommandEvent& event);
    void OnDebugSetTraceLevel(wxCommandEvent& WXUNUSED(event));
    void OnDebugPatternEditor(wxCommandEvent& WXUNUSED(event));
    void OnDebugUnitTests(wxCommandEvent& event);
    void OnDebugShowDirtyObjects(wxCommandEvent& event);
        // methods requiring a score
    void OnDebugCheckHarmony(wxCommandEvent& WXUNUSED(event));
    void OnDebugDumpBitmaps(wxCommandEvent& event);
    void OnDebugDumpGMObjects(wxCommandEvent& event);
    void OnDebugDumpStaffObjs(wxCommandEvent& event);
    void OnDebugSeeMidiEvents(wxCommandEvent& WXUNUSED(event));
    void OnDebugSeeSource(wxCommandEvent& event);
    void OnDebugSeeSourceForUndo(wxCommandEvent& event);
    void OnDebugSeeXML(wxCommandEvent& event);
    void OnDebugTestProcessor(wxCommandEvent& WXUNUSED(event));
    void OnDebugScoreUI(wxUpdateUIEvent& event);
#endif

    // Zoom events
    void OnComboZoom(wxCommandEvent& event);
    void OnZoom(wxCommandEvent& event, double rScale);
    void OnZoom100(wxCommandEvent& event) { OnZoom(event, 1.0); }
    void OnZoomFitWidth(wxCommandEvent& event);
    void OnZoomFitFull(wxCommandEvent& event);
    void OnZoomOther(wxCommandEvent& event);
    void OnZoomIncrease(wxCommandEvent& event);
    void OnZoomDecrease(wxCommandEvent& event);
    void OnZoomUpdateUI(wxUpdateUIEvent& event);

    //zoom related
    void UpdateZoomControls(double rScale);

    // View menu events
    void OnViewTools(wxCommandEvent& event);
    void OnViewRulers(wxCommandEvent& event);
    void OnViewRulersUI(wxUpdateUIEvent& event);
    void OnViewToolBar(wxCommandEvent& WXUNUSED(event));
    void OnViewStatusBar(wxCommandEvent& WXUNUSED(event));
    void OnToolbarsUI(wxUpdateUIEvent& event);
    void OnStatusbarUI(wxUpdateUIEvent& event);
    void OnViewPageMargins(wxCommandEvent& event);
    void OnViewWelcomePage(wxCommandEvent& event);
    void OnViewWelcomePageUI(wxUpdateUIEvent& event);

    // Sound menu events
    void OnSoundUpdateUI(wxUpdateUIEvent& event);
    void OnSoundTest(wxCommandEvent& WXUNUSED(event));
    void OnAllSoundsOff(wxCommandEvent& WXUNUSED(event));
    void OnRunMidiWizard(wxCommandEvent& WXUNUSED(event));
    void DoRunMidiWizard();
    void OnPlayStart(wxCommandEvent& WXUNUSED(event));
    void OnPlayCursorStart(wxCommandEvent& WXUNUSED(event));
    void OnPlayStop(wxCommandEvent& WXUNUSED(event));
    void OnPlayPause(wxCommandEvent& WXUNUSED(event));

    //Window menu events
    void OnWindowClose(wxCommandEvent& WXUNUSED(event));
    void OnWindowCloseAll(wxCommandEvent& WXUNUSED(event));
    void OnWindowNext(wxCommandEvent& WXUNUSED(event));
    void OnWindowPrev(wxCommandEvent& WXUNUSED(event));

    // Voice events
    void OnComboVoice(wxCommandEvent& event);

    //Help menu
    void OnAbout(wxCommandEvent& WXUNUSED(event));
    void OnHelpQuickGuide(wxCommandEvent& WXUNUSED(event));
    void OnHelpOpen(wxCommandEvent& event);
    void OnCheckForUpdates(wxCommandEvent& WXUNUSED(event));
    void OnVisitWebsite(wxCommandEvent& WXUNUSED(event));

    // Other menu items events
    void OnOptions(wxCommandEvent& WXUNUSED(event));
    void OnOpenBook(wxCommandEvent& event);
    void OnOpenBookUI(wxUpdateUIEvent& event);

    //other even managers
    void OnMetronomeTimer(wxTimerEvent& event);
    void OnMetronomeOnOff(wxCommandEvent& WXUNUSED(event));
    void OnMetronomeUpdate(wxSpinEvent& WXUNUSED(event));
    void OnMetronomeUpdateText(wxCommandEvent& WXUNUSED(event));
    void OnPaneClose(wxAuiManagerEvent& event);
    void OnKeyPress(wxKeyEvent& event);
	void OnKeyF1(wxCommandEvent& event);


    //textbook events and methods
    void OnBookFrame(wxCommandEvent& event);
    void OnBookFrameUpdateUI(wxUpdateUIEvent& event);
    void OnCloseBookFrame();

    //other events
    void OnCloseWindow(wxCloseEvent& event);

    // other methods
    void SetOpenHelpButton(bool fButtonPressed);
    void UpdateToolbarsLayout();
    void SilentlyCheckForUpdates(bool fSilent);
	inline wxMenu* GetEditMenu() {return m_pMenuEdit; }
    void OpenRecentFile(wxString sFile);      //call back from WelcomeDlg
    void NewScoreWindow(lmEditorMode* pMode, lmScore* pScore);

    //options
    bool ShowRulers();
    bool IsCountOffChecked();

    //welcome window
    void ShowWelcomeWindow();
    void OnCloseWelcomeWnd();

    //eBooks controller
    void SetHtmlWindow(lmHtmlWindow* pHtmlWin) { m_pHtmlWin = pHtmlWin; }
    lmHtmlWindow* GetHtmlWindow() { return m_pHtmlWin; }

    //debug method
    void DumpScore(lmScore* pScore);

	//access to information
    inline lmTextBookController* GetBookController() { return m_pBookController; }
    lmController* GetActiveController();
    wxFileHistory* GetFileHistory() { return m_pRecentFiles; }
    lmDocument* GetActiveDoc();

	// call backs
	void OnActiveChildChanged(lmTDIChildFrame* pFrame);

	//other
	void RedirectKeyPressEvent(wxKeyEvent& event);
    void SetFocusOnActiveView();
    void AddFileToHistory(const wxString& filename);

    //access to current active MDI Child
    lmScoreView* GetActiveScoreView();
    lmScore* GetActiveScore();




protected:
    void InitializeHelp();
    void InitializeBooks();
    void ScanForBooks(wxString sPath, wxString sPattern);
    void LoadRecentFiles();
    void SaveRecentFiles();

    //menu bar
    void AddMenuItem(wxMenu* pMenu, int nId, const wxString& sItemName,
                     const wxString& sToolTip = _T(""),
                     wxItemKind nKind = wxITEM_NORMAL,
                     const wxString& sIconName = _T("empty") );

    //status bar
    void CreateTheStatusBar(int nType=0);
    void DeleteTheStatusBar();



    //controllers, special windows, and other controls
    wxAuiManager            m_mgrAUI;           // wxAUI manager
    lmToolBox*              m_pToolBox;         //tool box window
    lmWelcomeWnd*           m_pWelcomeWnd;      //welcome window
    lmTextBookController*   m_pBookController;
    lmHtmlWindow*           m_pHtmlWin;
    lmHelpController*       m_pHelp;
    wxSpinCtrl*             m_pSpinMetronome;
    wxComboBox*             m_pComboZoom;

    lmMetronome*        m_pMainMtr;        //independent metronome
    lmMetronome*        m_pMtr;            //metronome currently associated to frame metronome controls

    //flags for toggle buttons/menus
    bool m_fHelpOpened;

	// menus
	wxMenu*			m_pMenuEdit;
    wxMenu*         m_pMenuBooks;

    // tool bars
    wxToolBar*      m_pTbFile;          // file toolbar
    wxToolBar*      m_pTbEdit;          // edit toolbar
    wxToolBar*      m_pTbZoom;          // zoom toolbar
    wxToolBar*      m_pTbPlay;          // play toolbar
    wxToolBar*      m_pTbMtr;           // metronome toolbar
    wxToolBar*      m_pToolbar;         // main toolbar
    wxToolBar*      m_pTbTextBooks;     // text books navigation toolbar

    // status bar
    lmStatusBar*    m_pStatusBar;

    bool    m_fSilentCheck;
    bool    m_fClosingAll;

    //other
    wxFileHistory*      m_pRecentFiles;     //list of rencently open files 

    DECLARE_EVENT_TABLE()
};


#endif    // __LM_MAINFRAME_H__
