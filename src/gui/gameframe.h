//
// This file is part of Gambit
// Copyright (c) 1994-2025, The Gambit Project (https://www.gambit-project.org)
//
// FILE: src/gui/gameframe.h
// Interface to frame containing the views of a game
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

#ifndef GAMBIT_GUI_GAMEFRAME_H
#define GAMBIT_GUI_GAMEFRAME_H

#include <wx/printdlg.h>

#include "gamedoc.h"

class wxSplitterWindow;
class wxSplitterEvent;

namespace Gambit::GUI {
class AnalysisNotebook;
class EfgPanel;
class NfgPanel;

class GameFrame final : public wxFrame, public GameView {
  wxSplitterWindow *m_splitter;
  EfgPanel *m_efgPanel;
  NfgPanel *m_nfgPanel;
  AnalysisNotebook *m_analysisPanel;

  wxPageSetupData m_pageSetupData;
  wxPrintData m_printData;

  void MakeMenus();
  void MakeToolbar();

  // Event handlers
  void OnCloseWindow(wxCloseEvent &);

  // Menu event handlers
  void OnFileNewEfg(wxCommandEvent &);
  void OnFileNewNfg(wxCommandEvent &);
  void OnFileOpen(wxCommandEvent &);
  void OnFileClose(wxCommandEvent &);
  void OnFileSave(wxCommandEvent &);
  void OnFileExportEfg(wxCommandEvent &);
  void OnFileExportNfg(wxCommandEvent &);
  void OnFileExportGraphic(wxCommandEvent &);
  void OnFileExportPS(wxCommandEvent &);
  void OnFileExportSVG(wxCommandEvent &);

  void OnFilePageSetup(wxCommandEvent &);
  void OnFilePrintPreview(wxCommandEvent &);
  void OnFilePrint(wxCommandEvent &);
  void OnFileExit(wxCommandEvent &);
  void OnFileMRUFile(wxCommandEvent &);

  void OnEditUndo(wxCommandEvent &);
  void OnEditRedo(wxCommandEvent &);

  void OnEditInsertMove(wxCommandEvent &);
  void OnEditInsertAction(wxCommandEvent &);
  void OnEditDeleteTree(wxCommandEvent &);
  void OnEditDeleteParent(wxCommandEvent &);
  void OnEditRemoveOutcome(wxCommandEvent &);
  void OnEditReveal(wxCommandEvent &);

  void OnEditNode(wxCommandEvent &);
  void OnEditMove(wxCommandEvent &);
  void OnEditGame(wxCommandEvent &);

  void OnEditNewPlayer(wxCommandEvent &);

  void OnViewProfiles(wxCommandEvent &);
  void OnViewZoom(wxCommandEvent &);
  void OnViewStrategic(wxCommandEvent &);

  void OnFormatLayout(wxCommandEvent &);
  void OnFormatLabels(wxCommandEvent &);
  void OnFormatDecimalsAdd(wxCommandEvent &);
  void OnFormatDecimalsDelete(wxCommandEvent &);
  void OnFormatFonts(wxCommandEvent &);

  void OnToolsDominance(wxCommandEvent &);
  void OnToolsEquilibrium(wxCommandEvent &);
  void OnToolsQre(wxCommandEvent &);

  void OnHelpAbout(wxCommandEvent &);

  void OnUnsplit(wxSplitterEvent &);

  /// @name Overriding wxWindow behavior
  //@{
  /// Flush any pending changes in document before processing event
  bool ProcessEvent(wxEvent &) override;
  //@}

  // Overriding GameView members
  void OnUpdate() override;

public:
  // CONSTRUCTOR AND DESTRUCTOR
  GameFrame(wxWindow *p_parent, GameDocument *p_doc);
  ~GameFrame() override;

  DECLARE_EVENT_TABLE()
};

} // namespace Gambit::GUI

#endif // GAMBIT_GUI_GAMEFRAME_H
