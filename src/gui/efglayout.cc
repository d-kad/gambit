//
// This file is part of Gambit
// Copyright (c) 1994-2025, The Gambit Project (https://www.gambit-project.org)
//
// FILE: src/gui/efglayout.cc
// Implementation of tree layout representation
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

#include <cmath>
#include <algorithm> // for std::min, std::max

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif // WX_PRECOMP

#include "gambit.h"
#include "efgdisplay.h"

using namespace Gambit;

//-----------------------------------------------------------------------
//                   class gbtNodeEntry: Member functions
//-----------------------------------------------------------------------

gbtNodeEntry::gbtNodeEntry(GameNode p_node)
  : m_node(p_node), m_parent(nullptr), m_x(-1), m_y(-1), m_nextMember(nullptr), m_inSupport(true),
    m_size(20), m_token(GBT_NODE_TOKEN_CIRCLE), m_branchStyle(GBT_BRANCH_STYLE_LINE),
    m_branchLabel(GBT_BRANCH_LABEL_HORIZONTAL), m_branchLength(0), m_sublevel(0), m_actionProb(0)
{
}

int gbtNodeEntry::GetChildNumber() const
{
  if (m_node->GetParent()) {
    return m_node->GetPriorAction()->GetNumber();
  }
  else {
    return 0;
  }
}

//
// Draws the node token itself, as well as the incoming branch
// (if not the root node)
//
void gbtNodeEntry::Draw(wxDC &p_dc, GameNode p_selection, bool p_noHints) const
{
  if (m_node->GetParent() && m_inSupport) {
    DrawIncomingBranch(p_dc);
  }
  else {
    m_branchAboveRect = wxRect();
    m_branchBelowRect = wxRect();
  }

  p_dc.SetPen(
      *wxThePenList->FindOrCreatePen(m_color, (p_selection == m_node) ? 6 : 3, wxPENSTYLE_SOLID));
  p_dc.SetTextForeground(m_color);

  if (m_token == GBT_NODE_TOKEN_LINE) {
    p_dc.DrawLine(m_x, m_y, m_x + m_size, m_y);
    if (m_branchStyle == GBT_BRANCH_STYLE_FORKTINE) {
      // "classic" Gambit style: draw a small 'token' to separate
      // the fork from the node
      p_dc.DrawEllipse(m_x - 1, m_y - 1, 3, 3);
    }
  }
  else if (m_token == GBT_NODE_TOKEN_BOX) {
    p_dc.SetBrush(*wxWHITE_BRUSH);
    p_dc.DrawRectangle(m_x, m_y - m_size / 2, m_size, m_size);
  }
  else if (m_token == GBT_NODE_TOKEN_DIAMOND) {
    wxPoint points[4] = {wxPoint(m_x + m_size / 2, m_y - m_size / 2), wxPoint(m_x, m_y),
                         wxPoint(m_x + m_size / 2, m_y + m_size / 2), wxPoint(m_x + m_size, m_y)};
    p_dc.SetBrush(*wxWHITE_BRUSH);
    p_dc.DrawPolygon(4, points);
  }
  else if (m_token == GBT_NODE_TOKEN_DOT) {
    p_dc.SetBrush(wxBrush(m_color, wxBRUSHSTYLE_SOLID));
    p_dc.DrawEllipse(m_x, m_y - m_size / 2, m_size, m_size);
  }
  else {
    // Default: draw circles
    p_dc.SetBrush(*wxWHITE_BRUSH);
    p_dc.DrawEllipse(m_x, m_y - m_size / 2, m_size, m_size);
  }

  int textWidth, textHeight;
  p_dc.SetFont(m_nodeAboveFont);
  p_dc.GetTextExtent(m_nodeAboveLabel, &textWidth, &textHeight);
  p_dc.DrawText(m_nodeAboveLabel, m_x + (m_size - textWidth) / 2, m_y - textHeight - 9);
  p_dc.SetFont(m_nodeBelowFont);
  p_dc.GetTextExtent(m_nodeBelowLabel, &textWidth, &textHeight);
  p_dc.DrawText(m_nodeBelowLabel, m_x + (m_size - textWidth) / 2, m_y + 9);

  p_dc.SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
  DrawOutcome(p_dc, p_noHints);
}

void gbtNodeEntry::DrawIncomingBranch(wxDC &p_dc) const
{
  const int xStart = m_parent->m_x + m_parent->m_size;
  const int xEnd = m_x;
  const int yStart = m_parent->m_y;
  const int yEnd = m_y;

  p_dc.SetPen(*wxThePenList->FindOrCreatePen(m_parent->m_color, 4, wxPENSTYLE_SOLID));
  p_dc.SetTextForeground(m_parent->m_color);

  if (m_branchStyle == GBT_BRANCH_STYLE_LINE) {
    p_dc.DrawLine(xStart, yStart, xEnd, yEnd);

    // Draw in the highlight indicating action probabilities
    if (m_actionProb >= Rational(0)) {
      p_dc.SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, 4, wxPENSTYLE_SOLID));
      p_dc.DrawLine(xStart, yStart, xStart + (int)((double)(xEnd - xStart) * (double)m_actionProb),
                    yStart + (int)((double)(yEnd - yStart) * (double)m_actionProb));
    }

    int textWidth, textHeight;
    p_dc.SetFont(m_branchAboveFont);
    p_dc.GetTextExtent(m_branchAboveLabel, &textWidth, &textHeight);

    // The angle of the branch
    const double theta = -atan((double)(yEnd - yStart) / (double)(xEnd - xStart));
    // The "centerpoint" of the branch
    const int xbar = (xStart + xEnd) / 2;
    const int ybar = (yStart + yEnd) / 2;

    if (m_branchLabel == GBT_BRANCH_LABEL_HORIZONTAL) {
      if (yStart >= yEnd) {
        p_dc.DrawText(m_branchAboveLabel, xbar - textWidth / 2,
                      ybar - textHeight + textWidth / 2 * (yEnd - yStart) / (xEnd - xStart));
        m_branchAboveRect =
            wxRect(xbar - textWidth / 2,
                   ybar - textHeight + textWidth / 2 * (yEnd - yStart) / (xEnd - xStart),
                   textWidth, textHeight);
      }
      else {
        p_dc.DrawText(m_branchAboveLabel, xbar - textWidth / 2,
                      ybar - textHeight - textWidth / 2 * (yEnd - yStart) / (xEnd - xStart));
        m_branchAboveRect =
            wxRect(xbar - textWidth / 2,
                   ybar - textHeight - textWidth / 2 * (yEnd - yStart) / (xEnd - xStart),
                   textWidth, textHeight);
      }
    }
    else {
      // Draw the text rotated appropriately
      p_dc.DrawRotatedText(m_branchAboveLabel,
                           (int)((double)xbar - (double)textHeight * sin(theta) -
                                 (double)textWidth * cos(theta) / 2.0),
                           (int)((double)ybar - (double)textHeight * cos(theta) +
                                 (double)textWidth * sin(theta) / 2.0),
                           theta * 180.0 / 3.14159);
      m_branchAboveRect = wxRect();
    }

    p_dc.SetFont(m_branchBelowFont);
    p_dc.GetTextExtent(m_branchBelowLabel, &textWidth, &textHeight);

    if (m_branchLabel == GBT_BRANCH_LABEL_HORIZONTAL) {
      if (yStart >= yEnd) {
        p_dc.DrawText(m_branchBelowLabel, xbar - textWidth / 2,
                      ybar - textWidth / 2 * (yEnd - yStart) / (xEnd - xStart));
        m_branchBelowRect =
            wxRect(xbar - textWidth / 2, ybar - textWidth / 2 * (yEnd - yStart) / (xEnd - xStart),
                   textWidth, textHeight);
      }
      else {
        p_dc.DrawText(m_branchBelowLabel, xbar - textWidth / 2,
                      ybar + textWidth / 2 * (yEnd - yStart) / (xEnd - xStart));
        m_branchBelowRect =
            wxRect(xbar - textWidth / 2, ybar + textWidth / 2 * (yEnd - yStart) / (xEnd - xStart),
                   textWidth, textHeight);
      }
    }
    else {
      // Draw the text rotated appropriately
      p_dc.DrawRotatedText(
          m_branchBelowLabel, (int)((double)xbar - (double)textWidth * cos(theta) / 2.0),
          (int)((double)ybar + (double)textWidth * sin(theta) / 2.0), theta * 180.0 / 3.14159);
      m_branchBelowRect = wxRect();
    }
  }
  else {
    // Old style fork-tine
    p_dc.DrawLine(xStart, yStart, xStart + m_branchLength, yEnd);
    p_dc.DrawLine(xStart + m_branchLength, yEnd, xEnd, yEnd);

    // Draw in the highlight indicating action probabilities
    if (m_actionProb >= Rational(0)) {
      p_dc.SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, 2, wxPENSTYLE_SOLID));
      p_dc.DrawLine(xStart, yStart, xStart + (int)((double)m_branchLength * (double)m_actionProb),
                    yStart + (int)((double)(yEnd - yStart) * (double)m_actionProb));
    }

    int textWidth, textHeight;
    p_dc.SetFont(m_branchAboveFont);
    p_dc.GetTextExtent(m_branchAboveLabel, &textWidth, &textHeight);
    p_dc.DrawText(m_branchAboveLabel, xStart + m_branchLength + 3, yEnd - textHeight - 3);
    m_branchAboveRect =
        wxRect(xStart + m_branchLength + 3, yEnd - textHeight - 3, textWidth, textHeight);

    p_dc.SetFont(m_branchBelowFont);
    p_dc.GetTextExtent(m_branchBelowLabel, &textWidth, &textHeight);
    p_dc.DrawText(m_branchBelowLabel, xStart + m_branchLength + 3, yEnd + 3);
    m_branchBelowRect = wxRect(xStart + m_branchLength + 3, yEnd + +3, textWidth, textHeight);
  }
}

static wxPoint DrawFraction(wxDC &p_dc, wxPoint p_point, const Rational &p_value)
{
  p_dc.SetFont(wxFont(7, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

  int numWidth, numHeight;
  const wxString num =
      wxString(lexical_cast<std::string>(p_value.numerator()).c_str(), *wxConvCurrent);
  p_dc.GetTextExtent(num, &numWidth, &numHeight);

  int denWidth, denHeight;
  const wxString den =
      wxString(lexical_cast<std::string>(p_value.denominator()).c_str(), *wxConvCurrent);
  p_dc.GetTextExtent(den, &denWidth, &denHeight);

  const int width = ((numWidth > denWidth) ? numWidth : denWidth);

  p_dc.DrawLine(p_point.x, p_point.y, p_point.x + width + 4, p_point.y);
  p_dc.DrawText(num, p_point.x + 2 + (width - numWidth) / 2, p_point.y - 2 - numHeight);
  p_dc.DrawText(den, p_point.x + 2 + (width - denWidth) / 2, p_point.y + 2);

  p_point.x += width + 14;
  return p_point;
}

void gbtNodeEntry::DrawOutcome(wxDC &p_dc, bool p_noHints) const
{
  wxPoint point(m_x + m_size + 20, m_y);

  const GameOutcome outcome = m_node->GetOutcome();
  if (!outcome) {
    if (p_noHints) {
      return;
    }

    int width, height;
    p_dc.SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD));
    p_dc.SetTextForeground(*wxLIGHT_GREY);
    p_dc.GetTextExtent(wxT("(u)"), &width, &height);
    p_dc.DrawText(wxT("(u)"), point.x, point.y - height / 2);
    m_outcomeRect = wxRect(point.x, point.y - height / 2, width, height);
    m_payoffRect = Array<wxRect>();
    return;
  }

  int width, height = 25;
  m_payoffRect = Array<wxRect>();
  for (const auto &player : m_node->GetGame()->GetPlayers()) {
    p_dc.SetTextForeground(m_style->GetPlayerColor(player->GetNumber()));

    const auto &payoff = outcome->GetPayoff<std::string>(player);

    if (payoff.find('/') != std::string::npos) {
      p_dc.SetPen(wxPen(m_style->GetPlayerColor(player->GetNumber()), 1, wxPENSTYLE_SOLID));
      const int oldX = point.x;
      point = DrawFraction(p_dc, point, outcome->GetPayoff<Rational>(player));
      m_payoffRect.push_back(wxRect(oldX - 5, point.y - height / 2, point.x - oldX + 10, height));
    }
    else {
      const wxString label = wxString(payoff.c_str(), *wxConvCurrent);
      p_dc.SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
      p_dc.GetTextExtent(label, &width, &height);
      p_dc.DrawText(label, point.x, point.y - height / 2);
      m_payoffRect.push_back(wxRect(point.x - 5, point.y - height / 2, width + 10, height));
      point.x += width + 10;
    }
  }

  if (height == 0) {
    // Happens if all payoffs are fractional
    height = 25;
  }

  m_outcomeRect =
      wxRect(m_x + m_size + 20, m_y - height / 2, point.x - (m_x + m_size + 20), height);
}

bool gbtNodeEntry::NodeHitTest(int p_x, int p_y) const
{
  if (p_x < m_x || p_x >= m_x + m_size) {
    return false;
  }

  if (m_token == GBT_NODE_TOKEN_LINE) {
    const int DELTA = 8; // a fudge factor for "almost" hitting the node
    return (p_y >= m_y - DELTA && p_y <= m_y + DELTA);
  }
  else {
    return (p_y >= m_y - m_size / 2 && p_y <= m_y + m_size / 2);
  }
}

//-----------------------------------------------------------------------
//                class gbtTreeLayout: Member functions
//-----------------------------------------------------------------------

gbtTreeLayout::gbtTreeLayout(gbtEfgDisplay *p_parent, gbtGameDocument *p_doc)
  : gbtGameView(p_doc),
    /* m_parent(p_parent),*/ m_infosetSpacing(40), c_leftMargin(20), c_topMargin(40)
{
}

GameNode gbtTreeLayout::NodeHitTest(int p_x, int p_y) const
{
  for (const auto &entry : m_nodeList) {
    if (entry->NodeHitTest(p_x, p_y)) {
      return entry->GetNode();
    }
  }
  return nullptr;
}

GameNode gbtTreeLayout::OutcomeHitTest(int p_x, int p_y) const
{
  for (const auto &entry : m_nodeList) {
    if (entry->OutcomeHitTest(p_x, p_y)) {
      return entry->GetNode();
    }
  }
  return nullptr;
}

GameNode gbtTreeLayout::BranchAboveHitTest(int p_x, int p_y) const
{
  for (const auto &entry : m_nodeList) {
    if (entry->BranchAboveHitTest(p_x, p_y)) {
      return entry->GetNode()->GetParent();
    }
  }
  return nullptr;
}

GameNode gbtTreeLayout::BranchBelowHitTest(int p_x, int p_y) const
{
  for (const auto &entry : m_nodeList) {
    if (entry->BranchAboveHitTest(p_x, p_y)) {
      return entry->GetNode()->GetParent();
    }
  }
  return nullptr;
}

GameNode gbtTreeLayout::InfosetHitTest(int p_x, int p_y) const
{
  for (const auto &entry : m_nodeList) {
    if (entry->GetNextMember() && entry->GetNode()->GetInfoset()) {
      if (p_x > entry->X() + entry->GetSublevel() * m_infosetSpacing - 2 &&
          p_x < entry->X() + entry->GetSublevel() * m_infosetSpacing + 2) {
        if (p_y > entry->Y() && p_y < entry->GetNextMember()->Y()) {
          // next iset is below this one
          return entry->GetNode();
        }
        else if (p_y > entry->GetNextMember()->Y() && p_y < entry->Y()) {
          // next iset is above this one
          return entry->GetNode();
        }
      }
    }
  }
  return nullptr;
}

wxString gbtTreeLayout::CreateNodeLabel(const gbtNodeEntry *p_entry, int p_which) const
{
  const GameNode n = p_entry->GetNode();

  try {
    switch (p_which) {
    case GBT_NODE_LABEL_NOTHING:
      return wxT("");
    case GBT_NODE_LABEL_LABEL:
      return {n->GetLabel().c_str(), *wxConvCurrent};
    case GBT_NODE_LABEL_PLAYER:
      if (n->GetPlayer()) {
        return {n->GetPlayer()->GetLabel().c_str(), *wxConvCurrent};
      }
      else {
        return wxT("");
      }
    case GBT_NODE_LABEL_ISETLABEL:
      if (n->GetInfoset()) {
        return {n->GetInfoset()->GetLabel().c_str(), *wxConvCurrent};
      }
      else {
        return wxT("");
      }
    case GBT_NODE_LABEL_ISETID:
      if (n->GetInfoset()) {
        if (n->GetInfoset()->IsChanceInfoset()) {
          return wxString::Format(wxT("C:%d"), n->GetInfoset()->GetNumber());
        }
        else {
          return wxString::Format(wxT("%d:%d"), n->GetPlayer()->GetNumber(),
                                  n->GetInfoset()->GetNumber());
        }
      }
      else {
        return _T("");
      }
    case GBT_NODE_LABEL_REALIZPROB:
      return {m_doc->GetProfiles().GetRealizProb(n).c_str(), *wxConvCurrent};
    case GBT_NODE_LABEL_BELIEFPROB:
      return {m_doc->GetProfiles().GetBeliefProb(n).c_str(), *wxConvCurrent};
    case GBT_NODE_LABEL_VALUE:
      if (n->GetInfoset() && n->GetPlayer()->GetNumber() > 0) {
        return {m_doc->GetProfiles().GetNodeValue(n, n->GetPlayer()->GetNumber()).c_str(),
                *wxConvCurrent};
      }
      else {
        return wxT("");
      }
    default:
      return wxT("");
    }
  }
  catch (...) {
    return wxT("");
  }
}

wxString gbtTreeLayout::CreateBranchLabel(const gbtNodeEntry *p_entry, int p_which) const
{
  const GameNode parent = p_entry->GetParent()->GetNode();

  try {
    switch (p_which) {
    case GBT_BRANCH_LABEL_NOTHING:
      return wxT("");
    case GBT_BRANCH_LABEL_LABEL:
      return {parent->GetInfoset()->GetAction(p_entry->GetChildNumber())->GetLabel().c_str(),
              *wxConvCurrent};
    case GBT_BRANCH_LABEL_PROBS:
      if (parent->GetPlayer() && parent->GetPlayer()->IsChance()) {
        return {static_cast<std::string>(
                    parent->GetInfoset()->GetActionProb(
                        parent->GetInfoset()->GetAction(p_entry->GetChildNumber())))
                    .c_str(),
                *wxConvCurrent};
      }
      else if (m_doc->NumProfileLists() == 0) {
        return wxT("");
      }
      else {
        return {m_doc->GetProfiles().GetActionProb(parent, p_entry->GetChildNumber()).c_str(),
                *wxConvCurrent};
      }
    case GBT_BRANCH_LABEL_VALUE:
      if (m_doc->NumProfileLists() == 0) {
        return wxT("");
      }
      else {
        return {m_doc->GetProfiles().GetActionValue(parent, p_entry->GetChildNumber()).c_str(),
                *wxConvCurrent};
      }
    default:
      return wxT("");
    }
  }
  catch (...) {
    return wxT("");
  }
}

gbtNodeEntry *gbtTreeLayout::GetValidParent(const GameNode &e)
{
  gbtNodeEntry *n = GetNodeEntry(e->GetParent());
  if (n) {
    return n;
  }
  else {
    return GetValidParent(e->GetParent());
  }
}

gbtNodeEntry *gbtTreeLayout::GetValidChild(const GameNode &e)
{
  for (const auto &child : e->GetChildren()) {
    gbtNodeEntry *n = GetNodeEntry(child);
    if (n) {
      return n;
    }
    else {
      n = GetValidChild(child);
      if (n) {
        return n;
      }
    }
  }
  return nullptr;
}

gbtNodeEntry *gbtTreeLayout::GetEntry(const GameNode &p_node) const
{
  for (const auto &entry : m_nodeList) {
    if (entry->GetNode() == p_node) {
      return entry;
    }
  }
  return nullptr;
}

GameNode gbtTreeLayout::PriorSameLevel(const GameNode &p_node) const
{
  gbtNodeEntry *entry = GetEntry(p_node);
  if (entry) {
    auto e = std::next(std::find(m_nodeList.rbegin(), m_nodeList.rend(), entry));
    while (e != m_nodeList.rend()) {
      if ((*e)->GetLevel() == entry->GetLevel()) {
        return (*e)->GetNode();
      }
    }
  }
  return nullptr;
}

GameNode gbtTreeLayout::NextSameLevel(const GameNode &p_node) const
{
  gbtNodeEntry *entry = GetEntry(p_node);
  if (entry) {
    auto e = std::next(std::find(m_nodeList.begin(), m_nodeList.end(), entry));
    while (e != m_nodeList.end()) {
      if ((*e)->GetLevel() == entry->GetLevel()) {
        return (*e)->GetNode();
      }
      ++e;
    }
  }
  return nullptr;
}

int gbtTreeLayout::LayoutSubtree(const GameNode &p_node, const BehaviorSupportProfile &p_support,
                                 int &p_maxy, int &p_miny, int &p_ycoord)
{
  int y1 = -1, yn = 0;
  const gbtStyle &settings = m_doc->GetStyle();

  gbtNodeEntry *entry = GetEntry(p_node);
  entry->SetNextMember(nullptr);
  if (m_doc->GetStyle().RootReachable() && p_node->GetInfoset() &&
      !p_node->GetInfoset()->GetPlayer()->IsChance()) {
    const GameInfoset infoset = p_node->GetInfoset();
    for (auto action : p_support.GetActions(infoset)) {
      yn = LayoutSubtree(p_node->GetChild(action), p_support, p_maxy, p_miny, p_ycoord);
      if (y1 == -1) {
        y1 = yn;
      }
    }
    entry->SetY((y1 + yn) / 2);
  }
  else {
    if (!p_node->IsTerminal()) {
      for (const auto &action : p_node->GetInfoset()->GetActions()) {
        yn = LayoutSubtree(p_node->GetChild(action), p_support, p_maxy, p_miny, p_ycoord);
        if (y1 == -1) {
          y1 = yn;
        }

        if (!p_node->GetPlayer()->IsChance() && !p_support.Contains(action)) {
          (*std::find_if(m_nodeList.begin(), m_nodeList.end(), [&](const gbtNodeEntry *e) {
            return e->GetNode() == p_node->GetChild(action);
          }))->SetInSupport(false);
        }
      }
      entry->SetY((y1 + yn) / 2);
    }
    else {
      entry->SetY(p_ycoord);
      p_ycoord += settings.TerminalSpacing();
    }
  }

  if (settings.BranchStyle() == GBT_BRANCH_STYLE_LINE) {
    entry->SetX(c_leftMargin +
                entry->GetLevel() * (settings.NodeSize() + settings.BranchLength()));
  }
  else {
    entry->SetX(c_leftMargin + entry->GetLevel() * (settings.NodeSize() + settings.BranchLength() +
                                                    settings.TineLength()));
  }

  if (p_node->GetPlayer() && p_node->GetPlayer()->IsChance()) {
    entry->SetColor(settings.ChanceColor());
    entry->SetToken(settings.ChanceToken());
  }
  else if (p_node->GetPlayer()) {
    entry->SetColor(settings.GetPlayerColor(p_node->GetPlayer()->GetNumber()));
    entry->SetToken(settings.PlayerToken());
  }
  else {
    entry->SetColor(settings.TerminalColor());
    entry->SetToken(settings.TerminalToken());
  }

  entry->SetSize(settings.NodeSize());
  entry->SetBranchStyle(settings.BranchStyle());
  if (settings.BranchStyle() == GBT_BRANCH_STYLE_LINE) {
    entry->SetBranchLabelStyle(settings.BranchLabels());
  }
  entry->SetBranchLength(settings.BranchLength());

  p_maxy = std::max(entry->Y(), p_maxy);
  p_miny = std::min(entry->Y(), p_miny);

  return entry->Y();
}

//
// Checks if there are any nodes in the same infoset as e that are either
// on the same level (if SHOWISET_SAME) or on any level (if SHOWISET_ALL)
//
gbtNodeEntry *gbtTreeLayout::NextInfoset(gbtNodeEntry *e)
{
  const gbtStyle &draw_settings = m_doc->GetStyle();

  auto entry = std::next(std::find(m_nodeList.begin(), m_nodeList.end(), e));
  while (entry != m_nodeList.end()) {
    gbtNodeEntry *e1 = *entry;
    // infosets are the same and the nodes are on the same level
    if (e->GetNode()->GetInfoset() == e1->GetNode()->GetInfoset()) {
      if (draw_settings.InfosetConnect() == GBT_INFOSET_CONNECT_ALL) {
        return e1;
      }
      else if (e->GetLevel() == e1->GetLevel()) {
        return e1;
      }
    }
    ++entry;
  }
  return nullptr;
}

//
// CheckInfosetEntry.  Checks how many infoset lines are to be drawn at each
// level, spaces them by setting each infoset's node's num to the previous
// infoset node+1.  Also lengthens the nodes by the amount of space taken up
// by the infoset lines.
//
void gbtTreeLayout::CheckInfosetEntry(gbtNodeEntry *e)
{
  gbtNodeEntry *infoset_entry;
  // Check if the infoset this entry belongs to (on this level) has already
  // been processed.  If so, make this entry->num the same as the one already
  // processed and return
  infoset_entry = NextInfoset(e);
  for (const auto &e1 : m_nodeList) {
    // if the infosets are the same, and they are on the same level, and e1 has been processed
    if (e->GetNode()->GetInfoset() == e1->GetNode()->GetInfoset() &&
        e->GetLevel() == e1->GetLevel() && e1->GetSublevel() > 0) {
      e->SetSublevel(e1->GetSublevel());
      if (infoset_entry) {
        e->SetNextMember(infoset_entry);
      }
      return;
    }
  }

  // If we got here, this entry does not belong to any processed infoset yet.
  // Check if it belongs to ANY infoset, if not just return
  if (!infoset_entry) {
    return;
  }

  // If we got here, then this entry is new and is connected to other entries
  // find the entry on the same level with the maximum num.
  // This entry will have num = num+1.
  int num = 0;
  for (const auto &e1 : m_nodeList) {
    // Find the max num for this level
    if (e->GetLevel() == e1->GetLevel()) {
      num = std::max(e1->GetSublevel(), num);
    }
  }
  num++;
  e->SetSublevel(num);
  e->SetNextMember(infoset_entry);
}

void gbtTreeLayout::FillInfosetTable(const GameNode &n, const BehaviorSupportProfile &cur_sup)
{
  const gbtStyle &draw_settings = m_doc->GetStyle();
  gbtNodeEntry *entry = GetNodeEntry(n);
  if (!n->IsTerminal()) {
    for (const auto &action : n->GetInfoset()->GetActions()) {
      const bool in_sup = n->GetPlayer()->IsChance() || cur_sup.Contains(action);
      if (in_sup || !draw_settings.RootReachable()) {
        FillInfosetTable(n->GetChild(action), cur_sup);
      }
    }
  }

  if (entry) {
    CheckInfosetEntry(entry);
  }
}

void gbtTreeLayout::UpdateTableInfosets()
{
  // Note that levels are numbered from 0, not 1.
  // create an array to hold max num for each level
  Array<int> nums(0, m_maxLevel + 1);

  for (int i = 0; i <= m_maxLevel + 1; nums[i++] = 0)
    ;
  // find the max e->num for each level
  for (const auto &entry : m_nodeList) {
    nums[entry->GetLevel()] = std::max(entry->GetSublevel() + 1, nums[entry->GetLevel()]);
  }

  for (int i = 0; i <= m_maxLevel; i++) {
    nums[i + 1] += nums[i];
  }

  // now add the needed length to each level, and set maxX accordingly
  m_maxX = 0;
  for (const auto &entry : m_nodeList) {
    if (entry->GetLevel() != 0) {
      entry->SetX(entry->X() +
                  (nums[entry->GetLevel() - 1] + entry->GetSublevel()) * m_infosetSpacing);
    }
    m_maxX = std::max(m_maxX, entry->X() + entry->GetSize());
  }
}

void gbtTreeLayout::UpdateTableParents()
{
  for (const auto &e : m_nodeList) {
    e->SetParent((e->GetNode() == m_doc->GetGame()->GetRoot()) ? e : GetValidParent(e->GetNode()));
  }
}

void gbtTreeLayout::Layout(const BehaviorSupportProfile &p_support)
{
  // Kinda kludgey; probably should query draw settings whenever needed.
  m_infosetSpacing = (m_doc->GetStyle().InfosetJoin() == GBT_INFOSET_JOIN_LINES) ? 10 : 40;

  if (m_nodeList.size() != m_doc->GetGame()->NumNodes()) {
    // A rebuild is in order; force it
    BuildNodeList(p_support);
  }

  int miny = 0, maxy = 0, ycoord = c_topMargin;
  LayoutSubtree(m_doc->GetGame()->GetRoot(), p_support, maxy, miny, ycoord);

  const gbtStyle &draw_settings = m_doc->GetStyle();
  if (draw_settings.InfosetConnect() != GBT_INFOSET_CONNECT_NONE) {
    // FIXME! This causes lines to disappear... sometimes.
    FillInfosetTable(m_doc->GetGame()->GetRoot(), p_support);
    UpdateTableInfosets();
  }

  UpdateTableParents();
  GenerateLabels();

  m_maxY = maxy + 25;
}

void gbtTreeLayout::BuildNodeList(const GameNode &p_node, const BehaviorSupportProfile &p_support,
                                  int p_level)
{
  auto *entry = new gbtNodeEntry(p_node);
  entry->SetStyle(&m_doc->GetStyle());
  m_nodeList.push_back(entry);
  entry->SetLevel(p_level);
  if (m_doc->GetStyle().RootReachable()) {
    const GameInfoset infoset = p_node->GetInfoset();
    if (infoset) {
      if (infoset->GetPlayer()->IsChance()) {
        for (const auto &child : p_node->GetChildren()) {
          BuildNodeList(child, p_support, p_level + 1);
        }
      }
      else {
        for (const auto &action : p_support.GetActions(infoset)) {
          BuildNodeList(p_node->GetChild(action), p_support, p_level + 1);
        }
      }
    }
  }
  else {
    for (const auto &child : p_node->GetChildren()) {
      BuildNodeList(child, p_support, p_level + 1);
    }
  }
  m_maxLevel = std::max(p_level, m_maxLevel);
}

void gbtTreeLayout::BuildNodeList(const BehaviorSupportProfile &p_support)
{
  for (auto entry : m_nodeList) {
    delete entry;
  }
  m_nodeList.clear();
  m_maxLevel = 0;
  BuildNodeList(m_doc->GetGame()->GetRoot(), p_support, 0);
}

void gbtTreeLayout::GenerateLabels()
{
  const gbtStyle &settings = m_doc->GetStyle();
  for (const auto &entry : m_nodeList) {
    entry->SetNodeAboveLabel(CreateNodeLabel(entry, settings.NodeAboveLabel()));
    entry->SetNodeAboveFont(settings.GetFont());
    entry->SetNodeBelowLabel(CreateNodeLabel(entry, settings.NodeBelowLabel()));
    entry->SetNodeBelowFont(settings.GetFont());
    if (entry->GetChildNumber() > 0) {
      entry->SetBranchAboveLabel(CreateBranchLabel(entry, settings.BranchAboveLabel()));
      entry->SetBranchAboveFont(settings.GetFont());
      entry->SetBranchBelowLabel(CreateBranchLabel(entry, settings.BranchBelowLabel()));
      entry->SetBranchBelowFont(settings.GetFont());

      const GameNode parent = entry->GetNode()->GetParent();
      if (parent->GetPlayer()->IsChance()) {
        entry->SetActionProb(static_cast<double>(parent->GetInfoset()->GetActionProb(
            parent->GetInfoset()->GetAction(entry->GetChildNumber()))));
      }
      else {
        const int profile = m_doc->GetCurrentProfile();
        if (profile > 0) {
          try {
            entry->SetActionProb((double)lexical_cast<Rational>(
                m_doc->GetProfiles().GetActionProb(parent, entry->GetChildNumber())));
          }
          catch (ValueException &) {
            // This occurs when the probability is undefined
            entry->SetActionProb(0.0);
          }
        }
      }
    }
  }
}

//
// RenderSubtree: Render branches and labels
//
// The following speed optimizations have been added:
// The algorithm now traverses the tree as a linear linked list, eliminating
// expensive searches.
//
// There was some clipping code in here, but it didn't correctly
// deal with drawing information sets while scrolling.  So, the code
// has temporarily been removed.  It remains to be seen whether
// performance will require a more sophisticated solution to the
// problem.  (TLT 5/2001)
//
void gbtTreeLayout::RenderSubtree(wxDC &p_dc, bool p_noHints) const
{
  const gbtStyle &settings = m_doc->GetStyle();

  for (const auto &entry : m_nodeList) {
    gbtNodeEntry *parentEntry = entry->GetParent();

    if (entry->GetChildNumber() == 1) {
      parentEntry->Draw(p_dc, m_doc->GetSelectNode(), p_noHints);

      if (m_doc->GetStyle().InfosetConnect() != GBT_INFOSET_CONNECT_NONE &&
          parentEntry->GetNextMember()) {
        const int nextX = parentEntry->GetNextMember()->X();
        const int nextY = parentEntry->GetNextMember()->Y();

        if ((m_doc->GetStyle().InfosetConnect() != GBT_INFOSET_CONNECT_SAMELEVEL) ||
            parentEntry->X() == nextX) {
#ifdef __WXGTK__
          // A problem with using styled pens and user scaling on wxGTK
          p_dc.SetPen(wxPen(parentEntry->GetColor(), 1, wxPENSTYLE_SOLID));
#else
          p_dc.SetPen(wxPen(parentEntry->GetColor(), 1, wxPENSTYLE_DOT));
#endif // __WXGTK__
          p_dc.DrawLine(parentEntry->X(), parentEntry->Y(), parentEntry->X(), nextY);
          if (settings.InfosetJoin() == GBT_INFOSET_JOIN_CIRCLES) {
            p_dc.DrawLine(parentEntry->X() + parentEntry->GetSize(), parentEntry->Y(),
                          parentEntry->X() + parentEntry->GetSize(), nextY);
          }

          if (parentEntry->GetNextMember()->X() != parentEntry->X()) {
            // Draw a little arrow in the direction of the iset.
            int startX, endX;
            if (settings.InfosetJoin() == GBT_INFOSET_JOIN_LINES) {
              startX = parentEntry->X();
              endX =
                  (startX + m_infosetSpacing *
                                ((parentEntry->GetNextMember()->X() > parentEntry->X()) ? 1 : -1));
            }
            else {
              if (parentEntry->GetNextMember()->X() < parentEntry->X()) {
                // information set is continued to the left
                startX = parentEntry->X() + parentEntry->GetSize();
                endX = parentEntry->X() - m_infosetSpacing;
              }
              else {
                // information set is continued to the right
                startX = parentEntry->X();
                endX = (parentEntry->X() + parentEntry->GetSize() + m_infosetSpacing);
              }
            }
            p_dc.DrawLine(startX, nextY, endX, nextY);
            if (startX > endX) {
              p_dc.DrawLine(endX, nextY, endX + m_infosetSpacing / 2,
                            nextY + m_infosetSpacing / 2);
              p_dc.DrawLine(endX, nextY, endX + m_infosetSpacing / 2,
                            nextY - m_infosetSpacing / 2);
            }
            else {
              p_dc.DrawLine(endX, nextY, endX - m_infosetSpacing / 2,
                            nextY + m_infosetSpacing / 2);
              p_dc.DrawLine(endX, nextY, endX - m_infosetSpacing / 2,
                            nextY - m_infosetSpacing / 2);
            }
          }
        }
      }
    }

    if (entry->GetNode()->IsTerminal()) {
      entry->Draw(p_dc, m_doc->GetSelectNode(), p_noHints);
    }

    // As we draw, we determine the outcome label extents.  Adjust the
    // overall size of the plot accordingly.
    if (entry->GetOutcomeExtent().GetRight() > m_maxX) {
      m_maxX = entry->GetOutcomeExtent().GetRight();
    }
  }
}

void gbtTreeLayout::Render(wxDC &p_dc, bool p_noHints) const { RenderSubtree(p_dc, p_noHints); }
