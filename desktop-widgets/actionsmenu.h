// SPDX-License-Identifier: GPL-2.0
#ifndef ACTIONSMENU_H
#define ACTIONSMENU_H

#include <QMenu>

class ActionsMenu : public QMenu {
	Q_OBJECT

public:
	explicit ActionsMenu(QWidget *parent = 0);
private
slots:
        void clearActionsMenu();
        void addToActionsMenu(QAction *action);
signals:
	void clearActions();
	void addToActions(QAction *action);
};

#endif // ACTIONSMENU_H
