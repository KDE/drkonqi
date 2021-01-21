/*******************************************************************
 * applicationdetailsexamples.h
 * SPDX-FileCopyrightText: 2010 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef APPLICATIONDETAILSEXAMPLES__H
#define APPLICATIONDETAILSEXAMPLES__H

#include <QObject>
#include <QString>

class ApplicationDetailsExamples : QObject
{
    Q_OBJECT
public:
    explicit ApplicationDetailsExamples(QObject *parent);
    bool hasExamples() const;
    QString examples() const;

private:
    QString m_examples;
};

#endif
