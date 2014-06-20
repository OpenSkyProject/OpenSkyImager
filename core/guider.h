/*
 * qhycore.h
 *
 *  Created on: 01.09.2013
 *      Author: Marco Gulino (marco@gulinux.net)
 *
 * Device access code is based on original QHYCCD.inc code from 
 * https://github.com/qhyccd-lzr
 *
 *
 * This file is part of "OpenSkyImager".
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GUIDER_H
#define GUIDER_H

enum GuiderAxis{GuideRightAscension = 0x1, GuideDeclination = 0x2};
enum GuiderMovement{GuideStop = 0, GuideIncrease = 1, GuideDecrease = -1};

#endif