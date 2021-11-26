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
    mDesktop = qApp->desktop();

    mScreenRoles = QMap<QScreen*, ScreenRole>();

    mUseMultiScreen = UBSettings::settings()->appUseMultiscreen->get().toBool();

    initScreenIndexes();

    connect(mDesktop, &QDesktopWidget::resized, this, &UBDisplayManager::adjustScreens);
    connect(mDesktop, &QDesktopWidget::workAreaResized, this, &UBDisplayManager::adjustScreens);
}

QWidget* UBDisplayManager::getPageWidget(int page) {
    return page == 0 ? mDisplayWidget : mPreviousDisplayWidgets.at(page - 1);
}

void UBDisplayManager::initScreenIndexes() {
    QList<QScreen*> screens = qApp->screens();
    QList<QString> names = QList<QString>();
    mScreenRoles.clear();
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
        } else if (name == "Virtual-3") {
            // nop
        } else if (name == "Virtual-4") {
            role.page_offset = 1;
        }
        mScreenRoles[screen] = role;

        printf(" %d: %s\n", i, name.toUtf8().data());
        printf("  Role{primary=%d, ignored=%d, page=%d}\n", (int)role.primary, (int)role.ignored, role.page_offset);
    }
    printf("</names>\n");
}

void UBDisplayManager::swapDisplayScreens(bool swap)
{
    printf("Warning: swapDisplayScreens is deprecated!\n");
    return;
    initScreenIndexes();
    if (swap)
    {
        // As it s a really specific ask and we don't have much time to handle it correctly
        // this code handles only the swap between the main display screen and the first previous one
        int displayScreenIndex = mDisplayScreenIndex;
        mDisplayScreenIndex = mPreviousScreenIndexes.at(0);
        mPreviousScreenIndexes.clear();
        mPreviousScreenIndexes.append(displayScreenIndex);
    }
    positionScreens();
    emit screenLayoutChanged();
    emit adjustDisplayViewsRequired();
}


UBDisplayManager::~UBDisplayManager() {}

int UBDisplayManager::numScreens() {
    return qApp->screens().size();
}

int UBDisplayManager::numPreviousViews() {
    return mPreviousScreenIndexes.size();
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
    if(pDisplayWidget && (pDisplayWidget != mDisplayWidget))
    {
        if (mDisplayWidget)
        {
            mDisplayWidget->hide();
            pDisplayWidget->setGeometry(mDisplayWidget->geometry());
            pDisplayWidget->setWindowFlags(mDisplayWidget->windowFlags());
        }
        mDisplayWidget = pDisplayWidget;
        //mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        mDisplayWidget->setGeometry(displayGeometry());
        if (UBSettings::settings()->appUseMultiscreen->get().toBool())
            UBPlatformUtils::showFullScreen(mDisplayWidget);
    }
}

void UBDisplayManager::setPreviousDisplaysWidgets(QList<UBBoardView*> pPreviousViews) {
    mPreviousDisplayWidgets = pPreviousViews;
}

QRect UBDisplayManager::controlGeometry() {
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        if (it.value().primary)
            return it.key()->geometry();
    }
}

QRect UBDisplayManager::displayGeometry() {
    QMapIterator<QScreen*, ScreenRole> it(mScreenRoles);
    while (it.hasNext()) {
        it.next();
        ScreenRole role = it.value();
        if (!role.ignored && !role.primary && role.page_offset == 0)
            return it.key()->geometry();
    }
}

void UBDisplayManager::reinitScreens(bool swap)
{
    Q_UNUSED(swap);
    adjustScreens(-1);
}

void UBDisplayManager::adjustScreens(int screen)
{
    Q_UNUSED(screen);
    initScreenIndexes();
    positionScreens();
    emit screenLayoutChanged();
}


void UBDisplayManager::positionScreens()
{
    // Fix for desktop on duplicated screens?
    /*for (int wi = mPreviousScreenIndexes.size(); wi < mPreviousDisplayWidgets.size(); wi++)
    {
        mPreviousDisplayWidgets.at(wi)->hide();
    }*/
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
        page->hide();
        if (mDisplayScreenIndex > -1) {
            page->setGeometry(screen->geometry());
            UBPlatformUtils::showFullScreen(page);
            printf("page %d display geometry: %dx%d\n", role.page_offset,
                   screen->geometry().width(), screen->geometry().height());
        }
    }
    if (mControlWidget && mControlScreenIndex > -1)
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


void UBDisplayManager::setRoleToScreen(DisplayRole role, int screenIndex) {
    Q_UNUSED(role);
    Q_UNUSED(screenIndex);
}


void UBDisplayManager::setUseMultiScreen(bool pUse) {
    mUseMultiScreen = pUse;
}

