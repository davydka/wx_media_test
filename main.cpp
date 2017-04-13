// To use this sample, simply select Open File from the file menu,
// select the file you want to play - and MediaPlayer will play the file in a
// the current notebook page, showing video if necessary.
//
// 1) Certain backends can't play the same media file at the same time (MCI,
//    Cocoa NSMovieView-Quicktime).
// 2) Positioning on Mac Carbon is messed up if put in a sub-control like a
//    Notebook (like this sample does).
//
// ----------------------------------------------------------------------------
// Pre-compiled header stuff
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------

#include "wx/mediactrl.h"   // for wxMediaCtrl
#include "wx/filedlg.h"     // for opening files from OpenFile
#include "wx/slider.h"      // for a slider for seeking within media
#include "wx/sizer.h"       // for positioning controls/wxBoxSizer
#include "wx/timer.h"       // timer for updating status bar
#include "wx/textdlg.h"     // for getting user text from OpenURL/Debug
#include "wx/notebook.h"    // for wxNotebook and putting movies in pages
#include "wx/cmdline.h"     // for wxCmdLineParser (optional)
#include "wx/listctrl.h"    // for wxListCtrl
#include "wx/dnd.h"         // drag and drop for the playlist
#include "wx/filename.h"    // For wxFileName::GetName()
#include "wx/config.h"      // for native wxConfig
#include "wx/vector.h"

// Under MSW we have several different backends but when linking statically
// they may be discarded by the linker (this definitely happens with MSVC) so
// force linking them. You don't have to do this in your code if you don't plan
// to use them, of course.
#if defined(__WXMSW__) && !defined(WXUSINGDLL)
    #include "wx/link.h"
    wxFORCE_LINK_MODULE(wxmediabackend_am)
    wxFORCE_LINK_MODULE(wxmediabackend_qt)
    wxFORCE_LINK_MODULE(wxmediabackend_wmp10)
#endif // static wxMSW build

#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "./logo.xpm"
#endif

// ----------------------------------------------------------------------------
// Bail out if the user doesn't want one of the
// things we need
// ----------------------------------------------------------------------------

#if !wxUSE_MEDIACTRL || !wxUSE_MENUS || !wxUSE_SLIDER || !wxUSE_TIMER || \
    !wxUSE_NOTEBOOK || !wxUSE_LISTCTRL
#error "Not all required elements are enabled.  Please modify setup.h!"
#endif

// ============================================================================
// Declarations
// ============================================================================

// ----------------------------------------------------------------------------
// Enumurations
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // Menu event IDs
    wxID_OPENFILESAMEPAGE,
    wxID_CLOSECURRENTPAGE,
    wxID_PLAY,
    wxID_PAUSE,
    wxID_NEXT,
    wxID_PREV,
    // Control event IDs
    wxID_NOTEBOOK,
    wxID_MEDIACTRL,
    wxID_LISTCTRL,
};

// ----------------------------------------------------------------------------
// wxMediaPlayerApp
// ----------------------------------------------------------------------------

class wxMediaPlayerApp : public wxApp
{
public:
#ifdef __WXMAC__
    virtual void MacOpenFiles(const wxArrayString & fileNames );
#endif

#if wxUSE_CMDLINE_PARSER
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

    // Files specified on the command line, if any.
    wxVector<wxString> m_params;
#endif // wxUSE_CMDLINE_PARSER

    virtual bool OnInit();

protected:
    class wxMediaPlayerFrame* m_frame;
};

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame
// ----------------------------------------------------------------------------

class wxMediaPlayerFrame : public wxFrame
{
public:
    // Ctor/Dtor
    wxMediaPlayerFrame(const wxString& title);
    ~wxMediaPlayerFrame();

    // Menu event handlers
    void OnQuit(wxCommandEvent& event);

    void OnOpenFileSamePage(wxCommandEvent& event);
    void OnCloseCurrentPage(wxCommandEvent& event);

    void OnPlay(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnPrev(wxCommandEvent& event);

    // Key event handlers
    void OnKeyDown(wxKeyEvent& event);

    // Quickie for playing from command line
    void AddToPlayList(const wxString& szString);

    // Media event handlers
    void OnMediaLoaded(wxMediaEvent& event);

    // Close event handlers
    void OnClose(wxCloseEvent& event);

private:
    // Common open file code
    void OpenFile(bool bNewPage);
    void DoOpenFile(const wxString& path, bool bNewPage);
    void DoPlayFile(const wxString& path);

    wxNotebook* m_notebook;     // Notebook containing our pages

    // Maybe I should use more accessors, but for simplicity
    // I'll allow the other classes access to our members
    friend class wxMediaPlayerApp;
    friend class wxMediaPlayerNotebookPage;
};



// ----------------------------------------------------------------------------
// wxMediaPlayerNotebookPage
// ----------------------------------------------------------------------------

class wxMediaPlayerNotebookPage : public wxPanel
{
    wxMediaPlayerNotebookPage(wxMediaPlayerFrame* parentFrame,
        wxNotebook* book, const wxString& be = wxEmptyString);

    // Media event handlers
    void OnMediaPlay(wxMediaEvent& event);
    void OnMediaPause(wxMediaEvent& event);
    void OnMediaFinished(wxMediaEvent& event);

public:
    bool IsBeingDragged();      // accessor for m_bIsBeingDragged

    // make wxMediaPlayerFrame able to access the private members
    friend class wxMediaPlayerFrame;

    int      m_nLastFileId;     // List ID of played file in listctrl
    wxString m_szFile;          // Name of currently playing file/location

    wxMediaCtrl* m_mediactrl;   // Our media control
    class wxMediaPlayerListCtrl* m_playlist;  // Our playlist
    int m_nLoops;               // Number of times media has looped
    bool m_bLoop;               // Whether we are looping or not
    bool m_bIsBeingDragged;     // Whether the user is dragging the scroll bar
    wxMediaPlayerFrame* m_parentFrame;  // Main wxFrame of our sample
    wxButton* m_prevButton;     // Go to previous file button
    wxButton* m_playButton;     // Play/pause file button
    wxButton* m_nextButton;     // Next file button
};

// ----------------------------------------------------------------------------
// wxMediaPlayerListCtrl
// ----------------------------------------------------------------------------
class wxMediaPlayerListCtrl : public wxListCtrl
{
public:
    void AddToPlayList(const wxString& szString)
    {
        wxListItem kNewItem;
        kNewItem.SetAlign(wxLIST_FORMAT_LEFT);

        int nID = this->GetItemCount();
        kNewItem.SetId(nID);
        kNewItem.SetMask(wxLIST_MASK_DATA);
        kNewItem.SetData(new wxString(szString));

        this->InsertItem(kNewItem);
        this->SetItem(nID, 0, wxT("*"));
        this->SetItem(nID, 1, wxFileName(szString).GetName());

        if (nID % 2)
        {
            kNewItem.SetBackgroundColour(wxColour(192,192,192));
            this->SetItem(kNewItem);
        }
    }

    void GetSelectedItem(wxListItem& listitem)
    {
        listitem.SetMask(wxLIST_MASK_TEXT |  wxLIST_MASK_DATA);
        int nLast = -1, nLastSelected = -1;
        while ((nLast = this->GetNextItem(nLast,
                                         wxLIST_NEXT_ALL,
                                         wxLIST_STATE_SELECTED)) != -1)
        {
            listitem.SetId(nLast);
            this->GetItem(listitem);
            if ((listitem.GetState() & wxLIST_STATE_FOCUSED) )
                break;
            nLastSelected = nLast;
        }
        if (nLast == -1 && nLastSelected == -1)
            return;
        listitem.SetId(nLastSelected == -1 ? nLast : nLastSelected);
        this->GetItem(listitem);
    }
};


// ============================================================================
//
// Implementation
//
// ============================================================================

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// [Functions]
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ----------------------------------------------------------------------------
// wxGetMediaStateText
//
// Converts a wxMediaCtrl state into something useful that we can display
// to the user
// ----------------------------------------------------------------------------
const wxChar* wxGetMediaStateText(int nState)
{
    switch(nState)
    {
        case wxMEDIASTATE_PLAYING:
            return wxT("Playing");
        ///case wxMEDIASTATE_PAUSED:
        default:
            return wxT("Paused");
    }
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// wxMediaPlayerApp
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ----------------------------------------------------------------------------
// This sets up this wxApp as the global wxApp that gui calls in wxWidgets
// use.  For example, if you were to be in windows and use a file dialog,
// wxWidgets would use wxTheApp->GetHInstance() which would get the instance
// handle of the application.  These routines in wx _DO NOT_ check to see if
// the wxApp exists, and thus will crash the application if you try it.
//
// IMPLEMENT_APP does this, and also implements the platform-specific entry
// routine, such as main or WinMain().  Use IMPLEMENT_APP_NO_MAIN if you do
// not desire this behaviour.
// ----------------------------------------------------------------------------
IMPLEMENT_APP(wxMediaPlayerApp)

// ----------------------------------------------------------------------------
// wxMediaPlayerApp command line parsing
// ----------------------------------------------------------------------------

#if wxUSE_CMDLINE_PARSER

void wxMediaPlayerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);

    parser.AddParam("input files",
                    wxCMD_LINE_VAL_STRING,
                    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
}

bool wxMediaPlayerApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if ( !wxApp::OnCmdLineParsed(parser) )
        return false;

    for (size_t paramNr=0; paramNr < parser.GetParamCount(); ++paramNr)
        m_params.push_back(parser.GetParam(paramNr));

    return true;
}

#endif // wxUSE_CMDLINE_PARSER

// ----------------------------------------------------------------------------
// wxMediaPlayerApp::OnInit
//
// Where execution starts - akin to a main or WinMain.
// 1) Create the frame and show it to the user
// 2) Process filenames from the commandline
// 3) return true specifying that we want execution to continue past OnInit
// ----------------------------------------------------------------------------
bool wxMediaPlayerApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    // SetAppName() lets wxConfig and others know where to write
    SetAppName(wxT("wxMediaPlayer"));

    wxMediaPlayerFrame *frame =
        new wxMediaPlayerFrame(wxT("media"));
    frame->Show(true);

#if wxUSE_CMDLINE_PARSER
    if ( !m_params.empty() )
    {
        for ( size_t n = 0; n < m_params.size(); n++ )
            frame->AddToPlayList(m_params[n]);

        wxCommandEvent theEvent(wxEVT_MENU, wxID_NEXT);
        frame->AddPendingEvent(theEvent);
    }
#endif // wxUSE_CMDLINE_PARSER

    return true;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// wxMediaPlayerFrame
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame Constructor
//
// 1) Create our menus
// 2) Create our notebook control and add it to the frame
// 3) Create our status bar
// 4) Connect our events
// 5) Start our timer
// ----------------------------------------------------------------------------

wxMediaPlayerFrame::wxMediaPlayerFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600,600))
{
    SetIcon(wxICON(sample));

    //
    // Create our notebook - using wxNotebook is luckily pretty
    // simple and self-explanatory in most cases
    //
    m_notebook = new wxNotebook(this, wxID_NOTEBOOK);

    this->Connect(wxID_NEXT, wxEVT_MENU,
                  wxCommandEventHandler(wxMediaPlayerFrame::OnNext));

    //
    // Close events
    //
    this->Connect(wxID_ANY, wxEVT_CLOSE_WINDOW,
                wxCloseEventHandler(wxMediaPlayerFrame::OnClose));

    //
    // End of Events
    //

    //
    //  Create an initial notebook page so the user has something
    //  to work with without having to go file->open every time :).
    //
    wxMediaPlayerNotebookPage* page =
        new wxMediaPlayerNotebookPage(this, m_notebook);
    m_notebook->AddPage(page,
                        wxT(""),
                        true);

}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame Destructor
//
// 1) Deletes child objects implicitly
// 2) Delete our timer explicitly
// ----------------------------------------------------------------------------
wxMediaPlayerFrame::~wxMediaPlayerFrame()
{

    //
    //  Here we save our info to the registry or whatever
    //  mechanism the OS uses.
    //
    //  This makes it so that when mediaplayer loads up again
    //  it restores the same files that were in the playlist
    //  this time, rather than the user manually re-adding them.
    //
    //  We need to do conf->DeleteAll() here because by default
    //  the config still contains the same files as last time
    //  so we need to clear it before writing our new ones.
    //
    //  TODO:  Maybe you could add a menu option to the
    //  options menu to delete the configuration on exit -
    //  all you'd need to do is just remove everything after
    //  conf->DeleteAll() here
    //
    //  As an exercise to the reader, try modifying this so
    //  that it saves the data for each notebook page
    //
    wxMediaPlayerListCtrl* playlist =
        ((wxMediaPlayerNotebookPage*)m_notebook->GetPage(0))->m_playlist;

    wxConfig conf;
    conf.DeleteAll();

    for(int i = 0; i < playlist->GetItemCount(); ++i)
    {
        wxString* pData = (wxString*) playlist->GetItemData(i);
        wxString s;
        s << i;
        conf.Write(s, *(pData));
        delete pData;
    }
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnClose
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnClose(wxCloseEvent& event)
{
    event.Skip(); // really close the frame
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::AddToPlayList
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::AddToPlayList(const wxString& szString)
{
    wxMediaPlayerNotebookPage* currentpage =
        ((wxMediaPlayerNotebookPage*)m_notebook->GetCurrentPage());

    currentpage->m_playlist->AddToPlayList(szString);
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnQuit
//
// Called from file->quit.
// Closes this application.
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnOpenFileSamePage
//
// Called from file->openfile.
// Opens and plays a media file in the current notebook page
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnOpenFileSamePage(wxCommandEvent& WXUNUSED(event))
{
    OpenFile(false);
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OpenFile
//
// Opens a file dialog asking the user for a filename, then
// calls DoOpenFile which will add the file to the playlist and play it
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OpenFile(bool bNewPage)
{
    wxFileDialog fd(this);

    if(fd.ShowModal() == wxID_OK)
    {
        DoOpenFile(fd.GetPath(), bNewPage);
    }
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::DoOpenFile
//
// Adds the file to our playlist, selects it in the playlist,
// and then calls DoPlayFile to play it
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::DoOpenFile(const wxString& path, bool bNewPage)
{
    if(bNewPage)
    {
        m_notebook->AddPage(
            new wxMediaPlayerNotebookPage(this, m_notebook),
            path,
            true);
    }

    wxMediaPlayerNotebookPage* currentpage =
        (wxMediaPlayerNotebookPage*) m_notebook->GetCurrentPage();

    if(currentpage->m_nLastFileId != -1)
        currentpage->m_playlist->SetItemState(currentpage->m_nLastFileId,
                                              0, wxLIST_STATE_SELECTED);

    wxListItem newlistitem;
    newlistitem.SetAlign(wxLIST_FORMAT_LEFT);

    int nID;

    newlistitem.SetId(nID = currentpage->m_playlist->GetItemCount());
    newlistitem.SetMask(wxLIST_MASK_DATA | wxLIST_MASK_STATE);
    newlistitem.SetState(wxLIST_STATE_SELECTED);
    newlistitem.SetData(new wxString(path));

    currentpage->m_playlist->InsertItem(newlistitem);
    currentpage->m_playlist->SetItem(nID, 0, wxT("*"));
    currentpage->m_playlist->SetItem(nID, 1, wxFileName(path).GetName());

    if (nID % 2)
    {
        newlistitem.SetBackgroundColour(wxColour(192,192,192));
        currentpage->m_playlist->SetItem(newlistitem);
    }

    DoPlayFile(path);
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::DoPlayFile
//
// Pauses the file if its the currently playing file,
// otherwise it plays the file
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::DoPlayFile(const wxString& path)
{
    wxMediaPlayerNotebookPage* currentpage =
        (wxMediaPlayerNotebookPage*) m_notebook->GetCurrentPage();

    wxListItem listitem;
    currentpage->m_playlist->GetSelectedItem(listitem);

    if( (  listitem.GetData() &&
           currentpage->m_nLastFileId == listitem.GetId() &&
           currentpage->m_szFile.compare(path) == 0 ) ||
        (  !listitem.GetData() &&
            currentpage->m_nLastFileId != -1 &&
            currentpage->m_szFile.compare(path) == 0)
      )
    {
        if(currentpage->m_mediactrl->GetState() == wxMEDIASTATE_PLAYING)
        {
            if( !currentpage->m_mediactrl->Pause() )
                wxMessageBox(wxT("Couldn't pause movie!"));
        }
        else
        {
            if( !currentpage->m_mediactrl->Play() )
                wxMessageBox(wxT("Couldn't play movie!"));
        }
    }
    else
    {
        int nNewId = listitem.GetData() ? listitem.GetId() :
                            currentpage->m_playlist->GetItemCount()-1;
        m_notebook->SetPageText(m_notebook->GetSelection(),
                                wxFileName(path).GetName());

        if(currentpage->m_nLastFileId != -1)
           currentpage->m_playlist->SetItem(
                    currentpage->m_nLastFileId, 0, wxT("*"));

        wxURI uripath(path);
        if( uripath.IsReference() )
        {
            if( !currentpage->m_mediactrl->Load(path) )
            {
                wxMessageBox(wxT("Couldn't load file!"));
                currentpage->m_playlist->SetItem(nNewId, 0, wxT("E"));
            }
            else
            {
                currentpage->m_playlist->SetItem(nNewId, 0, wxT("O"));
            }
        }
        else
        {
			currentpage->m_playlist->SetItem(nNewId, 0, wxT("O"));
        }

        currentpage->m_nLastFileId = nNewId;
        currentpage->m_szFile = path;
        currentpage->m_playlist->SetItem(currentpage->m_nLastFileId,
                                         1, wxFileName(path).GetName());
        currentpage->m_playlist->SetItem(currentpage->m_nLastFileId,
                                         2, wxT(""));
    }
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnMediaLoaded
//
// Called when the media is ready to be played - and does
// so, also gets the length of media and shows that in the list control
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnMediaLoaded(wxMediaEvent& WXUNUSED(evt))
{
    wxMediaPlayerNotebookPage* currentpage =
        (wxMediaPlayerNotebookPage*) m_notebook->GetCurrentPage();

    if( !currentpage->m_mediactrl->Play() )
    {
            wxMessageBox(wxT("Couldn't play movie!"));
        currentpage->m_playlist->SetItem(currentpage->m_nLastFileId, 0, wxT("E"));
    }
    else
    {
		currentpage->m_mediactrl->SetVolume(0.0);
        currentpage->m_playlist->SetItem(currentpage->m_nLastFileId, 0, wxT(">"));
    }

}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnCloseCurrentPage
//
// Called when the user wants to close the current notebook page
//
// 1) Get the current page number (wxControl::GetSelection)
// 2) If there is no current page, break out
// 3) Delete the current page
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnCloseCurrentPage(wxCommandEvent& WXUNUSED(event))
{
    if( m_notebook->GetPageCount() > 1 )
    {
    int sel = m_notebook->GetSelection();

    if (sel != wxNOT_FOUND)
    {
        m_notebook->DeletePage(sel);
    }
    }
    else
    {
        wxMessageBox(wxT("Cannot close main page"));
    }
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnPlay
//
// Called from file->play.
// Resumes the media if it is paused or stopped.
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnPlay(wxCommandEvent& WXUNUSED(event))
{
    wxMediaPlayerNotebookPage* currentpage =
        (wxMediaPlayerNotebookPage*) m_notebook->GetCurrentPage();

    wxListItem listitem;
    currentpage->m_playlist->GetSelectedItem(listitem);
    if ( !listitem.GetData() )
    {
        int nLast = -1;
        if ((nLast = currentpage->m_playlist->GetNextItem(nLast,
                                         wxLIST_NEXT_ALL,
                                         wxLIST_STATE_DONTCARE)) == -1)
        {
            // no items in list
            wxMessageBox(wxT("No items in playlist!"));
    }
        else
        {
        listitem.SetId(nLast);
            currentpage->m_playlist->GetItem(listitem);
        listitem.SetMask(listitem.GetMask() | wxLIST_MASK_STATE);
        listitem.SetState(listitem.GetState() | wxLIST_STATE_SELECTED);
            currentpage->m_playlist->SetItem(listitem);
            wxASSERT(listitem.GetData());
            DoPlayFile((*((wxString*) listitem.GetData())));
    }
    }
    else
    {
        wxASSERT(listitem.GetData());
        DoPlayFile((*((wxString*) listitem.GetData())));
    }
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnKeyDown
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnKeyDown(wxKeyEvent& event)
{
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnPrev
//
// Tedious wxListCtrl stuff.  Goes to prevous song in list, or if at the
// beginning goes to the last in the list.
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnPrev(wxCommandEvent& WXUNUSED(event))
{
    wxMediaPlayerNotebookPage* currentpage =
        (wxMediaPlayerNotebookPage*) m_notebook->GetCurrentPage();

    if (currentpage->m_playlist->GetItemCount() == 0)
        return;

    wxInt32 nLastSelectedItem = -1;
    while(true)
    {
        wxInt32 nSelectedItem = currentpage->m_playlist->GetNextItem(nLastSelectedItem,
                                                     wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (nSelectedItem == -1)
            break;
        nLastSelectedItem = nSelectedItem;
        currentpage->m_playlist->SetItemState(nSelectedItem, 0, wxLIST_STATE_SELECTED);
    }

    if (nLastSelectedItem == -1)
    {
        // nothing selected, default to the file before the currently playing one
        if(currentpage->m_nLastFileId == 0)
            nLastSelectedItem = currentpage->m_playlist->GetItemCount() - 1;
    else
            nLastSelectedItem = currentpage->m_nLastFileId - 1;
    }
    else if (nLastSelectedItem == 0)
        nLastSelectedItem = currentpage->m_playlist->GetItemCount() - 1;
    else
        nLastSelectedItem -= 1;

    if(nLastSelectedItem == currentpage->m_nLastFileId)
        return; // already playing... nothing to do

    wxListItem listitem;
    listitem.SetId(nLastSelectedItem);
    listitem.SetMask(wxLIST_MASK_TEXT |  wxLIST_MASK_DATA);
    currentpage->m_playlist->GetItem(listitem);
    listitem.SetMask(listitem.GetMask() | wxLIST_MASK_STATE);
    listitem.SetState(listitem.GetState() | wxLIST_STATE_SELECTED);
    currentpage->m_playlist->SetItem(listitem);

    wxASSERT(listitem.GetData());
    DoPlayFile((*((wxString*) listitem.GetData())));
}

// ----------------------------------------------------------------------------
// wxMediaPlayerFrame::OnNext
//
// Tedious wxListCtrl stuff.  Goes to next song in list, or if at the
// end goes to the first in the list.
// ----------------------------------------------------------------------------
void wxMediaPlayerFrame::OnNext(wxCommandEvent& WXUNUSED(event))
{
    wxMediaPlayerNotebookPage* currentpage =
        (wxMediaPlayerNotebookPage*) m_notebook->GetCurrentPage();

    if (currentpage->m_playlist->GetItemCount() == 0)
        return;

    wxInt32 nLastSelectedItem = -1;
    while(true)
    {
        wxInt32 nSelectedItem = currentpage->m_playlist->GetNextItem(nLastSelectedItem,
                                                     wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (nSelectedItem == -1)
            break;
        nLastSelectedItem = nSelectedItem;
        currentpage->m_playlist->SetItemState(nSelectedItem, 0, wxLIST_STATE_SELECTED);
    }

    if (nLastSelectedItem == -1)
    {
        if(currentpage->m_nLastFileId == currentpage->m_playlist->GetItemCount() - 1)
        nLastSelectedItem = 0;
    else
            nLastSelectedItem = currentpage->m_nLastFileId + 1;
    }
    else if (nLastSelectedItem == currentpage->m_playlist->GetItemCount() - 1)
            nLastSelectedItem = 0;
        else
            nLastSelectedItem += 1;

    if(nLastSelectedItem == currentpage->m_nLastFileId)
        return; // already playing... nothing to do

    wxListItem listitem;
    listitem.SetMask(wxLIST_MASK_TEXT |  wxLIST_MASK_DATA);
    listitem.SetId(nLastSelectedItem);
    currentpage->m_playlist->GetItem(listitem);
    listitem.SetMask(listitem.GetMask() | wxLIST_MASK_STATE);
    listitem.SetState(listitem.GetState() | wxLIST_STATE_SELECTED);
    currentpage->m_playlist->SetItem(listitem);

    wxASSERT(listitem.GetData());
    DoPlayFile((*((wxString*) listitem.GetData())));
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// wxMediaPlayerNotebookPage
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ----------------------------------------------------------------------------
// wxMediaPlayerNotebookPage Constructor
//
// Creates a media control and slider and adds it to this panel,
// along with some sizers for positioning
// ----------------------------------------------------------------------------
wxMediaPlayerNotebookPage::wxMediaPlayerNotebookPage(wxMediaPlayerFrame* parentFrame,
                                                     wxNotebook* theBook,
                                                     const wxString& szBackend)
                         : wxPanel(theBook, wxID_ANY),
                           m_nLastFileId(-1),
                           m_nLoops(0),
                           m_bLoop(true),
                           m_bIsBeingDragged(false),
                           m_parentFrame(parentFrame)
{
    //
    //  Create and attach a 2-column grid sizer
    //
    wxFlexGridSizer* sizer = new wxFlexGridSizer(2);
    sizer->AddGrowableCol(0);
    this->SetSizer(sizer);

    //
    //  Create our media control
    //
    m_mediactrl = new wxMediaCtrl();

    //  Make sure creation was successful
    bool bOK = m_mediactrl->Create(this, wxID_MEDIACTRL, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize, 0,
                                   szBackend);
    wxASSERT_MSG(bOK, wxT("Could not create media control!"));
    wxUnusedVar(bOK);

    sizer->Add(m_mediactrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND, 5);

    //
    //  Create the playlist/listctrl
    //
    m_playlist = new wxMediaPlayerListCtrl();
    m_playlist->Create(this, wxID_LISTCTRL, wxDefaultPosition,
                    wxDefaultSize,
                    wxLC_REPORT // wxLC_LIST
                    | wxSUNKEN_BORDER);

    //  Set the background of our listctrl to white
    m_playlist->SetBackgroundColour(*wxWHITE);


    m_playlist->AppendColumn(_(""), wxLIST_FORMAT_CENTER, 20);
    m_playlist->AppendColumn(_("File"), wxLIST_FORMAT_LEFT, /*wxLIST_AUTOSIZE_USEHEADER*/305);
    m_playlist->AppendColumn(_("Length"), wxLIST_FORMAT_CENTER, 75);

    sizer->Add(m_playlist, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND, 5);


    // Now that we have all our rows make some of them growable
    sizer->AddGrowableRow(0);

    //
    // Media Control events
    //
    this->Connect(wxID_MEDIACTRL, wxEVT_MEDIA_PLAY,
                  wxMediaEventHandler(wxMediaPlayerNotebookPage::OnMediaPlay));
    this->Connect(wxID_MEDIACTRL, wxEVT_MEDIA_PAUSE,
                  wxMediaEventHandler(wxMediaPlayerNotebookPage::OnMediaPause));
    this->Connect(wxID_MEDIACTRL, wxEVT_MEDIA_FINISHED,
                  wxMediaEventHandler(wxMediaPlayerNotebookPage::OnMediaFinished));
    this->Connect(wxID_MEDIACTRL, wxEVT_MEDIA_LOADED,
                  wxMediaEventHandler(wxMediaPlayerFrame::OnMediaLoaded),
                  (wxObject*)0, parentFrame);
}

// ----------------------------------------------------------------------------
// wxMediaPlayerNotebookPage::OnMediaPlay
//
// Called when the media plays.
// ----------------------------------------------------------------------------
void wxMediaPlayerNotebookPage::OnMediaPlay(wxMediaEvent& WXUNUSED(event))
{
    m_playlist->SetItem(m_nLastFileId, 0, wxT(">"));
}

// ----------------------------------------------------------------------------
// wxMediaPlayerNotebookPage::OnMediaPause
//
// Called when the media is paused.
// ----------------------------------------------------------------------------
void wxMediaPlayerNotebookPage::OnMediaPause(wxMediaEvent& WXUNUSED(event))
{
    m_playlist->SetItem(m_nLastFileId, 0, wxT("||"));
}

// ----------------------------------------------------------------------------
// wxMediaPlayerNotebookPage::OnMediaFinished
//
// Called when the media finishes playing.
// Here we loop it if the user wants to (has been selected from file menu)
// ----------------------------------------------------------------------------
void wxMediaPlayerNotebookPage::OnMediaFinished(wxMediaEvent& WXUNUSED(event))
{
    if(m_bLoop)
    {
        if ( !m_mediactrl->Play() )
        {
            wxMessageBox(wxT("Couldn't loop movie!"));
            m_playlist->SetItem(m_nLastFileId, 0, wxT("E"));
        }
        else
            ++m_nLoops;
    }
    else
    {
        m_playlist->SetItem(m_nLastFileId, 0, wxT("[]"));
    }
}

