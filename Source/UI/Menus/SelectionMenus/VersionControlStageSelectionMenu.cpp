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

#include "Common.h"
#include "VersionControlStageSelectionMenu.h"
#include "CommandIDs.h"
#include "Icons.h"

VersionControlStageSelectionMenu::VersionControlStageSelectionMenu()
{
    MenuPanel::Menu cmds;

    cmds.add(MenuItem::item(Icons::commit,
        CommandIDs::VersionControlCommitSelected,
        TRANS(I18n::Menu::Selection::vcsCommit))->closesMenu());

    cmds.add(MenuItem::item(Icons::reset,
        CommandIDs::VersionControlResetSelected,
        TRANS(I18n::Menu::Selection::vcsReset))->closesMenu());

    // TODO add named stashes in addition to the default one
    //cmds.add(MenuItem::item(Icons::stash,
    //    CommandIDs::VersionControlStashSelected,
    //    TRANS(I18n::Menu::Selection::vcsStash))->closesMenu());

    cmds.add(MenuItem::item(Icons::selectAll,
        CommandIDs::VersionControlSelectAll,
        TRANS(I18n::Menu::Selection::vcsSelectall)));

    cmds.add(MenuItem::item(Icons::selectNone,
        CommandIDs::VersionControlSelectNone,
        TRANS(I18n::Menu::Selection::vcsSelectnone))->closesMenu());

    this->updateContent(cmds, MenuPanel::SlideRight);
}
