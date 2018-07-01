/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

/**
 * GameStateNew
 *
 * Handle player choices when starting a new game
 * (e.g. character appearance)
 */

#include "Avatar.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "GameStateNew.h"
#include "GameStateLoad.h"
#include "GameStatePlay.h"
#include "InputState.h"
#include "ItemManager.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "SaveLoad.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "TooltipData.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetCheckBox.h"
#include "WidgetInput.h"
#include "WidgetLabel.h"
#include "WidgetListBox.h"
#include "WidgetTooltip.h"

GameStateNew::GameStateNew()
	: GameState()
	, current_option(0)
	, portrait_image(NULL)
	, portrait_border(NULL)
	, show_classlist(true)
	, modified_name(false)
	, delete_items(true)
	, game_slot(0)
{
	// set up buttons
	button_exit = new WidgetButton();
	button_exit->label = msg->get("Cancel");

	button_create = new WidgetButton();
	button_create->label = msg->get("Create");
	button_create->enabled = false;
	button_create->refresh();

	button_prev = new WidgetButton("images/menus/buttons/left.png");
	button_next = new WidgetButton("images/menus/buttons/right.png");

	input_name = new WidgetInput();
	input_name->max_length = 20;

	button_permadeath = new WidgetCheckBox();
	if (eset->death_penalty.permadeath) {
		button_permadeath->enabled = false;
		button_permadeath->setChecked(true);
	}

	class_list = new WidgetListBox (12);
	class_list->can_deselect = false;

	tip = new WidgetTooltip();

	// Read positions from config file
	FileParser infile;

	// @CLASS GameStateNew: Layout|Description of menus/gamenew.txt
	if (infile.open("menus/gamenew.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR button_prev|int, int, alignment : X, Y, Alignment|Position of button to choose the previous preset hero.
			if (infile.key == "button_prev") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_prev->setBasePos(x, y, a);
			}
			// @ATTR button_next|int, int, alignment : X, Y, Alignment|Position of button to choose the next preset hero.
			else if (infile.key == "button_next") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_next->setBasePos(x, y, a);
			}
			// @ATTR button_exit|int, int, alignment : X, Y, Alignment|Position of "Cancel" button.
			else if (infile.key == "button_exit") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_exit->setBasePos(x, y, a);
			}
			// @ATTR button_create|int, int, alignment : X, Y, Alignment|Position of "Create" button.
			else if (infile.key == "button_create") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_create->setBasePos(x, y, a);
			}
			// @ATTR button_permadeath|int, int, alignment : X, Y, Alignment|Position of checkbox for toggling permadeath.
			else if (infile.key == "button_permadeath") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				button_permadeath->setBasePos(x, y, a);
			}
			// @ATTR name_input|int, int, alignment : X, Y, Alignment|Position of the hero name textbox.
			else if (infile.key == "name_input") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				input_name->setBasePos(x, y, a);
			}
			// @ATTR portrait_label|label|Label for the "Choose a Portrait" text.
			else if (infile.key == "portrait_label") {
				portrait_label = eatLabelInfo(infile.val);
			}
			// @ATTR name_label|label|Label for the "Choose a Name" text.
			else if (infile.key == "name_label") {
				name_label = eatLabelInfo(infile.val);
			}
			// @ATTR permadeath_label|label|Label for the "Permadeath?" text.
			else if (infile.key == "permadeath_label") {
				permadeath_label = eatLabelInfo(infile.val);
			}
			// @ATTR classlist_label|label|Label for the "Choose a Class" text.
			else if (infile.key == "classlist_label") {
				classlist_label = eatLabelInfo(infile.val);
			}
			// @ATTR portrait|rectangle|Position and dimensions of the portrait image.
			else if (infile.key == "portrait") {
				portrait_pos = toRect(infile.val);
			}
			// @ATTR class_list|int, int, alignment : X, Y, Alignment|Position of the class list.
			else if (infile.key == "class_list") {
				int x = popFirstInt(infile.val);
				int y = popFirstInt(infile.val);
				ALIGNMENT a = parse_alignment(popFirstString(infile.val));
				class_list->setBasePos(x, y, a);
			}
			// @ATTR show_classlist|bool|Allows hiding the class list.
			else if (infile.key == "show_classlist") {
				show_classlist = toBool(infile.val);
			}
			else {
				infile.error("GameStateNew: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// set up labels
	color_normal = font->getColor("menu_normal");

	label_portrait = new WidgetLabel();
	label_portrait->setBasePos(portrait_label.x, portrait_label.y);
	label_portrait->set(portrait_label.x, portrait_label.y, portrait_label.justify, portrait_label.valign, msg->get("Choose a Portrait"), color_normal, portrait_label.font_style);

	label_name = new WidgetLabel();
	label_name->setBasePos(name_label.x, name_label.y);
	label_name->set(name_label.x, name_label.y, name_label.justify, name_label.valign, msg->get("Choose a Name"), color_normal, name_label.font_style);

	label_permadeath = new WidgetLabel();
	label_permadeath->setBasePos(permadeath_label.x, permadeath_label.y);
	label_permadeath->set(permadeath_label.x, permadeath_label.y, permadeath_label.justify, permadeath_label.valign, msg->get("Permadeath?"), color_normal, permadeath_label.font_style);

	label_classlist = new WidgetLabel();
	label_classlist->setBasePos(classlist_label.x, classlist_label.y);
	label_classlist->set(classlist_label.x, classlist_label.y, classlist_label.justify, classlist_label.valign, msg->get("Choose a Class"), color_normal, classlist_label.font_style);

	// set up class list
	for (unsigned i = 0; i < eset->hero_classes.list.size(); i++) {
		class_list->append(msg->get(eset->hero_classes.list[i].name), getClassTooltip(i));
	}

	if (!eset->hero_classes.list.empty())
		class_list->select(0);

	loadGraphics();
	loadOptions("hero_options.txt");

	loadPortrait(hero_options[0].portrait);
	setName(hero_options[0].name);
	setHeroOption(0);

	// Set up tab list
	tablist.add(button_exit);
	tablist.add(button_create);
	tablist.add(input_name);
	tablist.add(button_permadeath);
	tablist.add(button_prev);
	tablist.add(button_next);
	tablist.add(class_list);

	refreshWidgets();

	render_device->setBackgroundColor(Color(0,0,0,0));
}

void GameStateNew::loadGraphics() {
	Image *graphics;

	graphics = render_device->loadImage("images/menus/portrait_border.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		portrait_border = graphics->createSprite();
		graphics->unref();
	}
}

void GameStateNew::loadPortrait(const std::string& portrait_filename) {

	Image *graphics;

	if (portrait_image)
		delete portrait_image;

	portrait_image = NULL;
	graphics = render_device->loadImage(portrait_filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		portrait_image = graphics->createSprite();
		portrait_image->setDest(portrait_pos);
		graphics->unref();
	}
}

/**
 * Load body type "base" and portrait/head "portrait" options from a config file
 *
 * @param filename File containing entries for option=base,look
 */
void GameStateNew::loadOptions(const std::string& filename) {
	FileParser fin;
	// @CLASS GameStateNew: Hero options|Description of engine/hero_options.txt
	if (!fin.open("engine/" + filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) return;

	int cur_index = -1;
	while (fin.next()) {
		// @ATTR option|int, string, string, filename, string : Index, Base, Head, Portrait, Name|A default body, head, portrait, and name for a hero.
		if (fin.key == "option") {
			cur_index = std::max(0, popFirstInt(fin.val));

			if (static_cast<size_t>(cur_index + 1) > hero_options.size()) {
				hero_options.resize(cur_index + 1);
			}

			hero_options[cur_index].base = popFirstString(fin.val);
			hero_options[cur_index].head = popFirstString(fin.val);
			hero_options[cur_index].portrait = popFirstString(fin.val);
			hero_options[cur_index].name = msg->get(popFirstString(fin.val));

			all_options.push_back(cur_index);
		}
	}
	fin.close();

	if (hero_options.empty()) {
		hero_options.resize(1);
	}

	std::sort(all_options.begin(), all_options.end());
}

/**
 * If the name text box is empty or hasn't been user-modified, set the name
 *
 * @param default_name The name we want to use for the hero
 */
void GameStateNew:: setName(const std::string& default_name) {
	if (input_name->getText() == "" || !modified_name) {
		input_name->setText(default_name);
		modified_name = false;
	}
}

void GameStateNew::setHeroOption(int dir) {
	std::vector<int> *available_options = &all_options;

	// get the available options from the currently selected class
	int class_index;
	if ( (class_index = class_list->getSelected()) != -1) {
		if (static_cast<size_t>(class_index) < eset->hero_classes.list.size() && !eset->hero_classes.list[class_index].options.empty()) {
			available_options = &(eset->hero_classes.list[class_index].options);
		}
	}

	if (dir == 0) {
		// don't change current_option unless required
		if (std::find(available_options->begin(), available_options->end(), current_option) == available_options->end())
			current_option = available_options->front();
	}
	else if (dir == 1) {
		// increment current_option
		std::vector<int>::iterator it = std::find(available_options->begin(), available_options->end(), current_option);
		if (it == available_options->end()) {
			current_option = available_options->front();
		}
		else {
			++it;
			if (it != available_options->end())
				current_option = (*it);
			else
				current_option = available_options->front();
		}
	}
	else if (dir == -1) {
		// decrement current_option
		std::vector<int>::iterator it = std::find(available_options->begin(), available_options->end(), current_option);
		if (it == available_options->begin()) {
			current_option = available_options->back();
		}
		else {
			--it;
			current_option = (*it);
		}
	}

	loadPortrait(hero_options[current_option].portrait);
	setName(hero_options[current_option].name);
}

void GameStateNew::logic() {

	if (inpt->window_resized)
		refreshWidgets();

	if (!input_name->edit_mode)
		tablist.logic(true);

	input_name->logic();

	button_permadeath->checkClick();
	if (show_classlist && class_list->checkClick()) {
		setHeroOption(0);
	}

	// require character name
	if (input_name->getText() == "") {
		if (button_create->enabled) {
			button_create->enabled = false;
			button_create->refresh();
		}
	}
	else {
		if (!button_create->enabled) {
			button_create->enabled = true;
			button_create->refresh();
		}
	}

	if ((inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) || button_exit->checkClick()) {
		if (inpt->pressing[Input::CANCEL])
			inpt->lock[Input::CANCEL] = true;
		delete_items = false;
		showLoading();
		setRequestedGameState(new GameStateLoad());
	}

	if (button_create->checkClick()) {
		// start the new game
		inpt->lock_all = true;
		delete_items = false;
		showLoading();
		GameStatePlay* play = new GameStatePlay();
		Avatar *avatar = pc;
		avatar->stats.gfx_base = hero_options[current_option].base;
		avatar->stats.gfx_head = hero_options[current_option].head;
		avatar->stats.gfx_portrait = hero_options[current_option].portrait;
		avatar->stats.name = input_name->getText();
		avatar->stats.permadeath = button_permadeath->isChecked();
		save_load->setGameSlot(game_slot);
		play->resetGame();
		save_load->loadClass(class_list->getSelected());
		setRequestedGameState(play);
	}

	// scroll through portrait options
	if (button_next->checkClick()) {
		setHeroOption(1);
	}
	else if (button_prev->checkClick()) {
		setHeroOption(-1);
	}

	if (input_name->getText() != hero_options[current_option].name)
		modified_name = true;
}

void GameStateNew::refreshWidgets() {
	button_exit->setPos();
	button_create->setPos();

	button_prev->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_next->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	button_permadeath->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	class_list->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);

	label_portrait->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	label_name->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	label_permadeath->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
	label_classlist->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);

	input_name->setPos((settings->view_w - eset->resolutions.frame_w)/2, (settings->view_h - eset->resolutions.frame_h)/2);
}

void GameStateNew::render() {

	// display buttons
	button_exit->render();
	button_create->render();
	button_prev->render();
	button_next->render();
	input_name->render();
	button_permadeath->render();

	// display portrait option
	Rect src;
	Rect dest;

	src.w = dest.w = portrait_pos.w;
	src.h = dest.h = portrait_pos.h;
	src.x = src.y = 0;
	dest.x = portrait_pos.x + (settings->view_w - eset->resolutions.frame_w)/2;
	dest.y = portrait_pos.y + (settings->view_h - eset->resolutions.frame_h)/2;

	if (portrait_image) {
		portrait_image->setClip(src);
		portrait_image->setDest(dest);
		render_device->render(portrait_image);
		portrait_border->setClip(src);
		portrait_border->setDest(dest);
		render_device->render(portrait_border);
	}

	// display labels
	if (!portrait_label.hidden) label_portrait->render();
	if (!name_label.hidden) label_name->render();
	if (!permadeath_label.hidden) label_permadeath->render();

	// display class list
	if (show_classlist) {
		if (!classlist_label.hidden) label_classlist->render();
		class_list->render();

		TooltipData tip_new = class_list->checkTooltip(inpt->mouse);
		if (!tip_new.isEmpty()) {

			// when we render a tooltip it buffers the rasterized text for performance.
			// If this new tooltip is the same as the existing one, reuse.

			if (!tip_new.compare(&tip_buf)) {
				tip_buf.clear();
				tip_buf = tip_new;
			}
			tip->render(tip_buf, inpt->mouse, STYLE_FLOAT);
		}

	}

}

std::string GameStateNew::getClassTooltip(int index) {
	std::string tooltip;
	if (eset->hero_classes.list[index].description != "") tooltip += msg->get(eset->hero_classes.list[index].description);
	return tooltip;
}

GameStateNew::~GameStateNew() {
	if (portrait_image)
		delete portrait_image;

	if (portrait_border)
		delete portrait_border;

	if (delete_items) {
		delete items;
		items = NULL;
	}

	delete button_exit;
	delete button_create;
	delete button_next;
	delete button_prev;
	delete label_portrait;
	delete label_name;
	delete input_name;
	delete button_permadeath;
	delete label_permadeath;
	delete label_classlist;
	delete class_list;
	delete tip;
}
