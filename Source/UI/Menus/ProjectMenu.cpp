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
#include "ProjectMenu.h"
#include "ProjectNode.h"
#include "Icons.h"
#include "ModalDialogInput.h"
#include "ModalDialogConfirmation.h"
#include "PianoTrackNode.h"
#include "AutomationTrackNode.h"
#include "VersionControlNode.h"
#include "PatternEditorNode.h"
#include "AutomationSequence.h"
#include "MainLayout.h"
#include "AudioCore.h"
#include "Instrument.h"
#include "PianoSequence.h"
#include "HybridRoll.h"
#include "Document.h"
#include "SuccessTooltip.h"
#include "MidiTrackActions.h"
#include "PianoTrackActions.h"
#include "AutomationTrackActions.h"
#include "Pattern.h"
#include "UndoStack.h"
#include "Workspace.h"
#include "CommandIDs.h"

#define NUM_CONTROLLERS_TO_SHOW 80

ProjectMenu::ProjectMenu(ProjectNode &parentProject, AnimationType animationType) :
    project(parentProject)
{
    this->showMainMenu(animationType);
}

void ProjectMenu::handleCommandMessage(int commandId)
{
    switch (commandId)
    {
        case CommandIDs::AddTempoController:
        {
            bool hasTempoTrack = false;
            const auto autoTracks = this->project.findChildrenOfType<AutomationTrackNode>();
            
            for (auto *i : autoTracks)
            {
                if (i->getTrackControllerNumber() == MidiTrack::tempoController)
                {
                    hasTempoTrack = true;
                    break;
                }
            }
            
            if (hasTempoTrack)
            {
                App::Layout().showTooltip(TRANS(I18n::Menu::Project::addTempoFailed));
            }
            else
            {
                const auto autoLayerParams =
                    this->createAutoTrackTempate(TRANS(I18n::Defaults::tempoTrackName),
                        MidiTrack::tempoController);
                
                this->project.getUndoStack()->beginNewTransaction();
                this->project.getUndoStack()->perform(new AutomationTrackInsertAction(this->project,
                    &this->project, autoLayerParams,  TRANS(I18n::Defaults::tempoTrackName)));
            }

            this->dismiss();
            return;
        }

        case CommandIDs::ProjectTransposeUp:
        {
            this->project.checkpoint();
            const auto tracks = this->project.getTracks();
            for (int i = 0; i < tracks.size(); ++i)
            {
                if (dynamic_cast<PianoSequence *>(tracks.getUnchecked(i)->getSequence()))
                {
                    tracks.getUnchecked(i)->getPattern()->transposeAll(1, false);
                }
            }
        }
        return;

        case CommandIDs::ProjectTransposeDown:
        {
            this->project.checkpoint();
            const auto tracks = this->project.getTracks();
            for (int i = 0; i < tracks.size(); ++i)
            {
                if (dynamic_cast<PianoSequence *>(tracks.getUnchecked(i)->getSequence()))
                {
                    tracks.getUnchecked(i)->getPattern()->transposeAll(-1, false);
                }
            }
        }
        return;

        case CommandIDs::UnloadProject:
            App::Workspace().unloadProject(this->project.getId(), false, false);
            this->dismiss();
            return;

        case CommandIDs::DeleteProject:
        {
            auto confirmationDialog = ModalDialogConfirmation::Presets::deleteProject();
            auto &project = this->project;
            confirmationDialog->onOk = [&project]()
            {
                auto inputDialog = ModalDialogInput::Presets::deleteProjectConfirmation();
                inputDialog->onOk = [&project](const String &text)
                {
                    if (text == project.getName())
                    {
                        App::Workspace().unloadProject(project.getId(), true, true);
                    }
                    else
                    {
                        App::Layout().showTooltip(TRANS(I18n::Menu::Project::deleteCancelled));
                    }
                };

                App::Layout().showModalComponentUnowned(inputDialog.release());
            };

            App::Layout().showModalComponentUnowned(confirmationDialog.release());
            return;
        }
    }
}

ValueTree ProjectMenu::createPianoTrackTempate(const String &name, const String &instrumentId) const
{
    UniquePointer<MidiTrackNode> newItem(new PianoTrackNode(name));
    
    // We need to have at least one clip on a pattern:
    const Clip clip(newItem->getPattern());
    newItem->getPattern()->insert(clip, false);

    Random r;
    const auto colours = MenuPanel::getColoursList().getAllValues();
    const int ci = r.nextInt(colours.size());
    newItem->setTrackColour(Colour::fromString(colours[ci]), dontSendNotification);
    newItem->setTrackInstrumentId(instrumentId, false);

    // TODO insert a single note?

    return newItem->serialize();
}

ValueTree ProjectMenu::createAutoTrackTempate(const String &name,
    int controllerNumber, const String &instrumentId) const
{
    UniquePointer<MidiTrackNode> newItem(new AutomationTrackNode(name));

    // We need to have at least one clip on a pattern:
    const Clip clip(newItem->getPattern());
    newItem->getPattern()->insert(clip, false);

    auto itemLayer = static_cast<AutomationSequence *>(newItem->getSequence());
    
    newItem->setTrackControllerNumber(controllerNumber, false);
    newItem->setTrackInstrumentId(instrumentId, false);
    newItem->setTrackColour(Colours::royalblue, false);
    
    // init with a couple of events
    const float cv1 = newItem->isOnOffAutomationTrack() ? 1.f : 0.5f;
    const float cv2 = newItem->isOnOffAutomationTrack() ? 0.f : 0.5f;

    const auto beatRange = this->project.getProjectRangeInBeats();
    const float firstBeat = beatRange.getX();
    const float lastBeat = beatRange.getY();

    itemLayer->insert(AutomationEvent(itemLayer, firstBeat, cv1), false);
    // second event is placed at the end of the track for convenience:
    itemLayer->insert(AutomationEvent(itemLayer, lastBeat, cv2), false);

    return newItem->serialize();
}

void ProjectMenu::showMainMenu(AnimationType animationType)
{
    MenuPanel::Menu menu;

    menu.add(MenuItem::item(Icons::piano, CommandIDs::SwitchToEditMode,
        TRANS(I18n::Menu::Project::editorLinear))->closesMenu());

    menu.add(MenuItem::item(Icons::patterns, CommandIDs::SwitchToArrangeMode,
        TRANS(I18n::Menu::Project::editorPattern))->closesMenu());

    menu.add(MenuItem::item(Icons::versionControl, CommandIDs::SwitchToVersioningMode,
        TRANS(I18n::Menu::Project::editorVcs))->closesMenu());

    menu.add(MenuItem::item(Icons::create,
        TRANS(I18n::Menu::Project::addItems))->withSubmenu()->withAction([this]()
        {
            this->showCreateItemsMenu(MenuPanel::SlideLeft);
        }));

    menu.add(MenuItem::item(Icons::render,
        TRANS(I18n::Menu::Project::render))->withSubmenu()->withAction([this]()
        {
            this->showRenderMenu();
        }));

    menu.add(MenuItem::item(Icons::refactor,
        TRANS(I18n::Menu::Project::refactor))->withSubmenu()->withAction([this]()
        {
            this->showBatchActionsMenu(MenuPanel::SlideLeft);
        }));
    
    menu.add(MenuItem::item(Icons::close, CommandIDs::UnloadProject, TRANS(I18n::Menu::Project::unload)));
    menu.add(MenuItem::item(Icons::remove, CommandIDs::DeleteProject, TRANS(I18n::Menu::Project::deleteConfirm)));
    this->updateContent(menu, animationType);
}

void ProjectMenu::showCreateItemsMenu(AnimationType animationType)
{
    MenuPanel::Menu menu;
    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showMainMenu(MenuPanel::SlideRight);
        }));

#if HELIO_DESKTOP
    menu.add(MenuItem::item(Icons::browse, CommandIDs::ImportMidi,
        TRANS(I18n::Menu::Project::importMidi))->closesMenu());
#endif

    menu.add(MenuItem::item(Icons::pianoTrack,
        TRANS(I18n::Menu::Project::addTrack))->withSubmenu()->withAction([this]()
        {
            this->showNewTrackMenu(MenuPanel::SlideLeft);
        }));

    menu.add(MenuItem::item(Icons::automationTrack,
        TRANS(I18n::Menu::Project::addAutomation))->withSubmenu()->withAction([this]()
        {
            this->showNewAutomationMenu(MenuPanel::SlideLeft);
        }));

    this->updateContent(menu, animationType);
}

void ProjectMenu::showNewTrackMenu(AnimationType animationType)
{
    MenuPanel::Menu menu;
    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showCreateItemsMenu(MenuPanel::SlideRight);
        }));

    const auto &instruments = App::Workspace().getAudioCore().getInstruments();
    for (int i = 0; i < instruments.size(); ++i)
    {
        const String instrumentId = instruments[i]->getIdAndHash();
        menu.add(MenuItem::item(Icons::instrument,
            instruments[i]->getName())->withAction([this, instrumentId]()
            {
                auto &project = this->project;
                const ValueTree trackTemplate = this->createPianoTrackTempate("", instrumentId);
                auto inputDialog = ModalDialogInput::Presets::newTrack();
                inputDialog->onOk = [trackTemplate, &project](const String &input)
                {
                    project.checkpoint();
                    project.getUndoStack()->perform(new PianoTrackInsertAction(project,
                        &project, trackTemplate, input));
                };

                App::Layout().showModalComponentUnowned(inputDialog.release());
            }));
    }

    this->updateContent(menu, animationType);
}

void ProjectMenu::showNewAutomationMenu(AnimationType animationType)
{
    MenuPanel::Menu menu;
    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showCreateItemsMenu(MenuPanel::SlideRight);
        }));

    menu.add(MenuItem::item(Icons::automationTrack,
        CommandIDs::AddTempoController,
        TRANS(I18n::Menu::Project::addTempo)));

    const auto &instruments = App::Workspace().getAudioCore().getInstruments();
    for (int i = 0; i < instruments.size(); ++i)
    {
        const WeakReference<Instrument> instrument = instruments[i];
        menu.add(MenuItem::item(Icons::instrument,
            instruments[i]->getName())->withSubmenu()->withAction([this, instrument]()
            {
                this->showControllersMenuForInstrument(instrument);
            }));
    }

    this->updateContent(menu, animationType);
}

void ProjectMenu::showControllersMenuForInstrument(const WeakReference<Instrument> instrument)
{
    MenuPanel::Menu menu;
    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showNewAutomationMenu(MenuPanel::SlideRight);
        }));

    for (int controllerNumber = 0; controllerNumber < NUM_CONTROLLERS_TO_SHOW; ++controllerNumber)
    {
        const String controllerName = MidiMessage::getControllerName(controllerNumber);
        if (controllerName.isNotEmpty())
        {
            menu.add(MenuItem::item(Icons::automationTrack,
                String(controllerNumber) + ": " + TRANS(controllerName))->withAction([this, controllerNumber, instrument]()
                {
                    const String instrumentId = instrument ? instrument->getIdAndHash() : "";
                    const String trackName = TreeNode::createSafeName(MidiMessage::getControllerName(controllerNumber));
                    const ValueTree autoTrackParams = this->createAutoTrackTempate(trackName, controllerNumber, instrumentId);

                    this->project.getUndoStack()->beginNewTransaction();
                    this->project.getUndoStack()->perform(new AutomationTrackInsertAction(this->project,
                        &this->project, autoTrackParams, trackName));

                    this->dismiss();
                }));
        }
    }
    
    this->updateContent(menu, MenuPanel::SlideLeft);
}

void ProjectMenu::showRenderMenu()
{
    MenuPanel::Menu menu;

    // TODO! save rendered wavs on mobile in the same way as midi export
#if HELIO_DESKTOP
    const bool noRender = false;
#else
    const bool noRender = true;
#endif

    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showMainMenu(MenuPanel::SlideRight);
        }));

    menu.add(MenuItem::item(Icons::render, CommandIDs::RenderToWAV,
        TRANS(I18n::Menu::Project::renderWav))->disabledIf(noRender)->closesMenu());

    menu.add(MenuItem::item(Icons::render, CommandIDs::RenderToFLAC,
        TRANS(I18n::Menu::Project::renderFlac))->disabledIf(noRender)->closesMenu());

    menu.add(MenuItem::item(Icons::commit, CommandIDs::ExportMidi,
        TRANS(I18n::Menu::Project::renderMidi))->closesMenu());

    this->updateContent(menu, MenuPanel::SlideLeft);
}

void ProjectMenu::showBatchActionsMenu(AnimationType animationType)
{
    MenuPanel::Menu menu;
    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showMainMenu(MenuPanel::SlideRight);
        }));

    menu.add(MenuItem::item(Icons::up,
        CommandIDs::ProjectTransposeUp,
        TRANS(I18n::Menu::Project::transposeUp)));

    menu.add(MenuItem::item(Icons::down,
        CommandIDs::ProjectTransposeDown,
        TRANS(I18n::Menu::Project::transposeDown)));

    //menu.add(MenuItem::item(Icons::group, CommandIDs::RefactorRemoveOverlaps, TRANS(I18n::Menu::Project::cleanup)));

    const auto &tracks = this->project.findChildrenOfType<MidiTrackNode>();
    const auto &instruments = App::Workspace().getAudioCore().getInstruments();
    if (instruments.size() > 1 && tracks.size() > 0)
    {
        menu.add(MenuItem::item(Icons::instrument,
            TRANS(I18n::Menu::Project::changeInstrument))->withSubmenu()->withAction([this]()
            {
                this->showSetInstrumentMenu();
            }));
    }

    this->updateContent(menu, animationType);
}

void ProjectMenu::showSetInstrumentMenu()
{
    MenuPanel::Menu menu;
    menu.add(MenuItem::item(Icons::back,
        TRANS(I18n::Menu::back))->withTimer()->withAction([this]()
        {
            this->showBatchActionsMenu(MenuPanel::SlideRight);
        }));

    const auto &instruments = App::Workspace().getAudioCore().getInstruments();
    for (int i = 0; i < instruments.size(); ++i)
    {
        menu.add(MenuItem::item(Icons::instrument, instruments[i]->getName())->withAction([this, i, instruments]()
        {
            DBG(instruments[i]->getIdAndHash());

            const Array<MidiTrackNode *> tracks =
                this->project.findChildrenOfType<MidiTrackNode>();

            if (tracks.size() > 0)
            {
                this->project.getUndoStack()->beginNewTransaction();

                for (auto *track : tracks)
                {
                    const auto instrumentId = instruments[i]->getIdAndHash();
                    this->project.getUndoStack()->
                        perform(new MidiTrackChangeInstrumentAction(this->project,
                            track->getTrackId(), instrumentId));
                }
            }

            this->dismiss();
        }));
    }
    
    this->updateContent(menu, MenuPanel::SlideLeft);
}
