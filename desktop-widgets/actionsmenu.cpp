// SPDX-License-Identifier: GPL-2.0
#include "actionsmenu.h"

ActionsMenu::ActionsMenu(QWidget *parent) : QMenu(parent)
{
	connect(this, &ActionsMenu::clearActions, this, &ActionsMenu::clearActionsMenu);
	connect(this, &ActionsMenu::addToActions, this, &ActionsMenu::addToActionsMenu);
}

void ActionsMenu::clearActionsMenu()
{
	this->clear();
}

void ActionsMenu::addToActionsMenu(QAction *action)
{
	this->addAction(action);
}


