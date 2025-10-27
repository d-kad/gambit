//
// This file is part of Gambit
// Copyright (c) 1994-2025, The Gambit Project (https://www.gambit-project.org)
//
// FILE: src/gui/gambit.h
// Declaration of application-level class for Gambit graphical interface
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifndef GAMBIT_GUI_APP_H
#define GAMBIT_GUI_APP_H

#include <wx/wx.h>
#include <wx/config.h>  // for wxConfig
#include <wx/docview.h> // for wxFileHistory

namespace Gambit::GUI {

class GameDocument;

enum AppLoadResult { GBT_APP_FILE_OK = 0, GBT_APP_OPEN_FAILED = 1, GBT_APP_PARSE_FAILED = 2 };

class Application final : public wxApp {
  wxString m_currentDir; /* Current position in directory tree. */
  wxFileHistory m_fileHistory{10};
  std::list<GameDocument *> m_documents;

  bool OnInit() override;

public:
  Application() = default;
  ~Application() override = default;

  const wxString &GetCurrentDir() { return m_currentDir; }
  void SetCurrentDir(const wxString &p_dir);

  wxString GetHistoryFile(int index) const { return m_fileHistory.GetHistoryFile(index); }
  void AddMenu(wxMenu *p_menu)
  {
    m_fileHistory.UseMenu(p_menu);
    m_fileHistory.AddFilesToMenu(p_menu);
  }
  void RemoveMenu(wxMenu *p_menu) { m_fileHistory.RemoveMenu(p_menu); }

  AppLoadResult LoadFile(const wxString &);
#ifdef __WXMAC__
  void MacOpenFile(const wxString &filename) override { LoadFile(filename); }
#endif // __WXMAC__

  //!
  //! These manage the list of open documents
  //!
  //@{
  void AddDocument(GameDocument *p_doc) { m_documents.push_back(p_doc); }
  void RemoveDocument(const GameDocument *p_doc)
  {
    m_documents.erase(std::find(m_documents.begin(), m_documents.end(), p_doc));
  }
  bool AreDocumentsModified() const;
  //@}
};

} // namespace Gambit::GUI

DECLARE_APP(Gambit::GUI::Application)

#endif // GAMBIT_GUI_APP_H
