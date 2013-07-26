
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <vector>
#include <algorithm>
#include <numeric>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <wx/treectrl.h>
#include <wx/event.h>

#include "treelistctrl.h"

#include "resource.h"
#include "FileSizeDetecter.h"

class FileSizeDetectApp: public wxApp
{
public:
    virtual bool OnInit();
};
class FileSizeDetectFrame: public wxFrame
{
public:
    FileSizeDetectFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
	void OnSelectDir(wxCommandEvent &);
	void OnStart(wxCommandEvent &);
	void OnStop(wxCommandEvent &);
private:
	void FileDeteterErrorHandler(int err_code, const std::wstring &file_path);
	void FileDeteter_FileEndHandler(const bfs::path &path, uintmax_t size);
	void FileDeteter_DirBegHandler(const bfs::path &path);
	void FileDeteter_DirEndHandler(const bfs::path &path);
private:
	void OnThreadFileEnd(wxThreadEvent &e);
	void OnThreadDirBeg(wxThreadEvent &e);
	void OnThreadDirEnd(wxThreadEvent &e);
private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    wxDECLARE_EVENT_TABLE();

private:
	wxTextCtrl *m_txtCtrlPath;
	wxcode::wxTreeListCtrl *m_treeListCtrlInfo;
	wxButton *m_start_btn;
	bfs::path   m_root_path;

	file_size_detecter m_detecter;
	boost::shared_ptr<boost::thread> m_detect_thread;
	
	std::vector<wxTreeItemId> m_dir_items;
};


wxBEGIN_EVENT_TABLE(FileSizeDetectFrame, wxFrame)
	EVT_THREAD(ID_THREAD_FILE_END, FileSizeDetectFrame::OnThreadFileEnd)
	EVT_THREAD(ID_THREAD_DIR_BEG, FileSizeDetectFrame::OnThreadDirBeg)
	EVT_THREAD(ID_THREAD_DIR_END, FileSizeDetectFrame::OnThreadDirEnd)

	EVT_BUTTON(ID_BTN_SELECT, FileSizeDetectFrame::OnSelectDir)

	EVT_BUTTON(ID_BTN_START, FileSizeDetectFrame::OnStart)
	EVT_BUTTON(ID_BTN_STOP, FileSizeDetectFrame::OnStop)

    EVT_MENU(wxID_EXIT,  FileSizeDetectFrame::OnExit)
    EVT_MENU(wxID_ABOUT, FileSizeDetectFrame::OnAbout)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(FileSizeDetectApp);
bool FileSizeDetectApp::OnInit()
{
    FileSizeDetectFrame *frame = new FileSizeDetectFrame( "FileSizeDetecter", wxPoint(50, 50), wxSize(450, 340) );
    frame->Show( true );
    return true;
}



/////////////////// Frame
FileSizeDetectFrame::FileSizeDetectFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, "&File" );
    menuBar->Append( menuHelp, "&Help" );
    SetMenuBar( menuBar );
    CreateStatusBar();
    SetStatusText( "Welcome to FileSizeDetecter" );

	wxPanel *panel = new wxPanel(this, wxID_ANY);
	wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(topSizer);

	// 选择目录
	wxBoxSizer *selectFoldSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(selectFoldSizer, wxSizerFlags(0).Expand());

	// static
	selectFoldSizer->Add(new wxStaticText(panel, wxID_ANY, wxT("选择文件或目录：")));

	// text
	m_txtCtrlPath = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(300, -1));
	selectFoldSizer->Add(m_txtCtrlPath, wxSizerFlags(1).Align(wxALIGN_LEFT));

	// button
	selectFoldSizer->Add(new wxButton(panel, ID_BTN_SELECT, wxString("...")), wxSizerFlags(0).Align(wxALIGN_RIGHT));


	// 树形控件
	m_treeListCtrlInfo = new wxcode::wxTreeListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 200), wxTR_DEFAULT_STYLE);
	topSizer->Add(m_treeListCtrlInfo, wxSizerFlags(1).Expand());
	m_treeListCtrlInfo->AddColumn(wxT("文件名"), 300);
	m_treeListCtrlInfo->AddColumn(wxT("文件大小"));
	m_treeListCtrlInfo->SetColumnEditable(0, true);
	m_treeListCtrlInfo->SetColumnEditable(1, true);

	// 开始暂停按钮
	wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	m_start_btn = new wxButton(panel, ID_BTN_START, wxT("start"));
	buttonSizer->Add(m_start_btn, wxSizerFlags(1).Center());
	buttonSizer->Add(new wxButton(panel, ID_BTN_STOP, wxT("stop")), wxSizerFlags(1).Center());


	//topSizer->Fit(this);
	topSizer->SetSizeHints(this);

	this->Center();
}

void FileSizeDetectFrame::FileDeteterErrorHandler(int err_code, const std::wstring &file_path)
{
	switch (err_code)
	{
	case file_size_detecter::err_file_not_found:
		break;
	case file_size_detecter::err_file_illegal:
		break;
	case file_size_detecter::err_file_size:
		break;
	default:
		break;
	}
}

void FileSizeDetectFrame::OnExit(wxCommandEvent& event)
{
    Close( true );
}
void FileSizeDetectFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox( "检测占用空间最大的目录和文件\nAuthor: 张善禄",
                  "About FileSizeDetecter", wxOK | wxICON_INFORMATION );
}

void FileSizeDetectFrame::FileDeteter_FileEndHandler( const bfs::path &path, uintmax_t size )
{
	wxThreadEvent *e = new wxThreadEvent;
	e->SetId(ID_THREAD_FILE_END);
	e->SetString(path.wstring().c_str());
	e->SetPayload(size);

	QueueEvent(e);
}

void FileSizeDetectFrame::FileDeteter_DirBegHandler( const bfs::path &path )
{
	wxThreadEvent *e = new wxThreadEvent;
	e->SetId(ID_THREAD_DIR_BEG);
	e->SetString(path.wstring().c_str());

	QueueEvent(e);

}

void FileSizeDetectFrame::FileDeteter_DirEndHandler( const bfs::path &path )
{

	wxThreadEvent *e = new wxThreadEvent;
	e->SetId(ID_THREAD_DIR_END);
	e->SetString(path.wstring().c_str());

	QueueEvent(e);

}

void FileSizeDetectFrame::OnThreadFileEnd( wxThreadEvent &e )
{
	wxTreeItemId item;
	if (m_dir_items.empty())
	{
		item = m_treeListCtrlInfo->AddRoot(e.GetString());

		uintmax_t size = e.GetPayload<uintmax_t>();
		m_treeListCtrlInfo->SetItemText(item, 1, boost::lexical_cast<std::wstring>(size));

		m_detect_thread->join();
		m_start_btn->Enable(true);
	}
	else
	{
		item = m_treeListCtrlInfo->AppendItem(m_dir_items.back(), e.GetString());
		uintmax_t size = e.GetPayload<uintmax_t>();
		m_treeListCtrlInfo->SetItemText(item, 1, boost::lexical_cast<std::wstring>(size));
	}
}

void FileSizeDetectFrame::OnThreadDirBeg( wxThreadEvent &e )
{
	wxTreeItemId item;
	if (m_dir_items.empty())
	{
		item = m_treeListCtrlInfo->AddRoot(e.GetString());
	}
	else
	{
		item = m_treeListCtrlInfo->AppendItem(m_dir_items.back(), e.GetString());
	}
	m_dir_items.push_back(item);
}

boost::wformat KB_format(L"%.3fKB");
boost::wformat MB_format(L"%.3fMB");
boost::wformat GB_format(L"%.3fGB");
std::wstring get_readable_str(uintmax_t size)
{
	if (size > 1024 * 1024 * 1024)
	{
		GB_format.clear_binds();
		GB_format % (size * 1.0 / 1024 / 1024 / 1024);
		return GB_format.str();
	}
	else if (size > 1024 * 1024)
	{
		MB_format.clear_binds();
		MB_format % (size * 1.0 / 1024 / 1024);
		return MB_format.str();
	}
	else if (size > 1024)
	{
		KB_format.clear_binds();
		KB_format % (size * 1.0 / 1024);
		return KB_format.str();
	}

	return boost::lexical_cast<std::wstring>(size) + L" B";
}

void FileSizeDetectFrame::OnThreadDirEnd( wxThreadEvent &e )
{
	typedef std::pair<wxTreeItemId, uintmax_t> item_data_t;
	std::vector<item_data_t> sizeVec;

	wxTreeItemId cur_item = m_dir_items.back();
	// 找出最小 和 最大的文件
	wxTreeItemIdValue cookie;
	for (wxTreeItemId item = m_treeListCtrlInfo->GetFirstChild(cur_item, cookie); 
			item.IsOk(); 
			item = m_treeListCtrlInfo->GetNextSibling(item))
	{
		sizeVec.emplace_back(item, boost::lexical_cast<uintmax_t>(m_treeListCtrlInfo->GetItemText(item, 1).c_str() ) );
	}

	if (!sizeVec.empty())
	{
		// 排序
		std::sort(sizeVec.begin(), sizeVec.end(),
			[](const item_data_t &l, const item_data_t &r){ return l.second > r.second; }
		);

		// set
		m_treeListCtrlInfo->SetItemBackgroundColour(sizeVec.front().first, wxColour(wxT("#a0a0f0")));
		if (sizeVec.size() > 1)
			m_treeListCtrlInfo->SetItemBackgroundColour(sizeVec[1].first, wxColour(wxT("#c4c4f0")));

		if (sizeVec.size() > 2)
			m_treeListCtrlInfo->SetItemBackgroundColour(sizeVec[2].first, wxColour(wxT("#e8e8ff")));

		for (size_t index = 0; index < sizeVec.size() && index < 5; ++index)
		{
			m_treeListCtrlInfo->SetItemText(sizeVec[index].first, 1, get_readable_str(sizeVec[index].second) );
		}

		for (size_t index = 5; index < sizeVec.size(); ++index)
		{
			m_treeListCtrlInfo->Delete(sizeVec[index].first);
		}
	}
	// 计算出此目录的总大小
	uintmax_t dir_size = std::accumulate<std::vector<item_data_t>::iterator, uintmax_t>(sizeVec.begin(), sizeVec.end(), 0, 
		[](uintmax_t l, const item_data_t &r){ return l + r.second; }
	);

	m_treeListCtrlInfo->SetItemText(cur_item, 1, boost::lexical_cast<std::wstring>(dir_size).c_str());

	// 将此目录清除
	m_dir_items.pop_back();

	if (m_dir_items.empty())
	{
		m_treeListCtrlInfo->SetItemText(cur_item, 1, get_readable_str(dir_size));
		m_detect_thread->join();
		m_start_btn->Enable(true);
	}
	
}

void FileSizeDetectFrame::OnSelectDir(wxCommandEvent &)
{
	wxDirDialog dlg(this, wxT("选择想要检测的目录") );
	if (dlg.ShowModal() == wxID_OK)
	{
		m_txtCtrlPath->SetValue(dlg.GetPath() );
	}
}

void FileSizeDetectFrame::OnStart(wxCommandEvent &)
{
	m_root_path = m_txtCtrlPath->GetValue().Trim(true).Trim(false).c_str();
	if (m_root_path.empty())
	{
		wxMessageBox(wxT("路径不能为空或不能为相对路径"));
		return;
	}
	else if ( m_root_path.is_relative())
	{

		wxMessageBox(wxT("路径不能为相对路径"));
		return;
	}
	else if (!bfs::exists(m_root_path))
	{
		wxMessageBox(wxT("路径不存在"));
		return;
	}

	m_treeListCtrlInfo->DeleteRoot();

	m_root_path.make_preferred();

	m_detecter.set_path(m_root_path);
	m_detecter.register_file_end_handler( boost::bind(&FileSizeDetectFrame::FileDeteter_FileEndHandler, this, _1, _2) );
	m_detecter.register_dir_beg_handler( boost::bind(&FileSizeDetectFrame::FileDeteter_DirBegHandler, this, _1) );
	m_detecter.register_dir_end_handler( boost::bind(&FileSizeDetectFrame::FileDeteter_DirEndHandler, this, _1) );
	m_detecter.register_err_handler( boost::bind(&FileSizeDetectFrame::FileDeteterErrorHandler, this, _1, _2) );

	m_detect_thread.reset(new boost::thread(&file_size_detecter::start, &m_detecter) );

	m_start_btn->Enable(false);
}

void FileSizeDetectFrame::OnStop(wxCommandEvent &)
{

}
