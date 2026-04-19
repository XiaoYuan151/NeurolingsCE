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

#include "shijima-qt/Asset.hpp"

#include <QRect>

QRect Asset::getRectForImage(QImage const& image) {
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return { 0, 0, 1, 1 };
    }

    int startX = 0;
    int endX = image.width();
    int startY = 0;
    int endY = image.height();
    int x = 0;
    int y = 0;
    bool foundOpaque = false;

    for (x = 0; x < endX && !foundOpaque; ++x) {
        for (y = 0; y < endY; ++y) {
            if (image.pixelColor(x, y).alpha() > 0) {
                foundOpaque = true;
                break;
            }
        }
    }
    if (!foundOpaque) {
        return image.rect();
    }
    startX = x - 1;

    foundOpaque = false;
    for (y = 0; y < endY && !foundOpaque; ++y) {
        for (x = startX; x < endX; ++x) {
            if (image.pixelColor(x, y).alpha() > 0) {
                foundOpaque = true;
                break;
            }
        }
    }
    startY = y - 1;

    for (x = endX - 1; x > startX; --x) {
        for (y = startY; y < endY; ++y) {
            if (image.pixelColor(x, y).alpha() > 0) {
                break;
            }
        }
        if (y != endY) {
            break;
        }
    }
    endX = x + 1;

    for (y = endY - 1; y > startY; --y) {
        for (x = startX; x < endX; ++x) {
            if (image.pixelColor(x, y).alpha() > 0) {
                break;
            }
        }
        if (x != endX) {
            break;
        }
    }
    endY = y + 1;

    return { startX, startY, endX - startX, endY - startY };
}

void Asset::setImage(QImage const& image) {
    QImage safeImage = image;
    if (safeImage.isNull() || safeImage.width() <= 0 || safeImage.height() <= 0) {
        safeImage = QImage { 1, 1, QImage::Format_ARGB32_Premultiplied };
        safeImage.fill(Qt::transparent);
    }

    m_originalSize = safeImage.size();
    auto rect = getRectForImage(safeImage);
    m_offset = rect;
    m_image = safeImage.copy(rect);
    m_mirrored = m_image.mirrored(true, false);
#ifdef __linux__
    m_mask = QBitmap::fromImage(m_image.createAlphaMask());
    m_mirroredMask = QBitmap::fromImage(m_mirrored.createAlphaMask());
#endif
}
