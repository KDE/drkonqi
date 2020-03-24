/*******************************************************************
* backtracewidget.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#ifndef BACKTRACEWIDGET__H
#define BACKTRACEWIDGET__H

#include <QWidget>

#include "debugpackageinstaller.h"
#include "ui_backtracewidget.h"

namespace KSyntaxHighlighting {
    class SyntaxHighlighter;
}
class BacktraceRatingWidget;
class BacktraceGenerator;

class BacktraceWidget: public QWidget
{
    Q_OBJECT

public:
    explicit BacktraceWidget(BacktraceGenerator *generator, QWidget *parent = nullptr,
                             bool showToggleBacktrace = false);

    bool canInstallDebugPackages() const;

public Q_SLOTS:
    void generateBacktrace();
    void hilightExtraDetailsLabel(bool hilight);
    void focusImproveBacktraceButton();

    void toggleBacktrace(bool show);
    void extraDetailsLinkActivated(QString link);

Q_SIGNALS:
    void stateChanged();

private:
    BacktraceGenerator * m_btGenerator = nullptr;
    Ui::Form    ui;
    BacktraceRatingWidget *   m_backtraceRatingWidget = nullptr;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;
    DebugPackageInstaller * m_debugPackageInstaller = nullptr;

    void setAsLoading();
    void adjustWindowSize();

private Q_SLOTS:
    void loadData();
    void backtraceNewLine(const QString &);

    void regenerateBacktrace();

    void saveClicked();
    void copyClicked();

    void anotherDebuggerRunning();

    void installDebugPackages();
    void debugPackageError(const QString &);
    void debugPackageCanceled();
};

#endif
