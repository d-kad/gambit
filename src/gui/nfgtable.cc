//
// This file is part of Gambit
// Copyright (c) 1994-2025, The Gambit Project (https://www.gambit-project.org)
//
// FILE: src/gui/nfgtable.cc
// Implementation of strategic game matrix display/editor
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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif                // WX_PRECOMP
#include <wx/dnd.h>   // for drag-and-drop support
#include <wx/print.h> // for printing support
#include <wx/dcsvg.h> // for SVG output

#include "wx/sheet/sheet.h"

#include "renratio.h" // special renderer for rational numbers

#include "gamedoc.h"
#include "nfgpanel.h"
#include "nfgtable.h"
#include "dlexcept.h"

//=========================================================================
//                       class gbtTableWidgetBase
//=========================================================================

//!
//! This class handles some common overriding of wxSheet behavior
//! common to the sheets used in the strategic game display.
//!
class gbtTableWidgetBase : public wxSheet {
protected:
  //!
  //! @name Overriding wxSheet members to disable selection behavior
  //!
  //@{
  bool SelectRow(int, bool = false, bool = false) override { return false; }
  bool SelectRows(int, int, bool = false, bool = false) override { return false; }
  bool SelectCol(int, bool = false, bool = false) override { return false; }
  bool SelectCols(int, int, bool = false, bool = false) override { return false; }
  bool SelectCell(const wxSheetCoords &, bool = false, bool = false) override { return false; }
  bool SelectBlock(const wxSheetBlock &, bool = false, bool = false) override { return false; }
  bool SelectAll(bool = false) override { return false; }

  bool HasSelection(bool = true) const override { return false; }
  bool IsCellSelected(const wxSheetCoords &) const override { return false; }
  bool IsRowSelected(int) const override { return false; }
  bool IsColSelected(int) const override { return false; }
  bool DeselectBlock(const wxSheetBlock &, bool = false) override { return false; }
  bool ClearSelection(bool = false) override { return false; }
  //@}

  /// Overriding wxSheet member to suppress drawing of cursor
  void DrawCursorCellHighlight(wxDC &, const wxSheetCellAttr &) override {}

  /// Overriding wxSheet member to show editor on one click
  void OnCellLeftClick(wxSheetEvent &);

public:
  /// @name Lifecycle
  //@{
  /// Constructor
  gbtTableWidgetBase(wxWindow *p_parent, wxWindowID p_id, const wxPoint &p_pos = wxDefaultPosition,
                     const wxSize &p_size = wxDefaultSize, long p_style = wxWANTS_CHARS,
                     const wxString &p_name = wxT("wxSheet"))
    : wxSheet(p_parent, p_id, p_pos, p_size, p_style, p_name)
  {
    Connect(
        GetId(), wxEVT_SHEET_CELL_LEFT_DOWN,
        (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
            wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidgetBase::OnCellLeftClick))));
  }
  //@}

  /// @name Drop target interaction
  //@{
  /// Called when the drop target receives text.
  virtual bool DropText(int p_x, int p_y, const wxString &p_text) { return false; }
  //@}

  /// @name Access to scrollbars from underlying wxSheet
  //@{
  /// Get the vertical scrollbar
  wxScrollBar *GetVerticalScrollBar() const { return m_vertScrollBar; }
  /// Get the horizontal scrollbar
  wxScrollBar *GetHorizontalScrollBar() const { return m_horizScrollBar; }
  //@}
};

void gbtTableWidgetBase::OnCellLeftClick(wxSheetEvent &p_event)
{
  SetGridCursorCell(p_event.GetCoords());
  EnableCellEditControl(p_event.GetCoords());
}

//=========================================================================
//                class gbtTableWidgetDropTarget
//=========================================================================

//!
//! This simple class serves as a drop target for players; it simply
//! communicates the location and text of the drop to its owner for
//! further processing
//!
class gbtTableWidgetDropTarget : public wxTextDropTarget {
private:
  gbtTableWidgetBase *m_owner;

public:
  explicit gbtTableWidgetDropTarget(gbtTableWidgetBase *p_owner) : m_owner(p_owner) {}

  bool OnDropText(wxCoord x, wxCoord y, const wxString &p_text) override
  {
    return m_owner->DropText(x, y, p_text);
  }
};

//=========================================================================
//                       class gbtRowPlayerWidget
//=========================================================================

class gbtRowPlayerWidget : public gbtTableWidgetBase {
private:
  gbtGameDocument *m_doc;
  gbtTableWidget *m_table;

  /// @name Overriding wxSheet members for data access
  //@{
  /// Returns the value in the cell
  wxString GetCellValue(const wxSheetCoords &) override;
  /// Sets the value in the cell, by relabeling the strategy
  void SetCellValue(const wxSheetCoords &, const wxString &) override;
  /// Returns the attributes of the cell
  wxSheetCellAttr GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const override;
  //@}

  /// @name Overriding wxSheet members to customize drawing
  //@{
  /// Overrides to draw dominance indicators
  void DrawCell(wxDC &p_dc, const wxSheetCoords &p_coords) override;
  //@}

  void OnCellRightClick(wxSheetEvent &);

public:
  /// @name Lifecycle
  //@{
  /// Constructorw
  gbtRowPlayerWidget(gbtTableWidget *p_parent, gbtGameDocument *p_doc);
  //@}

  /// @name Synchronizing with document state
  //@{
  void OnUpdate();
  //@}

  /// @name Drop target interaction
  //@{
  /// Called when the drop target receives text.
  bool DropText(int p_x, int p_y, const wxString &p_text) override;
  //@}
};

gbtRowPlayerWidget::gbtRowPlayerWidget(gbtTableWidget *p_parent, gbtGameDocument *p_doc)
  : gbtTableWidgetBase(p_parent, wxID_ANY), m_doc(p_doc), m_table(p_parent)
{
  CreateGrid(m_table->NumRowContingencies(), m_table->NumRowPlayers());
  SetRowLabelWidth(1);
  SetColLabelHeight(1);
  SetScrollBarMode(SB_NEVER);
  SetGridLineColour(*wxBLACK);

  SetDropTarget(new gbtTableWidgetDropTarget(this));

  Connect(GetId(), wxEVT_SHEET_CELL_RIGHT_DOWN,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtRowPlayerWidget::OnCellRightClick))));
}

GameStrategy GetStrategy(const StrategySupportProfile &p_profile, int pl, int st)
{
  auto strategies = p_profile.GetStrategies(p_profile.GetGame()->GetPlayer(pl));
  return *std::next(strategies.begin(), st - 1);
}

void gbtRowPlayerWidget::OnCellRightClick(wxSheetEvent &p_event)
{
  if (m_table->NumRowPlayers() == 0 || m_doc->GetGame()->IsTree()) {
    p_event.Skip();
    return;
  }

  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  const wxSheetCoords coords = p_event.GetCoords();

  const int player = m_table->GetRowPlayer(coords.GetCol() + 1);
  const int strat = m_table->RowToStrategy(coords.GetCol() + 1, coords.GetRow());

  m_doc->DoDeleteStrategy(GetStrategy(support, player, strat));
}

wxString gbtRowPlayerWidget::GetCellValue(const wxSheetCoords &p_coords)
{
  if (IsLabelCell(p_coords)) {
    return wxT("");
  }

  if (m_table->NumRowPlayers() == 0) {
    return wxT("Payoffs");
  }

  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();

  const int player = m_table->GetRowPlayer(p_coords.GetCol() + 1);
  const int strat = m_table->RowToStrategy(p_coords.GetCol() + 1, p_coords.GetRow());

  return {GetStrategy(support, player, strat)->GetLabel().c_str(), *wxConvCurrent};
}

void gbtRowPlayerWidget::SetCellValue(const wxSheetCoords &p_coords, const wxString &p_value)
{
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();

  const int player = m_table->GetRowPlayer(p_coords.GetCol() + 1);
  const int strat = m_table->RowToStrategy(p_coords.GetCol() + 1, p_coords.GetRow());

  m_doc->DoSetStrategyLabel(GetStrategy(support, player, strat), p_value);
}

wxSheetCellAttr gbtRowPlayerWidget::GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const
{
  wxSheetCellAttr attr(GetSheetRefData()->m_defaultGridCellAttr);
  attr.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
  attr.SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
  attr.SetOrientation(wxHORIZONTAL);
  if (m_table->NumRowPlayers() > 0) {
    attr.SetForegroundColour(
        m_doc->GetStyle().GetPlayerColor(m_table->GetRowPlayer(p_coords.GetCol() + 1)));
    attr.SetReadOnly(m_doc->IsTree());
  }
  else {
    attr.SetForegroundColour(*wxBLACK);
    attr.SetReadOnly(true);
  }
  attr.SetBackgroundColour(*wxLIGHT_GREY);
  return attr;
}

void gbtRowPlayerWidget::DrawCell(wxDC &p_dc, const wxSheetCoords &p_coords)
{
  gbtTableWidgetBase::DrawCell(p_dc, p_coords);

  if (!m_table->ShowDominance() || IsLabelCell(p_coords) || m_table->NumRowPlayers() == 0) {
    return;
  }

  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  const int player = m_table->GetRowPlayer(p_coords.GetCol() + 1);
  const int strat = m_table->RowToStrategy(p_coords.GetCol() + 1, p_coords.GetRow());
  const Gambit::GameStrategy strategy = GetStrategy(support, player, strat);

  if (support.IsDominated(strategy, false)) {
    const wxRect rect = CellToRect(p_coords);
    if (support.IsDominated(strategy, true)) {
      p_dc.SetPen(wxPen(m_doc->GetStyle().GetPlayerColor(player), 2, wxPENSTYLE_SOLID));
    }
    else {
      p_dc.SetPen(wxPen(m_doc->GetStyle().GetPlayerColor(player), 1, wxPENSTYLE_SHORT_DASH));
    }
    p_dc.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    p_dc.DrawLine(rect.x + rect.width, rect.y, rect.x, rect.y + rect.height);
  }
}

void gbtRowPlayerWidget::OnUpdate()
{
  const int newRows = m_table->NumRowContingencies();
  if (newRows > GetNumberRows()) {
    InsertRows(0, newRows - GetNumberRows());
  }
  if (newRows < GetNumberRows()) {
    DeleteRows(0, GetNumberRows() - newRows);
  }

  const int newCols = m_table->NumRowPlayers();
  if (newCols > GetNumberCols()) {
    InsertCols(0, newCols - GetNumberCols());
  }
  if (newCols < GetNumberCols()) {
    DeleteCols(0, GetNumberCols() - newCols);
  }
  if (newCols == 0) {
    InsertCols(0, 1);
  }

  for (int col = 0; col < GetNumberCols(); col++) {
    for (int row = 0; row < GetNumberRows();
         SetCellSpan(wxSheetCoords(row++, col), wxSheetCoords(1, 1)))
      ;

    const int span = m_table->NumRowsSpanned(col + 1);

    int row = 0;
    while (row < GetNumberRows()) {
      SetCellSpan(wxSheetCoords(row, col), wxSheetCoords(span, 1));
      row += span;
    }
  }

  Refresh();
}

bool gbtRowPlayerWidget::DropText(wxCoord p_x, wxCoord p_y, const wxString &p_text)
{
  if (p_text[0] == 'P') {
    long pl;
    p_text.Right(p_text.Length() - 1).ToLong(&pl);

    if (m_table->NumRowPlayers() == 0) {
      m_table->SetRowPlayer(1, pl);
      return true;
    }

    for (int col = 0; col < GetNumberCols(); col++) {
      const wxRect rect = CellToRect(wxSheetCoords(0, col));

      if (p_x >= rect.x && p_x < rect.x + rect.width / 2) {
        m_table->SetRowPlayer(col + 1, pl);
        return true;
      }
      else if (p_x >= rect.x + rect.width / 2 && p_x < rect.x + rect.width) {
        m_table->SetRowPlayer(col + 2, pl);
        return true;
      }
    }
  }

  return false;
}

//=========================================================================
//                       class gbtColPlayerWidget
//=========================================================================

class gbtColPlayerWidget : public gbtTableWidgetBase {
private:
  gbtGameDocument *m_doc;
  gbtTableWidget *m_table;

  /// @name Overriding wxSheet members for data access
  //@{
  /// Returns the value in the cell
  wxString GetCellValue(const wxSheetCoords &) override;
  /// Sets the value in the cell, by relabeling the strategy
  void SetCellValue(const wxSheetCoords &, const wxString &) override;
  /// Returns the attributes of the cell
  wxSheetCellAttr GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const override;
  //@}

  /// @name Overriding wxSheet members to customize drawing
  //@{
  /// Overrides to draw dominance indicators
  void DrawCell(wxDC &p_dc, const wxSheetCoords &p_coords) override;
  //@}

  void OnCellRightClick(wxSheetEvent &);

public:
  /// @name Lifecycle
  //@{
  /// Constructor
  gbtColPlayerWidget(gbtTableWidget *p_parent, gbtGameDocument *p_doc);
  //@}

  /// @name Synchronizing with document state
  //@{
  void OnUpdate();
  //@}

  /// @name Drop target interaction
  //@{
  /// Called when the drop target receives text.
  bool DropText(int p_x, int p_y, const wxString &p_text) override;
  //@}
};

gbtColPlayerWidget::gbtColPlayerWidget(gbtTableWidget *p_parent, gbtGameDocument *p_doc)
  : gbtTableWidgetBase(p_parent, wxID_ANY), m_doc(p_doc), m_table(p_parent)
{
  CreateGrid(m_table->NumColPlayers(), 0);
  SetRowLabelWidth(1);
  SetColLabelHeight(1);
  SetScrollBarMode(SB_NEVER);
  SetGridLineColour(*wxBLACK);
  SetBackgroundColour(*wxLIGHT_GREY);

  SetDropTarget(new gbtTableWidgetDropTarget(this));

  Connect(GetId(), wxEVT_SHEET_CELL_RIGHT_DOWN,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtColPlayerWidget::OnCellRightClick))));
}

void gbtColPlayerWidget::OnCellRightClick(wxSheetEvent &p_event)
{
  if (m_table->NumColPlayers() == 0 || m_doc->GetGame()->IsTree()) {
    p_event.Skip();
    return;
  }

  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  const wxSheetCoords coords = p_event.GetCoords();

  const int player = m_table->GetColPlayer(coords.GetRow() + 1);
  const int strat = m_table->RowToStrategy(coords.GetRow() + 1, coords.GetCol());

  m_doc->DoDeleteStrategy(GetStrategy(support, player, strat));
}

void gbtColPlayerWidget::OnUpdate()
{
  const int newCols = m_table->NumColContingencies() * m_doc->NumPlayers();
  if (newCols > GetNumberCols()) {
    InsertCols(0, newCols - GetNumberCols());
  }
  if (newCols < GetNumberCols()) {
    DeleteCols(0, GetNumberCols() - newCols);
  }

  const int newRows = m_table->NumColPlayers();
  if (newRows > GetNumberRows()) {
    InsertRows(0, newRows - GetNumberRows());
  }
  if (newRows < GetNumberRows()) {
    DeleteRows(0, GetNumberRows() - newRows);
  }
  if (newRows == 0) {
    InsertRows(0, 1);
  }

  for (int row = 0; row < GetNumberRows(); row++) {
    for (int col = 0; col < GetNumberCols();
         SetCellSpan(wxSheetCoords(row, col++), wxSheetCoords(1, 1)))
      ;

    const int span = m_table->NumColsSpanned(row + 1) * m_doc->NumPlayers();

    int col = 0;
    while (col < GetNumberCols()) {
      SetCellSpan(wxSheetCoords(row, col), wxSheetCoords(1, span));
      col += span;
    }
  }

  Refresh();
}

wxString gbtColPlayerWidget::GetCellValue(const wxSheetCoords &p_coords)
{
  if (IsLabelCell(p_coords)) {
    return wxT("");
  }

  if (m_table->NumColPlayers() == 0) {
    return wxT("Payoffs");
  }

  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();

  const int player = m_table->GetColPlayer(p_coords.GetRow() + 1);
  const int strat = m_table->ColToStrategy(p_coords.GetRow() + 1, p_coords.GetCol());

  return {GetStrategy(support, player, strat)->GetLabel().c_str(), *wxConvCurrent};
}

void gbtColPlayerWidget::SetCellValue(const wxSheetCoords &p_coords, const wxString &p_value)
{
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();

  const int player = m_table->GetColPlayer(p_coords.GetRow() + 1);
  const int strat = m_table->ColToStrategy(p_coords.GetRow() + 1, p_coords.GetCol());

  m_doc->DoSetStrategyLabel(GetStrategy(support, player, strat), p_value);
}

wxSheetCellAttr gbtColPlayerWidget::GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const
{
  wxSheetCellAttr attr(GetSheetRefData()->m_defaultGridCellAttr);
  attr.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
  attr.SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
  attr.SetOrientation(wxHORIZONTAL);
  if (m_table->NumColPlayers() > 0) {
    attr.SetForegroundColour(
        m_doc->GetStyle().GetPlayerColor(m_table->GetColPlayer(p_coords.GetRow() + 1)));
    attr.SetReadOnly(m_doc->IsTree());
  }
  else {
    attr.SetForegroundColour(*wxBLACK);
    attr.SetReadOnly(true);
  }
  attr.SetBackgroundColour(*wxLIGHT_GREY);
  return attr;
}

void gbtColPlayerWidget::DrawCell(wxDC &p_dc, const wxSheetCoords &p_coords)
{
  gbtTableWidgetBase::DrawCell(p_dc, p_coords);

  if (!m_table->ShowDominance() || IsLabelCell(p_coords) || m_table->NumColPlayers() == 0) {
    return;
  }

  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  const int player = m_table->GetColPlayer(p_coords.GetRow() + 1);
  const int strat = m_table->ColToStrategy(p_coords.GetRow() + 1, p_coords.GetCol());
  const Gambit::GameStrategy strategy = GetStrategy(support, player, strat);

  if (support.IsDominated(strategy, false)) {
    const wxRect rect = CellToRect(p_coords);
    if (support.IsDominated(strategy, true)) {
      p_dc.SetPen(wxPen(m_doc->GetStyle().GetPlayerColor(player), 2, wxPENSTYLE_SOLID));
    }
    else {
      p_dc.SetPen(wxPen(m_doc->GetStyle().GetPlayerColor(player), 1, wxPENSTYLE_SHORT_DASH));
    }
    p_dc.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    p_dc.DrawLine(rect.x + rect.width, rect.y, rect.x, rect.y + rect.height);
  }
}

bool gbtColPlayerWidget::DropText(wxCoord p_x, wxCoord p_y, const wxString &p_text)
{
  if (p_text[0] == 'P') {
    long pl;
    p_text.Right(p_text.Length() - 1).ToLong(&pl);

    if (m_table->NumColPlayers() == 0) {
      m_table->SetColPlayer(1, pl);
      return true;
    }

    for (int col = 0; col < GetNumberCols(); col++) {
      const wxRect rect = CellToRect(wxSheetCoords(col, 0));

      if (p_y >= rect.y && p_y < rect.y + rect.height / 2) {
        m_table->SetColPlayer(col + 1, pl);
        return true;
      }
      else if (p_y >= rect.y + rect.height / 2 && p_y < rect.y + rect.height) {
        m_table->SetColPlayer(col + 2, pl);
        return true;
      }
    }
  }

  return false;
}

//=========================================================================
//                       class gbtPayoffsWidget
//=========================================================================

class gbtPayoffsWidget : public gbtTableWidgetBase {
private:
  gbtGameDocument *m_doc;
  gbtTableWidget *m_table;

  /// @name Overriding wxSheet members for data access
  //@{
  /// Returns the value in the cell
  wxString GetCellValue(const wxSheetCoords &) override;
  /// Sets the value in the cell, by editing the outcome
  void SetCellValue(const wxSheetCoords &, const wxString &) override;
  /// Returns the attributes of the cell
  wxSheetCellAttr GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const override;
  //@}

  /// @name Overriding wxSheet members for drawing behavior
  //@{
  /// Draws dark borders between contingencies
  void DrawCellBorder(wxDC &p_dc, const wxSheetCoords &p_coords) override;
  /// Overrides to draw dominance indicators
  void DrawCell(wxDC &p_dc, const wxSheetCoords &p_coords) override;
  //@}

  /// @name Overriding wxSheet members for event-handling behavior
  //@{
  /// Implement custom tab-traversal behavior
  void OnKeyDown(wxKeyEvent &);
  //@}

  /// Maps columns to corresponding player
  int ColToPlayer(int p_col) const;

public:
  gbtPayoffsWidget(gbtTableWidget *p_parent, gbtGameDocument *p_doc);

  /// @name Synchronizing with document state
  //@{
  void OnUpdate();
  //@}

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(gbtPayoffsWidget, gbtTableWidgetBase)
EVT_KEY_DOWN(gbtPayoffsWidget::OnKeyDown)
END_EVENT_TABLE()

gbtPayoffsWidget::gbtPayoffsWidget(gbtTableWidget *p_parent, gbtGameDocument *p_doc)
  : gbtTableWidgetBase(p_parent, wxID_ANY), m_doc(p_doc), m_table(p_parent)
{
  CreateGrid(0, 0);
  SetRowLabelWidth(1);
  SetColLabelHeight(1);
}

//
// Payoffs are ordered first by row players (in hierarchical order),
// followed by column players (in hierarchical order)
//
int gbtPayoffsWidget::ColToPlayer(int p_col) const
{
  const int index = p_col % m_doc->NumPlayers() + 1;
  if (index <= m_table->NumRowPlayers()) {
    return m_table->GetRowPlayer(index);
  }
  else {
    return m_table->GetColPlayer(index - m_table->NumRowPlayers());
  }
}

void gbtPayoffsWidget::OnUpdate()
{
  const int newCols = m_table->NumColContingencies() * m_doc->NumPlayers();
  if (newCols > GetNumberCols()) {
    InsertCols(0, newCols - GetNumberCols());
  }
  if (newCols < GetNumberCols()) {
    DeleteCols(0, GetNumberCols() - newCols);
  }

  const int newRows = m_table->NumRowContingencies();
  if (newRows > GetNumberRows()) {
    InsertRows(0, newRows - GetNumberRows());
  }
  if (newRows < GetNumberRows()) {
    DeleteRows(0, GetNumberRows() - newRows);
  }

  Refresh();
}

wxString gbtPayoffsWidget::GetCellValue(const wxSheetCoords &p_coords)
{
  if (IsLabelCell(p_coords)) {
    return wxT("");
  }

  const Gambit::PureStrategyProfile profile = m_table->CellToProfile(p_coords);
  auto player = m_doc->GetGame()->GetPlayer(ColToPlayer(p_coords.GetCol()));
  return {Gambit::lexical_cast<std::string>(profile->GetPayoff(player)).c_str(), *wxConvCurrent};
}

void gbtPayoffsWidget::SetCellValue(const wxSheetCoords &p_coords, const wxString &p_value)
{
  Gambit::PureStrategyProfile profile = m_table->CellToProfile(p_coords);
  Gambit::GameOutcome outcome = profile->GetOutcome();
  if (!outcome) {
    m_doc->DoNewOutcome(profile);
    profile = m_table->CellToProfile(p_coords);
    outcome = profile->GetOutcome();
  }
  const int player = ColToPlayer(p_coords.GetCol());
  try {
    m_doc->DoSetPayoff(outcome, player, p_value);
  }
  catch (ValueException &) {
    // For the moment, we will just silently discard edits which
    // give payoffs that are not valid numbers
    return;
  }
  catch (std::exception &ex) {
    gbtExceptionDialog(this, ex.what()).ShowModal();
  }
}

wxSheetCellAttr gbtPayoffsWidget::GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const
{
  if (IsLabelCell(p_coords)) {
    wxSheetCellAttr attr(GetSheetRefData()->m_defaultColLabelAttr);
    attr.SetBackgroundColour(*wxLIGHT_GREY);
    return attr;
  }

  wxSheetCellAttr attr(GetSheetRefData()->m_defaultGridCellAttr);
  attr.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
  attr.SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
  attr.SetOrientation(wxHORIZONTAL);
  const int player = ColToPlayer(p_coords.GetCol());
  attr.SetForegroundColour(m_doc->GetStyle().GetPlayerColor(player));
  attr.SetRenderer(wxSheetCellRenderer(new gbtRationalRendererRefData()));
  attr.SetEditor(wxSheetCellEditor(new gbtRationalEditorRefData()));
  attr.SetReadOnly(m_doc->IsTree());
  return attr;
}

void gbtPayoffsWidget::DrawCellBorder(wxDC &p_dc, const wxSheetCoords &p_coords)
{
  gbtTableWidgetBase::DrawCellBorder(p_dc, p_coords);

  const wxRect rect(CellToRect(p_coords));
  if (rect.width < 1 || rect.height < 1) {
    return;
  }

  p_dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SOLID));

  // Draw the dark border to the right of the last column of a contingency
  if ((p_coords.GetCol() + 1) % m_doc->GetGame()->NumPlayers() == 0) {
    p_dc.DrawLine(rect.x + rect.width, rect.y, rect.x + rect.width, rect.y + rect.height + 1);
  }

  // Draw the bottom border -- currently always dark
  p_dc.DrawLine(rect.x - 1, rect.y + rect.height, rect.x + rect.width, rect.y + rect.height);

  // Draw the top border for the first row
  if (p_coords.GetRow() == 0) {
    p_dc.DrawLine(rect.x - 1, rect.y, rect.x + rect.width, rect.y);
  }
}

void gbtPayoffsWidget::DrawCell(wxDC &p_dc, const wxSheetCoords &p_coords)
{
  gbtTableWidgetBase::DrawCell(p_dc, p_coords);

  if (!m_table->ShowDominance() || IsLabelCell(p_coords)) {
    return;
  }

  const Gambit::PureStrategyProfile profile = m_table->CellToProfile(p_coords);
  auto player = m_doc->GetGame()->GetPlayer(ColToPlayer(p_coords.GetCol()));
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();

  if (support.IsDominated(profile->GetStrategy(player), false)) {
    const wxRect rect = CellToRect(p_coords);
    if (support.IsDominated(profile->GetStrategy(player), true)) {
      p_dc.SetPen(
          wxPen(m_doc->GetStyle().GetPlayerColor(player->GetNumber()), 2, wxPENSTYLE_SOLID));
    }
    else {
      p_dc.SetPen(
          wxPen(m_doc->GetStyle().GetPlayerColor(player->GetNumber()), 1, wxPENSTYLE_SHORT_DASH));
    }
    p_dc.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    p_dc.DrawLine(rect.x + rect.width, rect.y, rect.x, rect.y + rect.height);
  }
}

//!
//! Overriding default wxSheet behavior: when editing, accepting the
//! edited value via the TAB key automatically moves the cursor to
//! the right *and* creates the editor in the next cell.  In addition,
//! tabbing off the rightmost cell entry automatically "wraps" to the
//! next row.
//!
void gbtPayoffsWidget::OnKeyDown(wxKeyEvent &p_event)
{
  if (GetNumberRows() && GetNumberCols()) {
    switch (p_event.GetKeyCode()) {
    case WXK_TAB: {
      if (IsCellEditControlCreated()) {
        DisableCellEditControl(true);

        int newRow = GetGridCursorRow(), newCol = GetGridCursorCol();

        if (p_event.ShiftDown()) {
          newCol--;
          if (newCol < 0) {
            newCol = GetNumberCols() - 1;
            newRow--;
            if (newRow < 0) {
              newRow = GetNumberRows() - 1;
            }
          }
        }
        else {
          newCol++;
          if (newCol >= GetNumberCols()) {
            newCol = 0;
            newRow++;
            if (newRow >= GetNumberRows()) {
              newRow = 0;
            }
          }
        }
        SetGridCursorCell(wxSheetCoords(newRow, newCol));
        MakeCellVisible(GetGridCursorCell());
        EnableCellEditControl(GetGridCursorCell());
      }
      break;
    }
    default:
      p_event.Skip();
    }
  }
}

//=========================================================================
//                       gbtTableWidget: Lifecycle
//=========================================================================

gbtTableWidget::gbtTableWidget(gbtNfgPanel *p_parent, wxWindowID p_id, gbtGameDocument *p_doc)
  : wxPanel(p_parent, p_id), m_doc(p_doc), m_nfgPanel(p_parent), m_payoffSheet(nullptr),
    m_rowSheet(nullptr), m_colSheet(nullptr)
{
  m_rowPlayers.push_back(1);
  m_colPlayers.push_back(2);

  // These depend on the row and column player lists having been populated,
  // which suggests some refactoring ought to be done as to where/how those
  // row and column players are recorded
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  m_payoffSheet = new gbtPayoffsWidget(this, p_doc);
  m_rowSheet = new gbtRowPlayerWidget(this, p_doc);
  m_colSheet = new gbtColPlayerWidget(this, p_doc);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
  m_payoffSheet->SetGridLineColour(*wxWHITE);

  auto *topSizer = new wxFlexGridSizer(2, 2, 0, 0);
  topSizer->AddGrowableRow(1);
  topSizer->AddGrowableCol(1);
  topSizer->Add(new wxPanel(this, wxID_ANY));
  topSizer->Add(m_colSheet, 1, wxEXPAND, 0);
  topSizer->Add(m_rowSheet, 1, wxEXPAND, 0);
  topSizer->Add(m_payoffSheet, 1, wxEXPAND, 0);

  SetSizer(topSizer);
  Layout();

  Connect(m_colSheet->GetId(), wxEVT_SHEET_VIEW_CHANGED,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnRowSheetScroll))));

  Connect(m_colSheet->GetId(), wxEVT_SHEET_VIEW_CHANGED,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnColSheetScroll))));

  Connect(m_payoffSheet->GetId(), wxEVT_SHEET_VIEW_CHANGED,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnPayoffScroll))));

  Connect(m_rowSheet->GetId(), wxEVT_SHEET_ROW_SIZE,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnRowSheetRow))));

  Connect(m_payoffSheet->GetId(), wxEVT_SHEET_ROW_SIZE,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnPayoffRow))));

  Connect(m_colSheet->GetId(), wxEVT_SHEET_COL_SIZE,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnColSheetColumn))));

  Connect(m_payoffSheet->GetId(), wxEVT_SHEET_COL_SIZE,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnPayoffColumn))));

  Connect(m_rowSheet->GetId(), wxEVT_SHEET_COL_SIZE,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnRowSheetColumn))));

  Connect(m_colSheet->GetId(), wxEVT_SHEET_ROW_SIZE,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnColSheetRow))));

  Connect(m_rowSheet->GetId(), wxEVT_SHEET_EDITOR_ENABLED,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnBeginEdit))));
  Connect(m_colSheet->GetId(), wxEVT_SHEET_EDITOR_ENABLED,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnBeginEdit))));
  Connect(m_payoffSheet->GetId(), wxEVT_SHEET_EDITOR_ENABLED,
          (wxObjectEventFunction) reinterpret_cast<wxEventFunction>(wxStaticCastEvent(
              wxSheetEventFunction, wxSheetEventFunction(&gbtTableWidget::OnBeginEdit))));
}

//!
//! Scroll row and column label sheets to match payoff sheet origin.
//! Note that wxSheet uses coordinates of -1 to indicate no scroll.
//!
//@{
void gbtTableWidget::OnRowSheetScroll(wxSheetEvent &)
{
  m_payoffSheet->SetGridOrigin(-1, m_rowSheet->GetGridOrigin().y);
}

void gbtTableWidget::OnColSheetScroll(wxSheetEvent &)
{
  m_payoffSheet->SetGridOrigin(m_colSheet->GetGridOrigin().x, -1);
}

void gbtTableWidget::OnPayoffScroll(wxSheetEvent &)
{
  m_colSheet->SetGridOrigin(m_payoffSheet->GetGridOrigin().x, 0);
  m_rowSheet->SetGridOrigin(0, m_payoffSheet->GetGridOrigin().y);
}
//@}

//!
//! These keep the row heights synchronized
//!
//@{
void gbtTableWidget::OnRowSheetRow(wxSheetEvent &p_event)
{
  const int height = m_rowSheet->GetRowHeight(p_event.GetRow());
  m_payoffSheet->SetDefaultRowHeight(height, true);
  m_payoffSheet->AdjustScrollbars();
  m_payoffSheet->Refresh();
  m_rowSheet->SetDefaultRowHeight(height, true);
  m_rowSheet->Refresh();
}

void gbtTableWidget::OnPayoffRow(wxSheetEvent &p_event)
{
  const int height = m_payoffSheet->GetRowHeight(p_event.GetRow());
  m_payoffSheet->SetDefaultRowHeight(height, true);
  m_payoffSheet->AdjustScrollbars();
  m_payoffSheet->Refresh();
  m_rowSheet->SetDefaultRowHeight(height, true);
  m_rowSheet->Refresh();
}
//@}

//!
//! These keep the column widths synchronized
//!
//@{
void gbtTableWidget::OnColSheetColumn(wxSheetEvent &p_event)
{
  const int width = m_colSheet->GetColWidth(p_event.GetCol());
  m_payoffSheet->SetDefaultColWidth(width, true);
  m_payoffSheet->AdjustScrollbars();
  m_payoffSheet->Refresh();
  m_colSheet->SetDefaultColWidth(width, true);
  m_colSheet->Refresh();
}

void gbtTableWidget::OnPayoffColumn(wxSheetEvent &p_event)
{
  const int width = m_payoffSheet->GetColWidth(p_event.GetCol());
  m_payoffSheet->SetDefaultColWidth(width, true);
  m_payoffSheet->AdjustScrollbars();
  m_payoffSheet->Refresh();
  m_colSheet->SetDefaultColWidth(width, true);
  m_colSheet->Refresh();
}
//@}

//!
//! These handle correctly sizing the label windows
//!
//@{
void gbtTableWidget::OnRowSheetColumn(wxSheetEvent &p_event)
{
  m_rowSheet->SetDefaultColWidth(m_rowSheet->GetColWidth(p_event.GetCol()), true);
  GetSizer()->Layout();
}

void gbtTableWidget::OnColSheetRow(wxSheetEvent &p_event)
{
  m_colSheet->SetDefaultRowHeight(m_colSheet->GetRowHeight(p_event.GetRow()), true);
  GetSizer()->Layout();
}
//@}

//!
//! This alerts the document to have any other windows post their pending
//! edits.
//!
void gbtTableWidget::OnBeginEdit(wxSheetEvent &) { m_doc->PostPendingChanges(); }

void gbtTableWidget::OnUpdate()
{
  if (m_doc->NumPlayers() > m_rowPlayers.size() + m_colPlayers.size()) {
    for (size_t pl = 1; pl <= m_doc->NumPlayers(); pl++) {
      if (!contains(m_rowPlayers, pl) && !contains(m_colPlayers, pl)) {
        m_rowPlayers.push_back(pl);
      }
    }
  }
  else if (m_doc->NumPlayers() < m_rowPlayers.size() + m_colPlayers.size()) {
    for (size_t i = 1; i <= m_rowPlayers.size(); i++) {
      if (m_rowPlayers[i] > static_cast<int>(m_doc->NumPlayers())) {
        erase_atindex(m_rowPlayers, i--);
      }
    }

    for (size_t i = 1; i <= m_colPlayers.size(); i++) {
      if (m_colPlayers[i] > static_cast<int>(m_doc->NumPlayers())) {
        erase_atindex(m_colPlayers, i--);
      }
    }
  }

  dynamic_cast<gbtPayoffsWidget *>(m_payoffSheet)->OnUpdate();

  // We add margins to the player labels to match the scrollbars,
  // so scrolling matches up
  m_colSheet->SetMargins(dynamic_cast<gbtPayoffsWidget *>(m_payoffSheet)
                             ->GetVerticalScrollBar()
                             ->GetSize()
                             .GetWidth(),
                         -1);
  m_rowSheet->SetMargins(-1, dynamic_cast<gbtPayoffsWidget *>(m_payoffSheet)
                                 ->GetHorizontalScrollBar()
                                 ->GetSize()
                                 .GetHeight());

  dynamic_cast<gbtRowPlayerWidget *>(m_rowSheet)->OnUpdate();
  dynamic_cast<gbtColPlayerWidget *>(m_colSheet)->OnUpdate();
  GetSizer()->Layout();
}

void gbtTableWidget::PostPendingChanges()
{
  if (m_payoffSheet->IsCellEditControlShown()) {
    m_payoffSheet->DisableCellEditControl(true);
  }

  if (m_rowSheet->IsCellEditControlShown()) {
    m_rowSheet->DisableCellEditControl(true);
  }

  if (m_colSheet->IsCellEditControlShown()) {
    m_colSheet->DisableCellEditControl(true);
  }
}

bool gbtTableWidget::ShowDominance() const { return m_nfgPanel->IsDominanceShown(); }

//=========================================================================
//                      gbtTableWidget: View state
//=========================================================================

void gbtTableWidget::SetRowPlayer(int index, int pl)
{
  if (contains(m_colPlayers, pl)) {
    m_colPlayers.erase(std::find(m_colPlayers.begin(), m_colPlayers.end(), pl));
  }
  Array<int> newPlayers;
  for (const auto &player : m_rowPlayers) {
    if (static_cast<int>(newPlayers.size()) == index - 1) {
      newPlayers.push_back(pl);
    }
    else if (player != pl) {
      newPlayers.push_back(player);
    }
  }
  m_rowPlayers = newPlayers;
  OnUpdate();
}

int NumStrategies(const StrategySupportProfile &p_profile, int p_player)
{
  return p_profile.GetStrategies(p_profile.GetGame()->GetPlayer(p_player)).size();
}

int gbtTableWidget::NumRowContingencies() const
{
  int ncont = 1;
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  for (int i = 1; i <= NumRowPlayers(); ncont *= NumStrategies(support, GetRowPlayer(i++)))
    ;
  return ncont;
}

int gbtTableWidget::NumRowsSpanned(int index) const
{
  int ncont = 1;
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  for (int i = index + 1; i <= NumRowPlayers(); ncont *= NumStrategies(support, GetRowPlayer(i++)))
    ;
  return ncont;
}

int gbtTableWidget::RowToStrategy(int player, int row) const
{
  const int strat = row / NumRowsSpanned(player);
  return (strat % NumStrategies(m_doc->GetNfgSupport(), GetRowPlayer(player)) + 1);
}

void gbtTableWidget::SetColPlayer(int index, int pl)
{
  if (contains(m_rowPlayers, pl)) {
    m_rowPlayers.erase(std::find(m_rowPlayers.begin(), m_rowPlayers.end(), pl));
  }
  Array<int> newPlayers;
  for (const auto &player : m_colPlayers) {
    if (static_cast<int>(newPlayers.size()) == index - 1) {
      newPlayers.push_back(pl);
    }
    else if (player != pl) {
      newPlayers.push_back(player);
    }
  }
  m_colPlayers = newPlayers;
  OnUpdate();
}

int gbtTableWidget::NumColContingencies() const
{
  int ncont = 1;
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  for (int i = 1; i <= NumColPlayers(); ncont *= NumStrategies(support, GetColPlayer(i++)))
    ;
  return ncont;
}

int gbtTableWidget::NumColsSpanned(int index) const
{
  int ncont = 1;
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();
  for (int i = index + 1; i <= NumColPlayers(); ncont *= NumStrategies(support, GetColPlayer(i++)))
    ;
  return ncont;
}

int gbtTableWidget::ColToStrategy(int player, int col) const
{
  const int strat = col / m_doc->NumPlayers() / NumColsSpanned(player);
  return (strat % NumStrategies(m_doc->GetNfgSupport(), GetColPlayer(player)) + 1);
}

Gambit::PureStrategyProfile gbtTableWidget::CellToProfile(const wxSheetCoords &p_coords) const
{
  const Gambit::StrategySupportProfile &support = m_doc->GetNfgSupport();

  const Gambit::PureStrategyProfile profile = m_doc->GetGame()->NewPureStrategyProfile();
  for (int i = 1; i <= NumRowPlayers(); i++) {
    const int player = GetRowPlayer(i);
    profile->SetStrategy(GetStrategy(support, player, RowToStrategy(i, p_coords.GetRow())));
  }

  for (int i = 1; i <= NumColPlayers(); i++) {
    const int player = GetColPlayer(i);
    profile->SetStrategy(GetStrategy(support, player, ColToStrategy(i, p_coords.GetCol())));
  }

  return profile;
}

class gbtNfgPrintout : public wxPrintout {
private:
  gbtTableWidget *m_table;

public:
  gbtNfgPrintout(gbtTableWidget *p_table, const wxString &p_label)
    : wxPrintout(p_label), m_table(p_table)
  {
  }
  ~gbtNfgPrintout() override = default;

  bool OnPrintPage(int) override
  {
    m_table->RenderGame(*GetDC(), 50, 50);
    return true;
  }
  bool HasPage(int page) override { return (page <= 1); }
  void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) override
  {
    *minPage = 1;
    *maxPage = 1;
    *selPageFrom = 1;
    *selPageTo = 1;
  }
};

wxPrintout *gbtTableWidget::GetPrintout()
{
  return new gbtNfgPrintout(this, wxString(m_doc->GetGame()->GetTitle().c_str(), *wxConvCurrent));
}

bool gbtTableWidget::GetBitmap(wxBitmap &p_bitmap, int p_marginX, int p_marginY)
{
  const int width =
      (m_rowSheet->CellToRect(wxSheetCoords(0, m_rowSheet->GetNumberCols() - 1)).GetRight() +
       m_colSheet->CellToRect(wxSheetCoords(0, m_colSheet->GetNumberCols() - 1)).GetRight() +
       2 * p_marginX);
  const int height =
      (m_rowSheet->CellToRect(wxSheetCoords(m_rowSheet->GetNumberRows() - 1, 0)).GetBottom() +
       m_colSheet->CellToRect(wxSheetCoords(m_colSheet->GetNumberRows() - 1, 0)).GetBottom() +
       2 * p_marginY);

  if (width > 65000 || height > 65000) {
    // This is just too huge to export to graphics
    return false;
  }

  wxMemoryDC dc;
  p_bitmap = wxBitmap(width, height);
  dc.SelectObject(p_bitmap);
  dc.Clear();
  RenderGame(dc, p_marginX, p_marginY);
  return true;
}

void gbtTableWidget::GetSVG(const wxString &p_filename, int p_marginX, int p_marginY)
{
  const int width =
      (m_rowSheet->CellToRect(wxSheetCoords(0, m_rowSheet->GetNumberCols() - 1)).GetRight() +
       m_colSheet->CellToRect(wxSheetCoords(0, m_colSheet->GetNumberCols() - 1)).GetRight() +
       2 * p_marginX);
  const int height =
      (m_rowSheet->CellToRect(wxSheetCoords(m_rowSheet->GetNumberRows() - 1, 0)).GetBottom() +
       m_colSheet->CellToRect(wxSheetCoords(m_colSheet->GetNumberRows() - 1, 0)).GetBottom() +
       2 * p_marginY);

  wxSVGFileDC dc(p_filename, width, height);
  // For some reason, this needs to be initialized
  dc.SetLogicalScale(1.0, 1.0);
  RenderGame(dc, p_marginX, p_marginY);
}

void gbtTableWidget::RenderGame(wxDC &p_dc, int p_marginX, int p_marginY)
{
  // The size of the image to be drawn
  const int maxX =
      (m_rowSheet->CellToRect(wxSheetCoords(0, m_rowSheet->GetNumberCols() - 1)).GetRight() +
       m_colSheet->CellToRect(wxSheetCoords(0, m_colSheet->GetNumberCols() - 1)).GetRight());
  const int maxY =
      (m_rowSheet->CellToRect(wxSheetCoords(m_rowSheet->GetNumberRows() - 1, 0)).GetBottom() +
       m_colSheet->CellToRect(wxSheetCoords(m_colSheet->GetNumberRows() - 1, 0)).GetBottom());

  // Get the size of the DC in pixels
  wxCoord w, h;
  p_dc.GetSize(&w, &h);

  // Calculate a scaling factor
  const double scaleX = (double)w / (double)(maxX + 2 * p_marginX);
  const double scaleY = (double)h / (double)(maxY + 2 * p_marginY);
  const double scale = (scaleX < scaleY) ? scaleX : scaleY;
  // Here, zooming in is often a good idea, since the number of pixels
  // on a page is generally quite large
  p_dc.SetUserScale(scale, scale);

  // Calculate the position on the DC to center the tree
  auto posX = (double)((w - (maxX * scale)) / 2.0);
  auto posY = (double)((h - (maxY * scale)) / 2.0);

  // The X and Y coordinates of the upper left of the payoff table
  const int payoffX =
      (int)(m_rowSheet->CellToRect(wxSheetCoords(0, m_rowSheet->GetNumberCols() - 1)).GetRight() *
            scale);
  const int payoffY =
      (int)(m_colSheet->CellToRect(wxSheetCoords(m_colSheet->GetNumberRows() - 1, 0)).GetBottom() *
            scale);

  p_dc.SetDeviceOrigin((int)posX, payoffY + (int)posY);
  m_rowSheet->DrawGridCells(
      p_dc, wxSheetBlock(0, 0, m_rowSheet->GetNumberRows(), m_rowSheet->GetNumberCols()));

  p_dc.SetDeviceOrigin(payoffX + (int)posX, (int)posY);
  m_colSheet->DrawGridCells(
      p_dc, wxSheetBlock(0, 0, m_colSheet->GetNumberRows(), m_colSheet->GetNumberCols()));

  p_dc.SetDeviceOrigin(payoffX + (int)posX, payoffY + (int)posY);
  m_payoffSheet->DrawGridCells(
      p_dc, wxSheetBlock(0, 0, m_payoffSheet->GetNumberRows(), m_payoffSheet->GetNumberCols()));
}
