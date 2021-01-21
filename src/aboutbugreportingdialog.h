/*******************************************************************
 * aboutbugreportingdialog.h
 * SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 ******************************************************************/

#ifndef ABOUTBUGREPORTINGDIALOG__H
#define ABOUTBUGREPORTINGDIALOG__H

#include <QDialog>

class QTextBrowser;

class AboutBugReportingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutBugReportingDialog(QWidget *parent = nullptr);
    ~AboutBugReportingDialog() override;
    void showSection(const QString &);

private Q_SLOTS:
    void handleInternalLinks(const QUrl &url);

private:
    QTextBrowser *m_textBrowser = nullptr;
};

#endif
