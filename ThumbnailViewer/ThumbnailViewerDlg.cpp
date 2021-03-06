#include "stdafx.h"
#include "ThumbnailViewer.h"
#include "ThumbnailViewerDlg.h"
#include "afxdialogex.h"

#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define THUMBNAIL_WIDTH 100
#define THUMBNAIL_HEIGHT 90

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


CThumbnailViewerDlg::CThumbnailViewerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_THUMBNAILVIEWER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CThumbnailViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, shelllist_);
	DDX_Control(pDX, IDC_MFCSHELLTREE1, shelltree_);
}

BEGIN_MESSAGE_MAP(CThumbnailViewerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TVN_SELCHANGED, IDC_MFCSHELLTREE1, &CThumbnailViewerDlg::OnTvnSelchangedMfcshelltree1)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


BOOL CThumbnailViewerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	CoInitialize(0);

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&token_, &gdiplusStartupInput, NULL);

	thumblist_.Create(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, ILC_COLOR32, 0, 1);
	shelllist_.SetImageList(&thumblist_, LVSIL_NORMAL);

	return TRUE;
}

void CThumbnailViewerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CThumbnailViewerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CThumbnailViewerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CThumbnailViewerDlg::OnTvnSelchangedMfcshelltree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	CString path;
	shelltree_.GetItemPath(path);

	if (!path.IsEmpty())
	{
		if (is_running_) StopThread();

		cur_path = path;
		filenames_.clear();

		CString wildcard(path);
		wildcard += L"\\*.*";

		CFileFind finder;
		BOOL isWorking = finder.FindFile(wildcard);

		while (isWorking)
		{
			isWorking = finder.FindNextFile();

			if (finder.IsDots() || finder.IsDirectory())
			{
				continue;
			}
			else
			{
				CString filepath = finder.GetFileName();

				if (IsValidImage(cur_path + L"\\" + filepath))
				{
					filepath.MakeLower();
					filenames_.push_back(filepath);
				}
			}
		}

		finder.Close();
		RunThread();
	}
	
	*pResult = 0;
}



bool CThumbnailViewerDlg::StopThread()
{
	if (is_running_ == false) return true;

	is_terminated = true;

	while (is_running_)
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	::CloseHandle(thread_);

	return true;
}

bool CThumbnailViewerDlg::IsValidImage(CString path)
{
	Gdiplus::Bitmap image(path.AllocSysString());

	if (image.GetFlags() == Gdiplus::ImageFlagsNone) return false;

	return true;
}


unsigned long _stdcall LoadThumbnail(LPVOID param)
{
	auto dlg = (CThumbnailViewerDlg*)param;
	int idx = 0;

	CListCtrl& list = dlg->shelllist_;
	CImageList* imglist = &dlg->thumblist_;

	int imgCount = imglist->GetImageCount();

	for (int i = 0; i < imgCount; i++)
	{
		imglist->Remove(i);
	}

	list.DeleteAllItems();
	imglist->SetImageCount(dlg->filenames_.size());

	list.SetRedraw(FALSE);

	for (auto iter = dlg->filenames_.begin();
		iter != dlg->filenames_.end() && !dlg->is_terminated;
		iter++, idx++)
	{
		HBITMAP hbitmap = NULL;
		CBitmap cbitmap;

		list.InsertItem(idx, *iter, idx);

		CString path;
		path.Format(L"%s\\%s", dlg->cur_path, *iter);

		Gdiplus::Bitmap image(path.AllocSysString());

		int srcWidth = image.GetWidth();
		int srcHeight = image.GetHeight();

		int destX = 0;
		int destY = 0;

		float per = 0.0;
		float perW = (static_cast<float>(THUMBNAIL_WIDTH) / static_cast<float>(srcWidth));
		float perH = (static_cast<float>(THUMBNAIL_HEIGHT) / static_cast<float>(srcHeight));

		if (perH < perW)
		{
			per = perH;
			destX = static_cast<int>((THUMBNAIL_WIDTH - (srcWidth * per)) / 2);
		}
		else
		{
			per = perW;
			destY = static_cast<int>((THUMBNAIL_HEIGHT - (srcHeight * per)) / 2);
		}

		int destWidth = static_cast<int>(srcWidth * per);
		int destHeight = static_cast<int>(srcHeight * per);

		Gdiplus::Bitmap b(static_cast<int>(THUMBNAIL_WIDTH), static_cast<int>(THUMBNAIL_HEIGHT), PixelFormat24bppRGB);
		b.SetResolution(image.GetHorizontalResolution(), image.GetVerticalResolution());

		Gdiplus::Graphics* graphics = Gdiplus::Graphics::FromImage(&b);
		Gdiplus::Color color(255, 255, 255, 255);
		graphics->Clear(color);
		graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		graphics->DrawImage(&image, Gdiplus::Rect(destX, destY, destWidth, destHeight));
		
		b.GetHBITMAP(color, &hbitmap);
		cbitmap.Attach(hbitmap);
		imglist->Replace(idx, &cbitmap, nullptr);

		delete graphics;

		cbitmap.Detach();
		DeleteObject(hbitmap);
	}

	list.SetRedraw(TRUE);
	list.Invalidate();

	dlg->is_running_ = false;
	dlg->is_terminated = false;
	dlg->thread_ = NULL;
	::CloseHandle(dlg->thread_);

	return 0;
}

bool CThumbnailViewerDlg::RunThread()
{
	thread_ = CreateThread(NULL, 0, LoadThumbnail, this, 0, &thread_id_);

	is_running_ = true;

	return true;
}

void CThumbnailViewerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	
	Gdiplus::GdiplusShutdown(token_);
	CoUninitialize();
}
