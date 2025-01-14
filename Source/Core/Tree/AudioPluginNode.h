/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

class AudioPlugin;
class AudioPluginEditor;

#include "TreeNode.h"

class AudioPluginNode final : public TreeNode
{
public:

    AudioPluginNode(AudioProcessorGraph::NodeID pluginID, const String &name);

    Image getIcon() const noexcept override;
    AudioProcessorGraph::NodeID getNodeId() const noexcept;

    bool hasMenu() const noexcept override;
    Component *createMenu() override;

    void showPage() override;
    
private:

    UniquePointer<Component> audioPluginEditor;
    const AudioProcessorGraph::NodeID nodeId;

};
