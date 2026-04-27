//
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "shijima-qt/ShijimaManager.hpp"
#include "../../runtime/ManagerRuntimeState.hpp"
#include "../ManagerUiState.hpp"
#include "../ManagerUiHelpers.hpp"
#include <QDesktopServices>
#include <QFrame>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QSizePolicy>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include "ElaPushButton.h"
#include "ElaTheme.h"

static void configureActionButton(ElaPushButton *button)
{
    const QFontMetrics metrics(button->font());
    const int horizontalPadding = 44;
    button->setMinimumWidth(metrics.horizontalAdvance(button->text()) + horizontalPadding);
    button->setMinimumHeight(34);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

static QLabel *createMetaLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setProperty("muted", true);
    return label;
}

static void applyHomeTheme(QWidget *homePage)
{
    auto mode = eTheme->getThemeMode();
    QColor panel = eTheme->getThemeColor(mode, ElaThemeType::BasicBase);
    QColor border = eTheme->getThemeColor(mode, ElaThemeType::BasicBorder);
    QColor text = eTheme->getThemeColor(mode, ElaThemeType::BasicText);
    QColor muted = text;
    muted.setAlpha(190);
    QColor preview = eTheme->getThemeColor(mode, ElaThemeType::WindowBase);

    homePage->setStyleSheet(QString(
        "#homeActionBar, #mascotDetailsPanel {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "}"
        "#homeSectionTitle {"
        "  color: %3;"
        "  font-weight: 600;"
        "}"
        "#mascotPreview {"
        "  background-color: %5;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "}"
        "QLabel[muted=\"true\"] {"
        "  color: %4;"
        "}"
    ).arg(panel.name(QColor::HexArgb), border.name(QColor::HexArgb),
        text.name(QColor::HexArgb), muted.name(QColor::HexArgb),
        preview.name(QColor::HexArgb)));
}

void ShijimaManager::setupHomePage() {
    m_ui->homePage = new QWidget(this);
    auto *homeLayout = new QVBoxLayout(m_ui->homePage);
    homeLayout->setContentsMargins(12, 12, 12, 12);
    homeLayout->setSpacing(10);

    auto *actionBar = new QFrame(m_ui->homePage);
    actionBar->setObjectName(QStringLiteral("homeActionBar"));
    auto *actionRow = new QHBoxLayout(actionBar);
    actionRow->setContentsMargins(10, 8, 10, 8);
    actionRow->setSpacing(8);

    auto *btnSpawn = new ElaPushButton(tr("Spawn Random"));
    btnSpawn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    configureActionButton(btnSpawn);
    connect(btnSpawn, &ElaPushButton::clicked, this, &ShijimaManager::spawnClicked);
    actionRow->addWidget(btnSpawn);

    auto *btnImport = new ElaPushButton(tr("Import"));
    btnImport->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    configureActionButton(btnImport);
    connect(btnImport, &ElaPushButton::clicked, this, &ShijimaManager::importAction);
    actionRow->addWidget(btnImport);

    auto *btnRefresh = new ElaPushButton(tr("Refresh"));
    btnRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    configureActionButton(btnRefresh);
    connect(btnRefresh, &ElaPushButton::clicked, this, &ShijimaManager::syncMascotLibrary);
    actionRow->addWidget(btnRefresh);

    auto *btnDelete = new ElaPushButton(tr("Delete"));
    btnDelete->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    configureActionButton(btnDelete);
    connect(btnDelete, &ElaPushButton::clicked, this, &ShijimaManager::deleteAction);
    actionRow->addWidget(btnDelete);

    auto *btnFolder = new ElaPushButton(tr("Show Folder"));
    btnFolder->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    configureActionButton(btnFolder);
    connect(btnFolder, &ElaPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_runtime->mascotsPath));
    });
    actionRow->addWidget(btnFolder);

    actionRow->addStretch();
    homeLayout->addWidget(actionBar);

    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(10);

    auto *libraryColumn = new QWidget(m_ui->homePage);
    auto *libraryLayout = new QVBoxLayout(libraryColumn);
    libraryLayout->setContentsMargins(0, 0, 0, 0);
    libraryLayout->setSpacing(6);
    auto *libraryTitle = new QLabel(tr("Mascot Library"), libraryColumn);
    libraryTitle->setObjectName(QStringLiteral("homeSectionTitle"));
    libraryLayout->addWidget(libraryTitle);

    m_ui->listWidget->setParent(m_ui->homePage);
    m_ui->listWidget->setUniformItemSizes(true);
    libraryLayout->addWidget(m_ui->listWidget, 1);
    contentRow->addWidget(libraryColumn, 1);

    auto *details = new QFrame(m_ui->homePage);
    details->setObjectName(QStringLiteral("mascotDetailsPanel"));
    m_ui->mascotDetailsPanel = details;
    details->setMinimumWidth(220);
    details->setMaximumWidth(300);
    auto *detailsLayout = new QVBoxLayout(details);
    detailsLayout->setContentsMargins(12, 12, 12, 12);
    detailsLayout->setSpacing(8);

    auto *detailsTitle = new QLabel(tr("Details"), details);
    detailsTitle->setObjectName(QStringLiteral("homeSectionTitle"));
    detailsLayout->addWidget(detailsTitle);

    m_ui->mascotPreviewLabel = new QLabel(details);
    m_ui->mascotPreviewLabel->setObjectName(QStringLiteral("mascotPreview"));
    m_ui->mascotPreviewLabel->setFixedSize(96, 96);
    m_ui->mascotPreviewLabel->setAlignment(Qt::AlignCenter);
    detailsLayout->addWidget(m_ui->mascotPreviewLabel, 0, Qt::AlignHCenter);

    m_ui->mascotNameLabel = new QLabel(details);
    m_ui->mascotNameLabel->setWordWrap(true);
    m_ui->mascotNameLabel->setAlignment(Qt::AlignCenter);
    QFont nameFont = m_ui->mascotNameLabel->font();
    nameFont.setBold(true);
    if (nameFont.pointSize() > 0) {
        nameFont.setPointSize(nameFont.pointSize() + 1);
    }
    else if (nameFont.pixelSize() > 0) {
        nameFont.setPixelSize(nameFont.pixelSize() + 1);
    }
    else {
        nameFont.setPointSizeF(10.0);
    }
    m_ui->mascotNameLabel->setFont(nameFont);
    detailsLayout->addWidget(m_ui->mascotNameLabel);

    m_ui->mascotVersionLabel = createMetaLabel(details);
    detailsLayout->addWidget(m_ui->mascotVersionLabel);

    m_ui->mascotAuthorLabel = createMetaLabel(details);
    detailsLayout->addWidget(m_ui->mascotAuthorLabel);

    m_ui->mascotDescriptionLabel = createMetaLabel(details);
    m_ui->mascotDescriptionLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detailsLayout->addWidget(m_ui->mascotDescriptionLabel, 1);

    contentRow->addWidget(details);
    homeLayout->addLayout(contentRow, 1);
    applyHomeTheme(m_ui->homePage);
    connect(eTheme, &ElaTheme::themeModeChanged, m_ui->homePage, [this]() {
        applyHomeTheme(m_ui->homePage);
    });
    updateSelectedMascotDetails();

    addPageNode(tr("Home"), m_ui->homePage, ElaIconType::House);
}
