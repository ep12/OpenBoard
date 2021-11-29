/*
 * Copyright (C) 2015-2018 Département de l'Instruction Publique (DIP-SEM)
 *
 * Copyright (C) 2013 Open Education Foundation
 *
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour
 * l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of OpenBoard.
 *
 * OpenBoard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * OpenBoard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBoard. If not, see <http://www.gnu.org/licenses/>.
 */




#ifndef UBDISPLAYMANAGER_H_
#define UBDISPLAYMANAGER_H_

#include <QtGui>
#include "core/UBApplicationController.h"

class UBBlackoutWidget;
class UBBoardView;
class QDesktopWidget;

struct ScreenRole {
    bool primary;
    bool ignored;
    int page_offset;
};

class UBDisplayManager : public QObject
{
    Q_OBJECT;

    public:
        UBDisplayManager(QObject *parent = 0);
        virtual ~UBDisplayManager();

        int numScreens();

        int numPreviousViews();

        void setControlWidget(QWidget* pControlWidget);

        void setDisplayWidget(QWidget* pDisplayWidget);

        void setDesktopWidget(QWidget* pControlWidget);

        void setPreviousDisplaysWidgets(QList<UBBoardView*> pPreviousViews);

        QScreen* controlScreen();

        QScreen* displayScreen();

        bool hasControl() { return controlScreen() != 0; }

        bool hasDisplay() { return displayScreen() != 0; }

        bool hasPrevious() { return numPreviousViews() > 0; }

        bool useMultiScreen() { return mUseMultiScreen; }

        void setUseMultiScreen(bool pUse);

        QRect controlGeometry() {
            return controlScreen()->geometry();
        }
        QRect displayGeometry() {
            return displayScreen()->geometry();
        }

   signals:

           void screenLayoutChanged();
           void adjustDisplayViewsRequired();

   public slots:

        void adjustScreens();
        void adjustScreens(int screen);
        void screenGeometryChanged(QRect geometry);
        void addOrRemoveScreen(QScreen* screen);

        void blackout();

        void unBlackout();

        void desktopModeChanged(bool displayed);
        void mainModeChanged(UBApplicationController::MainMode mode);

    private:

        void positionScreens();

        void getScreenRoles();

        void connectScreenSignals();
        void disconnectScreenSignals();

        QWidget* mControlWidget;

        QWidget* mDisplayWidget;

        QWidget *mDesktopWidget;

        QList<UBBoardView*> mPreviousDisplayWidgets;

        QList<UBBlackoutWidget*> mBlackoutWidgets;

        bool mUseMultiScreen;

        bool mIsShowingDesktop;

        QWidget* getPageWidget(int page);

        QMap<QScreen*, ScreenRole> mScreenRoles;
};

#endif /* UBDISPLAYMANAGER_H_ */
