#pragma once

#include <vector>

class CThumbnailViewerDlg : public CDialogEx
{
public:
	CThumbnailViewerDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THUMBNAILVIEWER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl shelllist_;
	CMFCShellTreeCtrl shelltree_;
	afx_msg void OnTvnSelchangedMfcshelltree1(NMHDR *pNMHDR, LRESULT *pResult);

	bool is_running_ = false;

	DWORD thread_id_;
	HANDLE thread_;
	bool is_terminated = false;
	CString cur_path;
	std::vector<CString> filenames_;
	CImageList thumblist_;
	ULONG_PTR token_;

	bool RunThread();
	bool StopThread();
	bool IsValidImage(CString path);
	
	afx_msg void OnDestroy();
};
