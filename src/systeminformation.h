/*******************************************************************
* systeminformation.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2019    Harald Sitter <sitter@kde.org>
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

#ifndef SYSTEMINFORMATION__H
#define SYSTEMINFORMATION__H

#include <QObject>

class SystemInformation: public QObject
{
    Q_OBJECT
    public:
        struct Config {
            Config();

            // Overrides m_operatingSystem value
            QString basicOperatingSystem;
            // Path to lsb_release executable
            QString lsbReleasePath;
            // Path to os-release file
            QString osReleasePath;
            // Function pointer to uname override
            void *unameFunc = nullptr;
        };

        explicit SystemInformation(Config infoConfig = Config(), QObject * parent = nullptr);
        ~SystemInformation() override;

        QString bugzillaPlatform() const;
        void setBugzillaPlatform(const QString &);

        QString operatingSystem() const;
        QString bugzillaOperatingSystem() const;

        QString distributionPrettyName() const;

        bool compiledSources() const;
        void setCompiledSources(bool);

        QString qtVersion() const;
        QString frameworksVersion() const;

        /// All helpers finished and the data is complete
        bool complete() const;

    private Q_SLOTS:
        void lsbReleaseFinished();

    private:
        QString fetchOSDetailInformation() const;
        QString fetchOSReleaseInformation();

        QString guessBugzillaPlatform(const QString&) const;

        void tryToSetBugzillaPlatform();
        void tryToSetBugzillaPlatformFromExternalInfo();

        QString     m_operatingSystem;
        QString     m_bugzillaOperatingSystem;
        QString     m_bugzillaPlatform;

        QString     m_distributionPrettyName;

        bool        m_compiledSources;

        bool m_complete; // all available data retrieved

        Config m_infoConfig;
};

#endif
