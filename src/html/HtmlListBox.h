///////////////////////////////////////////////////////////////////////////////
// Name:        wx/htmllbox.h
// Purpose:     lmHtmlListBox is a listbox whose items are wxHtmlCells
// Author:      Vadim Zeitlin
// Modified by:
// Created:     31.05.03
// RCS-ID:      $Id: htmllbox.h,v 1.20 2006/10/29 23:57:10 SN Exp $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef __LM_HTMLLISTBOX_H__
#define __LM_HTMLLISTBOX_H__

#include "wx/vlbox.h"               // base class
#include "wx/html/htmlwin.h"
#include "wx/ctrlsub.h"

#include "wx/filesys.h"

class wxHtmlCell;
class wxHtmlWinParser;
class lmHtmlListBoxCache;
class lmHtmlListBoxStyle;

extern const wxString lmHtmlListBoxNameStr;

// ----------------------------------------------------------------------------
// lmHtmlListBox
// ----------------------------------------------------------------------------

class lmHtmlListBox : public wxVListBox,
                                       public wxHtmlWindowInterface,
                                       public wxHtmlWindowMouseHelper
{
    DECLARE_ABSTRACT_CLASS(lmHtmlListBox)
public:
    // constructors and such
    // ---------------------

    // default constructor, you must call Create() later
    lmHtmlListBox();

    // normal constructor which calls Create() internally
    lmHtmlListBox(wxWindow *parent,
                  wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0,
                  const wxString& name = lmHtmlListBoxNameStr);

    // really creates the control and sets the initial number of items in it
    // (which may be changed later with SetItemCount())
    //
    // the only special style which may be specified here is wxLB_MULTIPLE
    //
    // returns true on success or false if the control couldn't be created
    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = lmHtmlListBoxNameStr);

    // destructor cleans up whatever resources we use
    virtual ~lmHtmlListBox();

    // override some base class virtuals
    virtual void RefreshLine(size_t line);
    virtual void RefreshLines(size_t from, size_t to);
    virtual void RefreshAll();
    virtual void SetItemCount(size_t count);


    // retrieve the file system used by the wxHtmlWinParser: if you use
    // relative paths in your HTML, you should use its ChangePathTo() method
    wxFileSystem& GetFileSystem() { return m_filesystem; }
    const wxFileSystem& GetFileSystem() const { return m_filesystem; }

    virtual void OnInternalIdle();

    // this method must be implemented in the derived class and should return
    // the body (i.e. without <html>) of the HTML for the given item
    virtual wxString OnGetItem(size_t n) const = 0;

    // this function may be overridden to decorate HTML returned by OnGetItem()
    virtual wxString OnGetItemMarkup(size_t n) const;


    // this method allows to customize the selection appearance: it may be used
    // to specify the colour of the text which normally has the given colour
    // colFg when it is inside the selection
    //
    // by default, the original colour is not used at all and all text has the
    // same (default for this system) colour inside selection
    virtual wxColour GetSelectedTextColour(const wxColour& colFg) const;

    // this is the same as GetSelectedTextColour() but allows to customize the
    // background colour -- this is even more rarely used as you can change it
    // globally using SetSelectionBackground()
    virtual wxColour GetSelectedTextBgColour(const wxColour& colBg) const;


    // we implement both of these functions in terms of OnGetItem(), they are
    // not supposed to be overridden by our descendants
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
    virtual wxCoord OnMeasureItem(size_t n) const;

    // This method may be overriden to handle clicking on a link in
    // the listbox. By default, clicking links is ignored.
    virtual void OnLinkClicked(size_t n, const wxHtmlLinkInfo& link);

    // event handlers
    void OnSize(wxSizeEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);


    // common part of all ctors
    void Init();

    // ensure that the given item is cached
    void CacheItem(size_t n) const;

private:
    // wxHtmlWindowInterface methods:
    virtual void SetHTMLWindowTitle(const wxString& title);
    virtual void OnHTMLLinkClicked(const wxHtmlLinkInfo& link);
    virtual wxHtmlOpeningStatus OnHTMLOpeningURL(wxHtmlURLType type,
                                                 const wxString& url,
                                                 wxString *redirect) const;
    virtual wxPoint HTMLCoordsToWindow(wxHtmlCell *cell,
                                       const wxPoint& pos) const;
    virtual wxWindow* GetHTMLWindow();
    virtual wxColour GetHTMLBackgroundColour() const;
    virtual void SetHTMLBackgroundColour(const wxColour& clr);
    virtual void SetHTMLBackgroundImage(const wxBitmap& bmpBg);
    virtual void SetHTMLStatusText(const wxString& text);
    virtual wxCursor GetHTMLCursor(HTMLCursor type) const;

    // returns index of item that contains given HTML cell
    size_t GetItemForCell(const wxHtmlCell *cell) const;

    // return physical coordinates of root wxHtmlCell of n-th item
    wxPoint GetRootCellCoords(size_t n) const;

    // Converts physical coordinates stored in @a pos into coordinates
    // relative to the root cell of the item under mouse cursor, if any. If no
    // cell is found under the cursor, returns false.  Otherwise stores the new
    // coordinates back into @a pos and pointer to the cell under cursor into
    // @a cell and returns true.
    bool PhysicalCoordsToCell(wxPoint& pos, wxHtmlCell*& cell) const;

    // The opposite of PhysicalCoordsToCell: converts coordinates relative to
    // given cell to physical coordinates in the window
    wxPoint CellCoordsToPhysical(const wxPoint& pos, wxHtmlCell *cell) const;

private:
    // this class caches the pre-parsed HTML to speed up display
    lmHtmlListBoxCache *m_cache;

    // HTML parser we use
    wxHtmlWinParser *m_htmlParser;

    // file system used by m_htmlParser
    wxFileSystem m_filesystem;

    // rendering style for the parser which allows us to customize our colours
    lmHtmlListBoxStyle *m_htmlRendStyle;


    // it calls our GetSelectedTextColour() and GetSelectedTextBgColour()
    friend class wxHtmlListBoxStyle;
    friend class wxHtmlListBoxWinInterface;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(lmHtmlListBox)
};



#endif // __LM_HTMLLISTBOX_H__

