/*
    CSliders.h:

    Copyright (C) 1995 John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

// CSliders.h : header file
//

#include "dialogs.h"
extern "C" {
  void DisplaySliders(int);
  int GetSliderValue(int);
  void SetSliderValue(int, int);
  void SetSliderMin(int, int);
  void SetSliderMax(int, int);
  void SetSliderLab(int, char *);
}

#define NUM_SLIDERS     (4)
#define NUM_BUTTONS     (4)
#define NUM_CHECKS      (4)

//**********************************************************************
// CSlider dialog
//**********************************************************************

class CSlider : public CDialog
{
// Construction
public:
      CSlider(CWnd* pParent = NULL);   // standard constructor
      BOOL Create();

      int       m_num[NUM_SLIDERS];
      long      m_max[NUM_SLIDERS];
      long      m_min[NUM_SLIDERS];
      CString   m_lab[NUM_SLIDERS];
      CScrollBar bar[NUM_SLIDERS];
      friend void SetSliderValue(int, int);
      friend void SetSliderMax(int, int);
      friend void SetSliderMin(int, int);
      friend void SetSliderLab(int, char *);
// Dialog Data
        //{{AFX_DATA(CSlider)
      enum { IDD = IDD_SLIDERS };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CSlider)
protected:
      virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
      virtual void PostNcDestroy();
      virtual void OnOK();
        //}}AFX_VIRTUAL

// Implementation
protected:
      CWnd* m_pParent;
      int m_nID;

      HICON m_hIcon;
      void OnRange(int i);
      void OnValue(int i);
      void LoadSlider(char *);
      void StoreSlider(char *);
      HCURSOR OnQueryDragIcon(void) {return (HCURSOR)m_hIcon; }

        // Generated message map functions
        //{{AFX_MSG(CSlider)
      virtual BOOL OnInitDialog();
      afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
      afx_msg void OnRange_1() { OnRange(0); }
      afx_msg void OnRange_2() { OnRange(1); }
      afx_msg void OnRange_3() { OnRange(2); }
      afx_msg void OnRange_4() { OnRange(3); }
      afx_msg void OnValue_1() { OnValue(0); }
      afx_msg void OnValue_2() { OnValue(1); }
      afx_msg void OnValue_3() { OnValue(2); }
      afx_msg void OnValue_4() { OnValue(3); }
      afx_msg void OnLoad();
      afx_msg void OnSetter();
      afx_msg void OnStore();
      afx_msg void OnClose();
        //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};

// CRangeDlg dialog

class CRangeDlg : public CDialog
{
// Construction
public:
        CRangeDlg(CWnd* pParent/* = NULL*/, int, long, long, long); // standard constructor

// Dialog Data
        //{{AFX_DATA(CRangeDlg)
        enum { IDD = IDD_SETSLIDER };
        long            val;
        int             name;
        long            max;
        long            min;
        //}}AFX_DATA
        //{{AFX_VIRTUAL(CRangeDlg)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL
// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CRangeDlg)
                // NOTE: the ClassWizard will add member functions here
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

class CButtons : public CDialog
{
// Construction
public:
  CButtons(CWnd* pParent = NULL, int b=0);   // standard constructor
  BOOL Create();

  int       m_button[NUM_BUTTONS];
  CString   m_lab[NUM_BUTTONS];
// Dialog Data
  //{{AFX_DATA(CButtons)
  enum { IDD = IDD_BUTTONS };
  // NOTE: the ClassWizard will add data members here
  //}}AFX_DATA


  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CButtons)
  int Read(int i);
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void Name(int i, char* t);
  void OnPush(int i);
  virtual void PostNcDestroy();
  virtual void OnOK();
  //}}AFX_VIRTUAL

  // Implementation
protected:
  CWnd* m_pParent;
  int m_nID;

  HICON m_hIcon;
  HCURSOR OnQueryDragIcon(void) {return (HCURSOR)m_hIcon; }

  // Generated message map functions
  //{{AFX_MSG(CButton)
  virtual BOOL OnInitDialog();
      afx_msg void OnPush_1() { OnPush(0); }
      afx_msg void OnPush_2() { OnPush(1); }
      afx_msg void OnPush_3() { OnPush(2); }
      afx_msg void OnPush_4() { OnPush(3); }
  afx_msg void OnClose();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};


class CChecks : public CDialog
{
// Construction
public:
      CChecks(CWnd* pParent = NULL, int b=0);   // standard constructor
      BOOL Create();

      int       m_check[NUM_CHECKS];
      CString   m_lab[NUM_CHECKS];
// Dialog Data
        //{{AFX_DATA(CChecks)
      enum { IDD = IDD_CHECKS };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CChecks)
protected:
      virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
      void OnPush(int i);
      int Read(int i);
      void Name(int i, char* t);
      virtual void PostNcDestroy();
      virtual void OnOK();
        //}}AFX_VIRTUAL

// Implementation
protected:
      CWnd* m_pParent;
      int m_nID;

      HICON m_hIcon;
      HCURSOR OnQueryDragIcon(void) {return (HCURSOR)m_hIcon; }

        // Generated message map functions
        //{{AFX_MSG(CChecks)
      virtual BOOL OnInitDialog();
      afx_msg void OnPush_1() { OnPush(0); }
      afx_msg void OnPush_2() { OnPush(1); }
      afx_msg void OnPush_3() { OnPush(2); }
      afx_msg void OnPush_4() { OnPush(3); }
      afx_msg void OnClose();
        //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
};


