/*
    Copyright (C) 2019 Christoph Roick <chrisito@gmx.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PTRACER_H
#define PTRACER_H

#include <QtGlobal>

/** On Linux, tell the process to allow the debugger to attach to it */
void setPtracer(qint64 debuggerpid, qint64 debuggeepid);

#endif
