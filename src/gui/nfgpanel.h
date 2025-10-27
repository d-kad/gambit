//
// This file is part of Gambit
// Copyright (c) 1994-2025, The Gambit Project (https://www.gambit-project.org)
//
// FILE: src/gui/nfgpanel.h
// Panel to display normal form games in tabular format
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

#ifndef GAMBIT_GUI_NFGPANEL_H
#define GAMBIT_GUI_NFGPANEL_H

#include "wx/sheet/sheet.h" // for wxSheet

class wxPrintout;

namespace Gambit::GUI {
class TableWidget;
class TablePlayerToolbar;

class NfgPanel final : public wxPanel, public GameView {
  wxWindow *m_dominanceToolbar;
  TablePlayerToolbar *m_playerToolbar;
  TableWidget *m_tableWidget;

  void OnToolsDominance(wxCommandEvent &);

public:
  NfgPanel(wxWindow *p_parent, GameDocument *p_doc);
  ~NfgPanel() override = default;

  bool IsDominanceShown() const { return m_dominanceToolbar->IsShown(); }

  /// @name Exporting/printing graphics
  //@{
  /// Creates a printout object of the game as currently displayed
  wxPrintout *GetPrintout();
  /// Creates a bitmap of the game as currently displayed
  bool GetBitmap(wxBitmap &, int marginX, int marginY);
  /// Outputs the game as currently displayed to a SVG file
  void GetSVG(const wxString &p_filename, int marginX, int marginY);
  /// Prints the game as currently displayed, centered on the DC
  void RenderGame(wxDC &p_dc, int marginX, int marginY);
  //@}

  // Overriding GameView members
  void OnUpdate() override;
  void PostPendingChanges() override;

  DECLARE_EVENT_TABLE()
};
} // namespace Gambit::GUI

#endif // GAMBIT_GUI_NFGPANEL_H
