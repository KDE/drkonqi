/*
    Copyright 2019 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QDebug>
#include <QTest>

#include <config-drkonqi.h>

#include <KCoreAddons>

#if HAVE_UNAME
# include <errno.h>
# include <sys/utsname.h>
#endif

#include <systeminformation.h>

class SystemInformationTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

#if HAVE_UNAME
    static int uname(utsname *buf)
    {
        strcpy(buf->sysname, "FreeBSD");
        strcpy(buf->release, "1.0.0");
        strcpy(buf->machine, "x86_64");
        return 0;
    }
#endif

    void initTestCase()
    {
    }

    // NOTE: bugzillaOperatingSystem is not tested anywhere because it is based
    //   on the compilation platform and testing it fairly moot because of that

    void testLsb()
    {
        SystemInformation::Config config;
        config.basicOperatingSystem = "Linux"; // other parts getting filled depends on OS
        config.lsbReleasePath = QFINDTESTDATA("lsb_release"); // double binary
        config.osReleasePath.clear();
        config.unameFunc = (void *) &uname;

        SystemInformation info(config);
        QTRY_VERIFY(info.complete());

        QCOMPARE(info.bugzillaPlatform(), "openSUSE RPMs");
        QCOMPARE(info.operatingSystem(), "FreeBSD 1.0.0 x86_64");
        QCOMPARE(info.distributionPrettyName(), "KDE SUSE User Edition 5.16");
        QCOMPARE(info.compiledSources(), false);
        QCOMPARE(info.qtVersion(), qVersion());
        QCOMPARE(info.frameworksVersion(), KCoreAddons::versionString());
    }

    void testOsRelease()
    {
        SystemInformation::Config config;
        config.basicOperatingSystem = "Linux"; // other parts getting filled depends on OS
        config.lsbReleasePath.clear();
        config.osReleasePath = QFINDTESTDATA("data/os-release"); // fixture
        config.unameFunc = (void *) &uname;

        SystemInformation info(config);
        QTRY_VERIFY(info.complete());

        QCOMPARE(info.bugzillaPlatform(), "FreeBSD Ports");
        QCOMPARE(info.operatingSystem(), "FreeBSD 1.0.0 x86_64");
        QCOMPARE(info.distributionPrettyName(), "FreeBSD #1");
        QCOMPARE(info.compiledSources(), false);
        QCOMPARE(info.qtVersion(), qVersion());
        QCOMPARE(info.frameworksVersion(), KCoreAddons::versionString());
    }
};

QTEST_MAIN(SystemInformationTest)

#include "systeminformationtest.moc"
