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

#include <cstdio> // XXX: TEMP

#include "UBDisplayManager.h"

#include <QScreen>
#include <QMap>

#include "frameworks/UBPlatformUtils.h"

#include "core/UBApplication.h"
#include "core/UBApplicationController.h"
#include "core/UBSettings.h"

#include "board/UBBoardView.h"
#include "board/UBBoardController.h"

#include "gui/UBBlackoutWidget.h"

#include "ui_blackoutWidget.h"

#include "core/memcheck.h"

#include "qdesktopwidget.h"

UBDisplayManager::UBDisplayManager(QObject *parent)
    : QObject(parent)
    , mControlScreenIndex(-1)
    , mDisplayScreenIndex(-1)
    , mControlWidget(0)
    , mDisplayWidget(0)
    , mDesktopWidget(0)
{
    mDesktop = qApp->desktop();  // This is deprecated!

    mScreenRoles = QMap<QScreen*, ScreenRole>();

    mUseMultiScreen = UBSettings::settings()->appUseMultiscreen->get().toBool();

    getScreenRoles();

    connect(mDesktop, &QDesktopWidget::resized, this, &UBDisplayManager::adjustScreens);
    connect(mDesktop, &QDesktopWidget::workAreaResized, this, &UBDisplayManager::adjustScreens);
}

UBDisplayManager::~UBDisplayManager() {}


int UBDisplayManager::numScreens() {
    // TODO: should this return the number of available screens
    // or just the number of managed screens?
    return qApp->screens().size();
}

int UBDisplayManager::numPreviousViews() {
    // TODO: used by src/core/UBApplicationController.cpp:105
    // maybe a max{role.page_offset} would be better??
    int n = 0;
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        if (it.value().page_offset > 0)
            n++;
    }
    return n;
}

QScreen* UBDisplayManager::controlScreen() {
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        if (it.value().primary)
            return it.key();
    }
    return 0;
}

QScreen* UBDisplayManager::displayScreen() {
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        ScreenRole role = it.value();
        if (!role.ignored && !role.primary && role.page_offset == 0)
            return it.key();
    }
    return 0;
}

QWidget* UBDisplayManager::getPageWidget(int page) {
    if (page == 0)
        return mDisplayWidget;
    if (page - 1 < mPreviousDisplayWidgets.size())
        return mPreviousDisplayWidgets.at(page - 1);
    return 0;
}

void UBDisplayManager::getScreenRoles() {
    //QSettings* settings = UBSettings::;
    QList<QScreen*> screens = qApp->screens();
    QList<QString> names = QList<QString>();
    mScreenRoles.clear();
    mPreviousScreenIndexes.clear();  // TODO: kill that
    // TODO: and now make that configurable!
    printf("<names>\n");
    for (int i = 0; i < screens.size(); i++) {
        QScreen* screen = screens[i];
        QString name = screen->name();
        names.append(name);
        ScreenRole role = ScreenRole { false, false, 0 };
        if (name == "Virtual-1") {
            role.ignored = true;
        } else if (name == "Virtual-2") {
            role.primary = true;
            mControlScreenIndex = i;  // TODO: kill that
        } else if (name == "Virtual-3") {
            mDisplayScreenIndex = i;  // TODO: kill that
            // nop
            // role.page_offset = 2;
        } else if (name == "Virtual-4") {
            role.page_offset = 1;
            mPreviousScreenIndexes.append(i);  // TODO: kill that
        }
        mScreenRoles[screen] = role;

        printf(" %d: %s\n", i, name.toUtf8().data());
        printf("  Role{primary=%d, ignored=%d, page=%d}\n", (int)role.primary, (int)role.ignored, role.page_offset);
    }
    printf("</names>\n");
}

void UBDisplayManager::setControlWidget(QWidget* pControlWidget) {
    if(hasControl() && pControlWidget && (pControlWidget != mControlWidget))
        mControlWidget = pControlWidget;
}

void UBDisplayManager::setDesktopWidget(QWidget* pControlWidget ) {
    if(pControlWidget && (pControlWidget != mControlWidget))
        mDesktopWidget = pControlWidget;
}

void UBDisplayManager::setDisplayWidget(QWidget* pDisplayWidget) {
    if(pDisplayWidget && (pDisplayWidget != mDisplayWidget)) {
        if (mDisplayWidget) {
            mDisplayWidget->hide();
            pDisplayWidget->setGeometry(mDisplayWidget->geometry());
            pDisplayWidget->setWindowFlags(mDisplayWidget->windowFlags());
        }
        mDisplayWidget = pDisplayWidget;
        mDisplayWidget->setGeometry(displayGeometry());
        if (UBSettings::settings()->appUseMultiscreen->get().toBool())
            UBPlatformUtils::showFullScreen(mDisplayWidget);
    }
}

void UBDisplayManager::setUseMultiScreen(bool pUse) {
    mUseMultiScreen = pUse;
}

void UBDisplayManager::setPreviousDisplaysWidgets(QList<UBBoardView*> pPreviousViews) {
    mPreviousDisplayWidgets = pPreviousViews;
}

void UBDisplayManager::reinitScreens(bool swap) {
    // TODO: remove that deprecated thing
    Q_UNUSED(swap);
    adjustScreens(-1);
}

void UBDisplayManager::adjustScreens(int screen)
{
    // TODO: remove the screen argument everywhere
    Q_UNUSED(screen);
    getScreenRoles();
    positionScreens();
    emit screenLayoutChanged();
}


void UBDisplayManager::positionScreens()
{
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        QScreen* screen = it.key();
        ScreenRole role = it.value();
        printf("positioning %s\n", screen->name().toUtf8().data());
        if (role.ignored)
            continue;
        if (role.primary) {
            if (mDesktopWidget) {
                mDesktopWidget->hide();
                mDesktopWidget->setGeometry(screen->geometry());
                printf("Primary desktop geometry: %dx%d\n", screen->geometry().width(), screen->geometry().height());
            }
            if (mControlWidget) {
                mControlWidget->hide();
                mControlWidget->setGeometry(screen->geometry());
                UBPlatformUtils::showFullScreen(mControlWidget);
                printf("Primary control geometry: %dx%d\n", screen->geometry().width(), screen->geometry().height());
            }
            continue;
        }
        QWidget* page = getPageWidget(role.page_offset);
        if (page) {
            page->hide();
            page->setGeometry(screen->geometry());
            UBPlatformUtils::showFullScreen(page);
            printf("page %d display geometry: %dx%d\n", role.page_offset,
                   screen->geometry().width(), screen->geometry().height());
        }
    }
    if (mControlWidget)
        mControlWidget->activateWindow();
}

void UBDisplayManager::blackout() {
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        QScreen* screen = it.key();
        ScreenRole role = it.value();
        if (role.ignored)
            continue;

        UBBlackoutWidget *blackoutWidget = new UBBlackoutWidget(); //deleted in UBDisplayManager::unBlackout
        Ui::BlackoutWidget *blackoutUi = new Ui::BlackoutWidget();
        blackoutUi->setupUi(blackoutWidget);

        connect(blackoutUi->iconButton, SIGNAL(pressed()), blackoutWidget, SLOT(doActivity()));
        connect(blackoutWidget, SIGNAL(activity()), this, SLOT(unBlackout()));

        // display Uniboard logo on main screen
        blackoutUi->iconButton->setVisible(role.primary);
        blackoutUi->labelClickToReturn->setVisible(role.primary);

        blackoutWidget->setGeometry(screen->geometry());
        mBlackoutWidgets << blackoutWidget;
    }
    
    UBPlatformUtils::fadeDisplayOut();
    
    foreach(UBBlackoutWidget *blackoutWidget, mBlackoutWidgets) {
        UBPlatformUtils::showFullScreen(blackoutWidget);
    }
}

void UBDisplayManager::unBlackout() {
    while (!mBlackoutWidgets.isEmpty())
        mBlackoutWidgets.takeFirst()->close();
        // the widget is also destroyed thanks to its Qt::WA_DeleteOnClose attribute
    UBPlatformUtils::fadeDisplayIn();
    UBApplication::boardController->freezeW3CWidgets(false);
}

