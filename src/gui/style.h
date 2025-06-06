//
// This file is part of Gambit
// Copyright (c) 1994-2025, The Gambit Project (https://www.gambit-project.org)
//
// FILE: src/gui/style.h
// Class to store settings related to graphical interface styling
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

#ifndef STYLE_H
#define STYLE_H

#include "gambit.h"
#include "core/tinyxml.h"

const int GBT_NODE_TOKEN_LINE = 0;
const int GBT_NODE_TOKEN_BOX = 1;
const int GBT_NODE_TOKEN_CIRCLE = 2;
const int GBT_NODE_TOKEN_DIAMOND = 3;
const int GBT_NODE_TOKEN_DOT = 4;

const int GBT_BRANCH_STYLE_LINE = 0;
const int GBT_BRANCH_STYLE_FORKTINE = 1;

const int GBT_BRANCH_LABEL_HORIZONTAL = 0;
const int GBT_BRANCH_LABEL_ROTATED = 1;

const int GBT_INFOSET_JOIN_LINES = 0;
const int GBT_INFOSET_JOIN_CIRCLES = 1;

const int GBT_INFOSET_CONNECT_NONE = 0;
const int GBT_INFOSET_CONNECT_SAMELEVEL = 1;
const int GBT_INFOSET_CONNECT_ALL = 2;

const int GBT_NODE_LABEL_NOTHING = 0;
const int GBT_NODE_LABEL_LABEL = 1;
const int GBT_NODE_LABEL_PLAYER = 2;
const int GBT_NODE_LABEL_ISETLABEL = 3;
const int GBT_NODE_LABEL_ISETID = 4;
const int GBT_NODE_LABEL_REALIZPROB = 5;
const int GBT_NODE_LABEL_BELIEFPROB = 6;
const int GBT_NODE_LABEL_VALUE = 7;

const int GBT_BRANCH_LABEL_NOTHING = 0;
const int GBT_BRANCH_LABEL_LABEL = 1;
const int GBT_BRANCH_LABEL_PROBS = 2;
const int GBT_BRANCH_LABEL_VALUE = 3;

class wxFont;

class gbtStyle {
private:
  // Node styling
  int m_nodeSize{10}, m_terminalSpacing{50};
  int m_chanceToken{GBT_NODE_TOKEN_DOT}, m_playerToken{GBT_NODE_TOKEN_DOT},
      m_terminalToken{GBT_NODE_TOKEN_DOT};
  bool m_rootReachable{false};

  // Branch styling
  int m_branchLength{60}, m_tineLength{20};
  int m_branchStyle{GBT_BRANCH_STYLE_FORKTINE}, m_branchLabels{GBT_BRANCH_LABEL_HORIZONTAL};

  // Information set styling
  int m_infosetConnect{GBT_INFOSET_CONNECT_ALL}, m_infosetJoin{GBT_INFOSET_JOIN_CIRCLES};

  // Legend styling
  int m_nodeAboveLabel{GBT_NODE_LABEL_LABEL}, m_nodeBelowLabel{GBT_NODE_LABEL_ISETID};
  int m_branchAboveLabel{GBT_BRANCH_LABEL_LABEL}, m_branchBelowLabel{GBT_BRANCH_LABEL_PROBS};

  // Fonts for legends
  wxFont m_font;

  // Colors for nodes
  wxColour m_chanceColor, m_terminalColor;
  mutable Gambit::Array<wxColour> m_playerColors;

  // Decimal places to display
  int m_numDecimals{4};

public:
  // Lifecycle
  gbtStyle();

  // Node styling
  int NodeSize() const { return m_nodeSize; }
  void SetNodeSize(int p_nodeSize) { m_nodeSize = p_nodeSize; }

  int TerminalSpacing() const { return m_terminalSpacing; }
  void SetTerminalSpacing(int p_spacing) { m_terminalSpacing = p_spacing; }

  int ChanceToken() const { return m_chanceToken; }
  void SetChanceToken(int p_token) { m_chanceToken = p_token; }

  int PlayerToken() const { return m_playerToken; }
  void SetPlayerToken(int p_token) { m_playerToken = p_token; }

  int TerminalToken() const { return m_terminalToken; }
  void SetTerminalToken(int p_token) { m_terminalToken = p_token; }

  bool RootReachable() const { return m_rootReachable; }
  void SetRootReachable(bool p_reachable) { m_rootReachable = p_reachable; }

  // Branch styling
  int BranchLength() const { return m_branchLength; }
  void SetBranchLength(int p_length) { m_branchLength = p_length; }

  int TineLength() const { return m_tineLength; }
  void SetTineLength(int p_length) { m_tineLength = p_length; }

  int BranchStyle() const { return m_branchStyle; }
  void SetBranchStyle(int p_style) { m_branchStyle = p_style; }

  int BranchLabels() const { return m_branchLabels; }
  void SetBranchLabels(int p_labels) { m_branchLabels = p_labels; }

  // Information set styling
  int InfosetConnect() const { return m_infosetConnect; }
  void SetInfosetConnect(int p_connect) { m_infosetConnect = p_connect; }

  int InfosetJoin() const { return m_infosetJoin; }
  void SetInfosetJoin(int p_join) { m_infosetJoin = p_join; }

  // Legends
  int NodeAboveLabel() const { return m_nodeAboveLabel; }
  void SetNodeAboveLabel(int p_label) { m_nodeAboveLabel = p_label; }

  int NodeBelowLabel() const { return m_nodeBelowLabel; }
  void SetNodeBelowLabel(int p_label) { m_nodeBelowLabel = p_label; }

  int BranchAboveLabel() const { return m_branchAboveLabel; }
  void SetBranchAboveLabel(int p_label) { m_branchAboveLabel = p_label; }

  int BranchBelowLabel() const { return m_branchBelowLabel; }
  void SetBranchBelowLabel(int p_label) { m_branchBelowLabel = p_label; }

  // Fonts
  const wxFont &GetFont() const { return m_font; }
  void SetFont(const wxFont &p_font) { m_font = p_font; }

  // Colors
  const wxColour &ChanceColor() const { return m_chanceColor; }
  void SetChanceColor(const wxColour &p_color) { m_chanceColor = p_color; }

  const wxColour &TerminalColor() const { return m_terminalColor; }
  void SetTerminalColor(const wxColour &p_color) { m_terminalColor = p_color; }

  const wxColour &GetPlayerColor(int pl) const;
  void SetPlayerColor(int pl, const wxColour &p_color) { m_playerColors[pl] = p_color; }

  // Decimals
  int NumDecimals() const { return m_numDecimals; }
  void SetNumDecimals(int p_decimals) { m_numDecimals = p_decimals; }

  // Reset to the "factory" defaults
  void SetDefaults();

  /// @name Reading and writing XML
  //@{
  /// Get the color settings as an XML entry
  std::string GetColorXML() const;
  /// Set the color settings from an XML entry
  void SetColorXML(TiXmlNode *p_node);

  /// Get the font settings as an XML entry
  std::string GetFontXML() const;
  /// Set the font settings from an XML entry
  void SetFontXML(TiXmlNode *p_node);

  /// Get the layout settings as an XML entry
  std::string GetLayoutXML() const;
  /// Set the layout settings from an XML entry
  void SetLayoutXML(TiXmlNode *p_node);

  /// Get the label settings as an XML entry
  std::string GetLabelXML() const;
  /// Set the label settings from an XML entry
  void SetLabelXML(TiXmlNode *p_node);
  //@}
};

#endif // STYLE_H
